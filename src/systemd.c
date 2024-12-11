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
	"After=network-online.target",
	"Wants=network-online.target",
	NULL
};
__private const char* SERVICE_SERVICE[] = {
	NULL, //ExecStart dinamic setting this property
	NULL, //Enviroment
	"Type=oneshot",
	"Restart=on-failure",
	"RestartSec=30m",
	NULL
};
__private const char* TIMER_UNIT[] = {
	"Description=ghostmirror timer, auto select archlinux mirror",
	NULL
};
__private const char* TIMER_TIMER[] = {
	NULL, //OnCalendar dinamic setting this property
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

__private int file_exists(const char* path){
	__fclose FILE* f = fopen(path, "r");
	return f ? 1 : 0;
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

__private char* sd_path(const char* name, const char* ext, int wants){
	char home[PATH_MAX];
	return str_printf("%s/%s/%s.%s", path_home(home), (wants ? SYSTEMD_USER_WANTS_PATH : SYSTEMD_USER_PATH), name, ext);
}

__private void sd_mkdir(void){
	char home[PATH_MAX];
	__free char* destdirb = str_printf("%s/%s", path_home(home), SYSTEMD_USER_PATH);
	__free char* destdirw = str_printf("%s/%s", path_home(home), SYSTEMD_USER_WANTS_PATH);
	if( !dir_exists(destdirb) ) dir_mk(destdirb);
	if( !dir_exists(destdirw) ) dir_mk(destdirw);
}

__private int sd_exists_user_config(const char* name, const char* type){
	__free char* servicePath = sd_path(name, type, 0);
	return file_exists(servicePath);
}

__private int sd_exists_wants(const char* name, const char* type){
	__free char* servicePath = sd_path(name, type, 1);
	return file_exists(servicePath);
}

__private FILE* sd_create_user_config(const char* name, const char* type){
	__free char* servicePath = sd_path(name, type, 0);
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

	{
		__sderr sd_bus_error err = SD_BUS_ERROR_NULL;
		ret = sd_bus_call_method(bus,
			"org.freedesktop.systemd1",          // service name
			"/org/freedesktop/systemd1",         // object
			"org.freedesktop.systemd1.Manager",  // interface
			"Reload",                            // method
			&err,                                // error
			NULL,                                // no return value
			""
		);
		sdbus_check("reload timer", ret, &err);
	}

	{
		__free char* unit = str_printf("%s.timer", name);
		__sderr sd_bus_error err = SD_BUS_ERROR_NULL;
		dbg_info("restart %s", unit);
		ret = sd_bus_call_method(bus,
			"org.freedesktop.systemd1",
			"/org/freedesktop/systemd1",
			"org.freedesktop.systemd1.Manager",
			"RestartUnit",
			&err,
			NULL,
			"ss",
			unit, 
			"replace"
		);
		sdbus_check("restart timer", ret, &err);
	}
}


/*
systemctl --user enable myapp.timer
systemctl --user start myapp.timer
~/.config/systemd/user/default.target.wants/
*/

__private void sdbus_unmask_timer(const char *name) {
    __sdbus sd_bus *bus = NULL;
	__sderr sd_bus_error err = SD_BUS_ERROR_NULL;

	int ret = sd_bus_open_user(&bus);
	if( ret < 0 ) die("sd-bus connection error: %s", strerror(-ret));
	__free char* unit = str_printf("%s.timer", name);
	dbg_info("unmask timer %s", unit);

	ret = sd_bus_call_method(bus,
		"org.freedesktop.systemd1",
		"/org/freedesktop/systemd1",
		"org.freedesktop.systemd1.Manager",
		"UnmaskUnitFiles",
		&err,
		NULL,
		"asb", 
		1,
		unit,
		0
	);
	sdbus_check("enable unit", ret, &err);
}


//abilita
__private void sd_enable_timer(const char *name, int enable) {
    __sdbus sd_bus *bus = NULL;
	__sderr sd_bus_error err = SD_BUS_ERROR_NULL;

	int ret = sd_bus_open_user(&bus);
	if( ret < 0 ) die("sd-bus connection error: %s", strerror(-ret));
	__free char* unit = str_printf("%s.timer", name);
	dbg_info("enable(%d) timer %s", enable, unit);

	if( enable ){
		ret = sd_bus_call_method(bus,
			"org.freedesktop.systemd1",
			"/org/freedesktop/systemd1",
			"org.freedesktop.systemd1.Manager",
			"EnableUnitFiles",
			&err,
			NULL,
			"asbb", 
			1,
			unit,
			0,									 // (0=permanent, 1=runtime)
			1                                    // Enable (0=no, 1=yes)
		);
	}
	else{
		ret = sd_bus_call_method(bus,
			"org.freedesktop.systemd1",
			"/org/freedesktop/systemd1",
			"org.freedesktop.systemd1.Manager",
			"DisableUnitFiles",
			&err,
			NULL,
			"asb", 
			1,
			unit,
			1
		);
	}
	sdbus_check("enable/disable unit", ret, &err);
}

__private void sd_start_timer(const char *name, int start){
    __sdbus sd_bus *bus = NULL;
	__sderr sd_bus_error err = SD_BUS_ERROR_NULL;

	int ret = sd_bus_open_user(&bus);
	if( ret < 0 ) die("sd-bus connection error: %s", strerror(-ret));
	__free char* unit = str_printf("%s.timer", name);

	dbg_info("start(%d) timer %s", start, unit);

	ret = sd_bus_call_method(bus,
		"org.freedesktop.systemd1",
		"/org/freedesktop/systemd1",
		"org.freedesktop.systemd1.Manager",
		(start ? "StartUnit" : "StopUnit"),
		&err,
		NULL,
		"ss",
		unit,
		"replace"                          // (replace, fail, isolate, ecc.)
	);
	sdbus_check("start/stop timer", ret, &err);
}

__private const char* systemd_service_version(void){
	char *hd;
	if( (hd = getenv("SERVICE_VERSION")) == NULL ) return "0.0.0";
	return hd;
}

void systemd_timer_set(unsigned day, const char* mirrorpath, const char* speedmode, const char* sortmode){
	unsigned enableService = 0;
	int uid = getuid();
	
	dbg_info("systemd serviceV%s appV:%s uid:%u day:%u path:%s speed:%s sortmode:%s", systemd_service_version(), VERSION_STR, uid, day, mirrorpath, speedmode, sortmode);
	
	if( !sdbus_user_linger_get(uid) ){
		dbg_info("linger is not enabled, I'll take care of it.");
		sdbus_user_linger_set(uid, 1);
	}
	
	int forcereconfig = strcmp(systemd_service_version(), VERSION_STR);
	
	if( forcereconfig && sd_exists_wants("ghostmirror", "timer") ){
		sd_start_timer("ghostmirror", 0);
		sd_enable_timer("ghostmirror", 0);
	}

	if( !sd_exists_wants("ghostmirror", "timer") ){
		sd_mkdir();
		if( forcereconfig || !sd_exists_user_config("ghostmirror", "service") ){
			dbg_info("create service config, forced: %u", forcereconfig);
			__free char* execstart  = str_printf("ExecStart=/usr/bin/ghostmirror -DumlsS %s %s %s %s", mirrorpath, mirrorpath, speedmode, sortmode);
			__free char* enviroment = str_printf(ENVIROMENT_FORMAT, VERSION_STR);
			SERVICE_SERVICE[0] = execstart;
			SERVICE_SERVICE[1] = enviroment;
			__fclose FILE* f = sd_create_user_config("ghostmirror", "service");
			sd_write_section(f, "Unit", SERVICE_UNIT);
			sd_write_section(f, "Service", SERVICE_SERVICE);
		}
		enableService = 1;
	}

	time_t expired = time(NULL);// + day * 86400;
	struct tm* sexpired = gmtime(&expired);
	__free char* oncalendar = str_printf(ONCALENDAR_FORMAT, sexpired->tm_mday, day);
	TIMER_TIMER[0] = oncalendar;
	
	{
		dbg_info("create timer config");
		__fclose FILE* f = sd_create_user_config("ghostmirror", "timer");
		sd_write_section(f, "Unit", TIMER_UNIT);
		sd_write_section(f, "Timer", TIMER_TIMER);
		sd_write_section(f, "Install", TIMER_INSTALL);
	}

	if( enableService ){
		sdbus_unmask_timer("ghostmirror");
		sd_enable_timer("ghostmirror", 1);
	}
	sd_daemon_reload_restart_timer("ghostmirror");
}

long systemd_restart_count(void){
	char *hd;
	if( (hd = getenv("RESTART_COUNT")) == NULL ) return -1;
	errno = 0;
	char* end = NULL;
	long ret = strtol(hd, &end, 10);
	if( !end || *end || errno ) return -1;
	return ret;
}



















