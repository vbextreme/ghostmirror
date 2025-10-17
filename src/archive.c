#include <notstd/core.h>
#include <notstd/str.h>

#include <omp.h>
#include <gm/archive.h>

#include <archive.h>

#include <zlib-ng.h>

static zng_stream* strm;

void gzip_init(unsigned maxthr){
	strm = MANY(zng_stream, maxthr);
	m_header(strm)->len = maxthr;
	for (unsigned i = 0; i < maxthr; ++i) {
		memset(&strm[i], 0, sizeof(zng_stream));
		if (zng_inflateInit2(&strm[i], 16 + MAX_WBITS) != Z_OK) die("Unable to initialize zlib-ng");
	}
}

void gzip_end(void){
	mforeach(strm, i){
		zng_inflateEnd(&strm[i]);
	}
	m_free(strm);
}

int gzip_decompress_stream(void* ptrdec, void* data, size_t size){
	const unsigned tid = omp_get_thread_num();
	if( tid >= m_header(strm)->len ) die("internal error, tid %u >= %lu", tid, m_header(strm)->len);
	uint8_t* dec = *(void**)ptrdec;
	size_t framesize = size * 8;
	dec = m_grow(dec, framesize);
	zng_stream* s = &strm[tid];
	s->avail_in  = size;
	s->next_in   = (uint8_t*)data;
	s->avail_out = framesize;
	s->next_out  = (uint8_t*)(m_addressing(dec, m_header(dec)->len));
	
	while( 1 ){
		//dbg_info("inflate %lu -> %lu", size, framesize);
		int ret = zng_inflate(s, Z_NO_FLUSH);
		m_header(dec)->len += framesize - s->avail_out;
		*((void**)ptrdec) = dec;
		switch( ret ){
			case Z_BUF_ERROR:
			case Z_OK:
				if( s->avail_in == 0 ){
					//dbg_info("end buffer in");
					return 0;
				}
				if( s->avail_out == 0 ){
					//dbg_info("need more output data: framesize %lu", framesize);
					dec = m_grow(dec, framesize);
					framesize = m_available(dec);
					//dbg_info("available %lu", framesize);
					s->avail_out = framesize;
				}
				//dbg_info("decompressed len: %u", mem_header(dec)->len);
				s->next_out = (uint8_t*)(m_addressing(dec, m_header(dec)->len));
			break;
			
			case Z_STREAM_END  : dbg_info("stream end");    zng_inflateReset(s); return 1;
			case Z_DATA_ERROR  : dbg_error("data error");   zng_inflateReset(s); errno = EBADMSG; return -1;
			case Z_STREAM_ERROR: dbg_error("stream error"); zng_inflateReset(s); errno = ENOSTR;  return -1;
			case Z_MEM_ERROR   : dbg_error("out of mem");   zng_inflateReset(s); errno = ENOMEM;  return -1;
			default            : dbg_error("unknow");       zng_inflateReset(s); errno = EINVAL;  return -1;
		}
	}
}


void* gzip_decompress(void* data){
	const unsigned tid = omp_get_thread_num();
	if( tid >= m_header(strm)->len ) die("internal error, tid %u >= %lu", tid, m_header(strm)->len);
	size_t datasize = m_header(data)->len;
	size_t framesize = datasize * 4;
	void* dec = MANY(char, framesize);
	
	zng_stream* s = &strm[tid];
	s->avail_in  = datasize;
	s->next_in   = (uint8_t*)data;
	s->avail_out = framesize;
	s->next_out  = (uint8_t*)dec;
	
	int ret;
	do {
		ret = zng_inflate(s, Z_NO_FLUSH);
		if (ret != Z_OK && ret != Z_STREAM_END) {
			switch (ret) {
				case Z_DATA_ERROR: errno = EBADMSG; break;
				default          : errno = EINVAL; break;
			}
			m_free(dec);
			dbg_error("decompression failed %d", ret);
			return NULL;
		}
		m_header(dec)->len += framesize - s->avail_out;
		if (s->avail_out == 0) {
			dec = m_grow(dec, framesize);
			framesize = m_available(dec);
			s->avail_out = framesize;
		}
		s->next_out = (uint8_t*)(m_addressing(dec, m_header(dec)->len));
	}while( ret != Z_STREAM_END );
	zng_inflateReset(s);
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
	tar->end    = tar->loaddr + m_header(data)->len;
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
			if( ent->path ) m_free(ent->path);
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
	if( e->path ) m_free(e->path);
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
				m_header(ent)->cleanup = ent_dtor;
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
	if( pax.path ) m_free(pax.path);
	return NULL;
}

void tar_close(tar_s* tar){
	if( tar->global.path ){
		m_free(tar->global.path);
	}
}

int tar_errno(tar_s* tar){
	return tar->err;
}

















