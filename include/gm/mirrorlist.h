#ifndef __GM_MIRRORLIST_H__
#define __GM_MIRRORLIST_H__

#include <notstd/core.h>

#define MIRROR_TYPE_HTTP   0x1
#define MIRROR_TYPE_HTTPS  0x2
#define MIRROR_LIST_URL    "https://archlinux.org/mirrorlist/all/"

char* mirrorlist_load(const char* fname);
char* mirrorlist_download(const unsigned tos);
const char* mirrorlist_find_country(const char* str, const char* country);
const char* mirrorlist_country_next(const char* str);
char* mirrorlist_server_next(const char** pline, int uncommented, unsigned type);
char* mirrorlist_country_dup(const char* p);
const char* mirrorlist_find_country_byurl(const char* mirrorlist, const char* url);

#endif
