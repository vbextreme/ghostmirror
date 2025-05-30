#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include <stdint.h>
#include <stdlib.h>


typedef enum { TAR_FILE, TAR_HARD_LINK, TAR_SYMBOLIC_LINK, TAR_CHAR_DEV, TAR_BLK_DEV, TAR_DIR, TAR_FIFO, TAR_CONTIGUOUS, TAR_GLOBAL, TAR_EXTEND, TAR_VENDOR } tartype_e;

typedef struct tarent_s{
	char*     path;
	size_t    size;
	tartype_e type;
	void*     data;
}tarent_s;

typedef struct tar_s{
	void*     start;
	uintptr_t loaddr;
	size_t    end;
	tarent_s  global;
	int       err;
}tar_s;

void gzip_init(unsigned maxthr);
void* gzip_decompress(void* data);

void tar_mopen(tar_s* tar, void* data);
tarent_s* tar_next(tar_s* tar);
void tar_close(tar_s* tar);
int tar_errno(tar_s* tar);

#endif
