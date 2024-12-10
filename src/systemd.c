#include <notstd/core.h>
#include <notstd/str.h>

#include <gm/systemd.h>

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <systemd/sd-bus.h>
#include <unistd.h>
#include <dirent.h>

/* loginctl enable-linger your_username */

#define __sdbus __cleanup(sd_bus_flush_close_unrefp)
#define __sderr __cleanup(sd_bus_error_free)
#define __sdmsg __cleanup(sd_bus_message_unrefp)

char* path_home(char* path);

__private const char* SERVICE_UNIT[] = {
	"Description=Execute ghost mirror",
	"After=network.target",
	NULL
};
__private const char* SERVICE_SERVICE[] = {
	NULL,
	"Type=oneshot",
	NULL
};
__private const char* TIMER_UNIT[] = {
	"Description=ghostmirror timer, auto select archlinux mirror",
	NULL
};
__private const char* TIMER_TIMER[] = {
	"OnCalendar=*-*-* *:*:00:",
	"Persistent=true",
	NULL
};
__private const char* TIMER_INSTALL[] = {
	"WantedBy=timers.target",
	NULL
};

void file_cleanup(void* ppf){
	FILE* f = *(void**)ppf;
	if( f ) fclose(f);
}

__private int dir_exists(const char* path){
	DIR* d = opendir(path);
	if( !d ) return errno == ENOENT ? 0 : 1;
	closedir(d);
	return 1;
}

__private void dir_mk(const char* path){
	const char* p;
	unsigned len = 0;
	unsigned next = 0;
	unsigned lend = 0;
	char pout[PATH_MAX];
	if( *path == '/' ) ++path;
	while( *(p=str_tok(path, "/", 0, &len, &next)) ){
		if( lend + len + 1 > PATH_MAX ) die("you like to much create a big path");
		pout[lend++] = '/';
		strncpy(&pout[lend], p, len);
		lend += len;
		pout[lend] = 0;
		if( !dir_exists(pout) ){
			dbg_info("mkdir: %s", pout);
			mkdir(pout, PATH_PRIVIL);
		}
	}
}

__private char* sd_path(const char* name, const char* ext){
	char home[PATH_MAX];
	return str_printf("%s/%s/%s.%s", path_home(home), SYSTEMD_USER_PATH, name, ext);
}

__private void sd_mkdir(void){
	char home[PATH_MAX];
	__free char* destdir = str_printf("%s/%s", path_home(home), SYSTEMD_USER_PATH);
	dir_mk(destdir);
}

__private int sd_exists_user_config(const char* name, const char* type){
	__free char* servicePath = sd_path(name, type);
	__fclose FILE* f = fopen(servicePath, "r");
	return f ? 1 : 0;
}

__private FILE* sd_create_user_config(const char* name, const char* type){
	__free char* servicePath = sd_path(name, type);
	FILE* f = fopen(servicePath, "w");
	if( !f ) die("unable to open file '%s': %s", servicePath, strerror(errno));
	dbg_info("created %s", servicePath);
	return f;
}

__private void sd_write_section(FILE* f, const char* section, const char** list){
	fprintf(f, "[%s]\n", section);
	for( unsigned i = 0; list[i]; ++i ){
		fprintf(f, "%s\n", list[i]);
	}
	fputc('\n', f);
}

__private void sdbus_check(const char* desc, int err, sd_bus_error* sderr){
	if( err < 0 || sd_bus_error_is_set(sderr) ){
		if( sd_bus_error_is_set(sderr) ) die("%s systemd dbus error(%d:%s): %s::%s", desc, err, strerror(-err), sderr->name, sderr->message);
		else die("%s systemd dbus error(%d): %s", desc, err, strerror(-err));
	}
}

__private int sdbus_user_linger_get(uid_t uid){
	__sdbus sd_bus *bus = NULL;
	int linger = 0;
	const char* path = NULL;

	int ret = sd_bus_open_system(&bus);
    if( ret < 0 ) die("sd-bus connection error: %s", strerror(-ret));

	{
		//logind
		__sdmsg sd_bus_message *reply = NULL;
		__sderr sd_bus_error error = SD_BUS_ERROR_NULL;
		ret = sd_bus_call_method(bus,
			"org.freedesktop.login1",
			"/org/freedesktop/login1",
			"org.freedesktop.login1.Manager",
			"GetUser",
			&error,
			&reply,
		    "u",
			uid
		);
		sdbus_check("logind.getuser", ret, &error);
		// /org/freedesktop/login1/user/_UID_
		if( (ret=sd_bus_message_read(reply, "o", &path)) < 0 ) die("read systemd reply.getuser: %s", strerror(-ret));
	}
	{
		__sdmsg sd_bus_message *reply = NULL;
		__sderr sd_bus_error error = SD_BUS_ERROR_NULL;
		ret = sd_bus_get_property(bus,
			"org.freedesktop.login1",
			path,
			"org.freedesktop.login1.User",
			"Linger",
			&error,
			&reply,
			"b"
		);
		sdbus_check("logind.linger", ret, &error);
		if( (ret = sd_bus_message_read(reply, "b", &linger)) < 0 ) die("read systemd reply.linger: %s", strerror(-ret));
    }

	return linger;
}

