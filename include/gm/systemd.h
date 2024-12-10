#ifndef __GM_SYSTEMD_H__
#define __GM_SYSTEMD_H__

#define SYSTEMD_USER_PATH       ".config/systemd/user"
#define SYSTEMD_USER_WANTS_PATH ".config/systemd/user/timers.target.wants"

#define __fclose __cleanup(file_cleanup)

#define PATH_PRIVIL 0760

void systemd_timer_set(unsigned day, const char* mirrorpath, const char* speedmode, const char* sortmode);

#endif
