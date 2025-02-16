#ifndef __INVESTIGATION_H__
#define __INVESTIGATION_H__

#include <notstd/opt.h>
#include <gm/arch.h>

#define IP_TEST_INTERNET_CONNECTION "8.8.8.8"

void investigate_mirrors(mirror_s* mirrors, option_s* oinv);

#endif