__private void sdbus_user_linger_set(uid_t uid, int enable){
	enable = !!enable;
	sd_bus *bus = NULL;
	int ret = sd_bus_open_system(&bus);
    if( ret < 0 ) die("sd-bus connection error: %s", strerror(-ret));

	{
		__sderr sd_bus_error error = SD_BUS_ERROR_NULL;
		ret = sd_bus_call_method(bus,
			"org.freedesktop.login1",
			"/org/freedesktop/login1",
			"org.freedesktop.login1.Manager",
			"SetUserLinger",
			&error,
			NULL,
			"ubb",
			uid,
			enable,
			0
		);
		sdbus_check("logind.setuserlinger", ret, &error);
    }
}

/*
systemctl --user daemon-reload
systemctl --user restart myapp.timer
*/
__private void sd_daemon_reload_restart_timer(const char *name) {
	__sdbus sd_bus *bus = NULL;
	int ret = sd_bus_open_user(&bus);
    if( ret < 0 ) die("sd-bus connection error: %s", strerror(-ret));

	ret = sd_bus_call_method(bus,
		"org.freedesktop.systemd1",          // service name
		"/org/freedesktop/systemd1",         // object
		"org.freedesktop.systemd1.Manager",  // interface
		"Reload",                            // method
		NULL,                                // cotext
		NULL,                                // no return value
		""
	);
	if( ret < 0 ) die("reload systemd timer error: %s\n", strerror(-ret));

	__free char* unit = str_printf("%s.timer", name);
	ret = sd_bus_call_method(bus,
		"org.freedesktop.systemd1",
		"/org/freedesktop/systemd1",
		"org.freedesktop.systemd1.Manager",
		"RestartUnit",
		NULL,
		NULL,
		"ss",                                // arguments
        unit, "replace"
	);
	if( ret < 0 ) die("restart systemd timer %s error: %s\n", unit, strerror(-ret));
}


/*
systemctl --user enable myapp.timer
systemctl --user start myapp.timer
~/.config/systemd/user/default.target.wants/
*/
//abilita
__private void sd_enable_timer(const char *name) {
    __sdbus sd_bus *bus = NULL;
	int ret = sd_bus_open_user(&bus);
	if( ret < 0 ) die("sd-bus connection error: %s", strerror(-ret));
	__free char* unit = str_printf("%s.timer", name);
	ret = sd_bus_call_method(bus,
		"org.freedesktop.systemd1",
		"/org/freedesktop/systemd1",
		"org.freedesktop.systemd1.Manager",
		"EnableUnitFiles",
		NULL,               
		NULL,
		"asbb", 
		1,
		&unit,
		1,									 // (0=permanent, 1=runtime)
		1                                    // Enable (0=no, 1=yes)
	);
	if( ret < 0 ) die("enable systemd unit file %s error: %s\n", unit, strerror(-ret));
}

/*
__private void sd_start_timer(const char *name) {
    __sdbus sd_bus *bus = NULL;
	int ret = sd_bus_open_user(&bus);
	if( ret < 0 ) die("sd-bus connection error: %s", strerror(-ret));
	__free char* unit = str_printf("%s.timer", name);

	ret = sd_bus_call_method(bus,
		"org.freedesktop.systemd1",
		"/org/freedesktop/systemd1",
		"org.freedesktop.systemd1.Manager",
		"StartUnit",
		NULL,
		NULL,
		"ss",
		unit,
		"replace"                          // (replace, fail, isolate, ecc.)
	);
	if( ret < 0 ) die("start systemd unit file %s error: %s\n", unit, strerror(-ret));
}
*/

void systemd_timer_set(unsigned day, const char* mirrorpath, const char* speedmode, const char* sortmode){
	unsigned enableService = 0;
	int uid = getuid();

	dbg_info("systemd uid:%u day:%u path:%s speed:%s sortmode:%s", uid, day, mirrorpath, speedmode, sortmode);
	
	if( !sdbus_user_linger_get(uid) ){
		dbg_info("linger is not enabled, I'll take care of it.");
		sdbus_user_linger_set(uid, 1);
	}
	
	if( !sd_exists_user_config("ghostmirror", "service") ){
		sd_mkdir();
		__free char* execstart = str_printf("ExecStart=/usr/bin/ghostmirror -DumlsS %s %s %s %s", mirrorpath, mirrorpath, speedmode, sortmode);
		SERVICE_SERVICE[0] = execstart;
		__fclose FILE* f = sd_create_user_config("ghostmirror", "service");
		sd_write_section(f, "Unit", SERVICE_UNIT);
		sd_write_section(f, "Service", SERVICE_SERVICE);
		enableService = 1;
	}

	time_t expired = time(NULL) + day * 86400;
	struct tm* sexpired = gmtime(&expired);
	__free char* oncalendar = str_printf("OnCalendar=%u-%u-%u *:*:00:", sexpired->tm_year + 1900, sexpired->tm_mon + 1, sexpired->tm_mday);
	TIMER_TIMER[0] = oncalendar;

	__fclose FILE* f = sd_create_user_config("ghostmirror", "timer");
	sd_write_section(f, "Unit", TIMER_UNIT);
	sd_write_section(f, "Timer", TIMER_TIMER);
	sd_write_section(f, "Install", TIMER_INSTALL);
	
	if( enableService ) sd_enable_timer("ghostmirror");
	sd_daemon_reload_restart_timer("ghostmirror");
}
















