#ifndef __ARCH_H__
#define __ARCH_H__

#include <limits.h>
#include <stdint.h>
#include <time.h>

#define DOWNLOAD_RETRY    3
#define DOWNLOAD_WAIT     1000

typedef enum { MIRROR_UNKNOW, MIRROR_LOCAL, MIRROR_ERR } mirrorStatus_e;

#define MIRROR_TYPE_HTTP  0x1
#define MIRROR_TYPE_HTTPS 0x2

#define SPEED_LIGHT  "git"
#define SPEED_NORMAL "chromium"
#define SPEED_HEAVY  "linux-firmware"
#define SPEED_MAX    3

typedef enum {
	FIELD_OUTOFDATE,
	FIELD_UPTODATE,
	FIELD_MORERECENT,
	FIELD_SYNC,
	FIELD_RETRY,
	FIELD_TOTAL,
	FIELD_VIRTUAL_SPEED,
	FIELD_COUNT
}field_e;

typedef struct pkgdesc{
	char   filename[NAME_MAX];
	char   name[NAME_MAX];
	char   version[NAME_MAX];
	time_t lastsync;
}pkgdesc_s;

typedef struct repo{
	pkgdesc_s* db;
	char**     ls;
}repo_s;

typedef struct mirror{
	mirrorStatus_e status;
	char*          country;
	char*          url;
	const char*    arch;
	repo_s         repo[2];
	double         speed;
	unsigned       rfield[FIELD_COUNT];
}mirror_s;

char* mirror_loading(const char* fname, const unsigned tos);
mirror_s* mirrors_country(mirror_s* mirrors, const char* mirrorlist, const char* safemirrorlist, const char* country, const char* arch, int uncommented, unsigned type);
void mirrors_update(mirror_s* mirrors, const int progress, const unsigned ndownload, const unsigned tos);
void mirrors_cmp_db(mirror_s* mirrors, const int progress);
void add_sort_mode(const char* mode);
void mirrors_sort(mirror_s* mirrors);
void mirrors_update_sync(mirror_s* mirrors, const char mode, const unsigned maxdownload, const unsigned touts, const int progress);
void country_list(const char* mirrorlist);
void mirrors_speed(mirror_s* mirrors, const char* arch, int progress, unsigned type);






#endif
