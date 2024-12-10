#include <notstd/core.h>
#include <notstd/str.h>

#include <gm/archive.h>

#include <archive.h>
#include <zlib.h>

void* gzip_decompress(void* data) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    if( inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK ) die("Unable to initialize zlib");
	size_t datasize = mem_header(data)->len;
	size_t framesize = datasize * 2;
    void* dec = MANY(char, framesize);

    strm.avail_in  = datasize;
    strm.next_in   = (Bytef*)data;
    strm.avail_out = framesize;
    strm.next_out  = (Bytef*)dec;

    int ret;
    do{
        if( (ret=inflate(&strm, Z_NO_FLUSH)) == Z_ERRNO ){
			mem_free(dec);
			dbg_error("decompression failed");
			return NULL;
		}
		mem_header(dec)->len += framesize - strm.avail_out;
		if (strm.avail_out == 0) {
			dec = mem_upsize(dec, framesize);
			framesize = mem_available(dec);
			strm.avail_out = framesize;
		}
		strm.next_out  = (Bytef*)(mem_addressing(dec, mem_header(dec)->len));
    }while( ret != Z_STREAM_END );

    inflateEnd(&strm);
    return dec;
}

#define TAR_BLK  148
#define TAR_CHK  8
#define TAR_SIZE 512
#define TAR_MAGIC "ustar"

typedef struct htar_s{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char pad[12];
}htar_s;

__private htar_s zerotar;

void tar_mopen(tar_s* tar, void* data){
	tar->start  = data;
	tar->loaddr = (uintptr_t)data;
	tar->end    = tar->loaddr + mem_header(data)->len;
	tar->err    = 0;
	//dbg_info("start: %lu end: %lu tot: %lu n: %lu", tar->loaddr, tar->end, tar->end-tar->loaddr, (tar->end-tar->loaddr)/512);
	memset(&tar->global, 0, sizeof tar->global);
}

__private unsigned tar_checksum(void* data){
	uint8_t* d = (uint8_t*) data;
	unsigned i;
	unsigned chk = 0;

	for( i = 0; i < TAR_BLK; ++i )
		chk += d[i];
	for( unsigned k = 0; k < TAR_CHK; ++k )
		chk += ' ';
	i += TAR_CHK;
	for( ; i < sizeof(htar_s); ++i )
		chk += d[i];
	
	return chk;
}

__private htar_s* htar_get(tar_s* tar){
	if( tar->loaddr >= tar->end ){
		dbg_error("out of tar bound");
		tar->err = ENOENT;
		return NULL;
	}
	htar_s* h = (htar_s*)tar->loaddr;
	if( !memcmp(h, &zerotar, sizeof zerotar) ){
		tar->loaddr += sizeof(htar_s);
		if( tar->loaddr >= tar->end ){
			dbg_error("no more data");
			tar->err = ENOENT;
			return NULL;
		}
		h = (htar_s*)tar->loaddr;
		if( !memcmp(h, &zerotar, sizeof zerotar) ){
			//dbg_info("end of tar");
			return NULL;
		}
		dbg_error("aspected end block");
		tar->err = EBADF;
		return NULL;
	}

	unsigned chk = strtoul(h->checksum, NULL, 8);
	if( chk != tar_checksum(h) ){
		dbg_error("wrong checksum");
		tar->err = EBADE;
		return NULL;
	}
	if( strcmp(h->magic, TAR_MAGIC) ){
		dbg_error("wrong magic");
		tar->err = ENOEXEC;
		return NULL;
	}
	return h;
}

__private int htar_pax(tar_s* tar, htar_s* h, tarent_s* ent){
	unsigned size = strtoul(h->size, NULL, 8);
	char* kv  = (char*)((uintptr_t)h + sizeof(htar_s));
	char* ekv = kv + size;
	while( kv < ekv ){
		unsigned kvsize = strtoul(kv, &kv, 10);
		++kv;
		char* k = kv;
		char* ek = strchr(k, '=');
		if( !ek ){
			tar->err = EINVAL;
			dbg_error("aspected assign: '%s'", kv);
			return -1;
		}
		char* v = ek+1;
		kv = k + kvsize;
		char* ev = kv - 1;

		if( !strncmp(k, "size", 4) ){
			ent->size = strtoul(v, NULL, 10);
		}
		else if( !strncmp(k, "path", 4) ){
			if( ent->path ) mem_free(ent->path);
			ent->path = MANY(char, (ev-v) + 1);
			memcpy(ent->path, v, ev - v);
			ent->path[ev-v] = 0;
		}
		else{
			//TODO
		}
	}

	return 0;
}

__private void htar_next_ent(tar_s* tar, tarent_s* ent){
	const size_t rawsize = ROUND_UP(ent->size, sizeof(htar_s));
	tar->loaddr += sizeof(htar_s) + rawsize;
}

__private void htar_next_htar(tar_s* tar, htar_s* h){
	size_t s = strtoul(h->size, NULL, 8);
	const size_t rawsize = ROUND_UP(s, sizeof(htar_s));
	tar->loaddr += sizeof(htar_s) + rawsize;
}

__private void ent_dtor(void* ent){
	tarent_s* e = ent;
	if( e->path ) mem_free(e->path);
}

tarent_s* tar_next(tar_s* tar){
	htar_s* h;
	tarent_s pax = {0};
 	tarent_s* ent;

	while( (h = htar_get(tar)) ){
		switch( h->typeflag ){
			case 'g':
				if( htar_pax(tar, h, &tar->global) ) goto ONERR;
				htar_next_htar(tar, h);
			break;

			case 'x':
				if( htar_pax(tar, h, &pax) ) goto ONERR;
				htar_next_htar(tar, h);
			break;

			case '0' ... '9':
				ent = NEW(tarent_s);
				mem_header(ent)->cleanup = ent_dtor;
				memset(ent, 0, sizeof(tarent_s));

				ent->type = h->typeflag - '0';
				if( pax.size > 0 ){
					ent->size = pax.size;
				}
				else if( tar->global.size > 0 ){
					ent->size = tar->global.size;
				}
				else{
					ent->size = strtoul(h->size, NULL, 8);
				}
				ent->data = ent->size ? (void*)(tar->loaddr + sizeof(htar_s)) : NULL;

				if( pax.path ){
					ent->path = pax.path;
					pax.path = NULL;
				}
				else if( tar->global.path ){
					ent->path = str_dup(tar->global.path, 0);
				}
				else{
					if( h->prefix[0] ){
						size_t pl = strnlen(h->prefix, sizeof h->prefix);
						size_t nl = strnlen(h->name, sizeof h->name);
						ent->path = MANY(char, pl+nl+2);
						memcpy(ent->path, h->prefix, pl);
						ent->path[pl] = '/';
						memcpy(&ent->path[pl+1], h->name, nl);
						ent->path[pl+nl+1] = 0;
					}
					else{
						size_t nl = strnlen(h->name, sizeof h->name);
						ent->path = MANY(char, nl+1);
						memcpy(ent->path, h->name, nl);
						ent->path[nl] = 0;
					}
				}
				htar_next_ent(tar, ent);
			return ent;

			default:
				dbg_error("unknow type");
				goto ONERR;
			break;
		}
	}
	
ONERR:
	if( pax.path ) mem_free(pax.path);
	return NULL;
}

void tar_close(tar_s* tar){
	if( tar->global.path ){
		mem_free(tar->global.path);
	}
}

int tar_errno(tar_s* tar){
	return tar->err;
}

















