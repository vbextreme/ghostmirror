#ifndef __ARCH_H__
#define __ARCH_H__

#include <limits.h>
#include <stdint.h>
#include <time.h>

#define VERSION_PART_MAJOR    3
#define VERSION_PART_MINOR    2
#define VERSION_PART_PATCHES  1
#define VERSION_PART_REVISION 0

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
	char*          url;
	const char*    arch;
	repo_s         repo[2];
	double         speed;
	unsigned       totalpkg;
	unsigned       syncdatepkg;
	unsigned       outofdatepkg;
	unsigned       uptodatepkg;
	unsigned       notexistspkg;
	unsigned       extrapkg;
	unsigned       checked;	
}mirror_s;

char* mirror_loading(const char* fname, const unsigned tos);
mirror_s* mirrors_country(mirror_s* mirrors, const char* mirrorlist, const char* country, const char* arch, int uncommented);
void mirrors_update(mirror_s* mirrors, const int progress, const unsigned ndownload, const unsigned tos);
void mirrors_cmp_db(mirror_s* mirrors, const int progress);
void mirrors_update_sync(mirror_s* mirrors, const char mode, const unsigned maxdownload, const unsigned touts, const int progress);
void country_list(const char* mirrorlist);
void mirrors_speed(mirror_s* mirrors, const char* arch, int progress);







#endif
