#ifndef __GM_SYSTEMD_H__
#define __GM_SYSTEMD_H__

#define SYSTEMD_USER_PATH       ".config/systemd/user"
#define SYSTEMD_USER_WANTS_PATH ".config/systemd/user/timers.target.wants"
#define ONCALENDAR_FORMAT       "OnCalendar=*-*-%u/%u"
#define ENVIROMENT_FORMAT       "Enviroment=RESTART_COUNT,SERVICE_VERSION=%s"

#define SYSTEMD_SERVICE_RETRY_MAX 3

#define __fclose __cleanup(file_cleanup)

#define PATH_PRIVIL 0760

void systemd_timer_set(unsigned day, const char* mirrorpath, const char* speedmode, const char* sortmode);
long systemd_restart_count(void);

#endif
