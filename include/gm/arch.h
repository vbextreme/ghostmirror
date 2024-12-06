#ifndef __ARCH_H__
#define __ARCH_H__

#include <limits.h>
#include <stdint.h>
#include <time.h>

#define DOWNLOAD_RETRY    3
#define DOWNLOAD_WAIT     1000

#define MIRROR_LIST_URL   "https://archlinux.org/mirrorlist/all/"
#define MIRROR_TYPE_HTTP  0x1
#define MIRROR_TYPE_HTTPS 0x2

//TODO pacman mirrorlist is not a default path need to parse pacman config and get the include directory for each db
#define PACMAN_MIRRORLIST "/etc/pacman.d/mirrorlist"
#define PACMAN_LOCAL_DB   "/var/lib/pacman/sync"

#define SPEED_LIGHT  "git"
#define SPEED_NORMAL "chromium"
#define SPEED_HEAVY  "linux-firmware"

#define WEIGHT_OUTOFDATE  49.0
#define WEIGHT_MOREUPDATE 49.0
#define WEIGHT_SPEED      2.0

typedef enum { MIRROR_UNKNOW, MIRROR_LOCAL, MIRROR_ERR } mirrorStatus_e;

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
	char*          proxy;
	int            isproxy;
	const char*    arch;
	repo_s         repo[2];
	double         speed;
	double         stability;
	unsigned       outofdate;
	unsigned       uptodate;
	unsigned       morerecent;
	unsigned       sync;
	unsigned       retry;
	unsigned       total;
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
void mirrors_stability(mirror_s* mirrors);



#endif
