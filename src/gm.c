#include <notstd/core.h>
#include <notstd/str.h>
#include <notstd/opt.h>

#include <gm/www.h>
#include <gm/archive.h>
#include <gm/arch.h>
#include <gm/mirrorlist.h>
#include <gm/investigation.h>
#include <gm/systemd.h>
#include <gm/gm.h>
#include <gm/inutility.h>

#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#define DEFAULT_THREADS 4
#define DEFAULT_TOUT    3
#define DEFAULT_ARCH    "x86_64"

//TODO
//  create generic term table 
//	creating lock for protect
//  try streaming curl and archive in one time (remove intermedie buffer)
//  try --estimated-history
//
//	time:
//	before 15.4 with retry TIMEOUT bug 12/13s
//	without retry 6/8s
//
//
//  systemd: false
//    step1: ghostmirror -PoclLS Italy,Germany,France ./mirrorlist.new 30 state,outofdate,morerecent,ping
//    step2: ghostmirror -PmuolsS  ./mirrorlist.new ./mirrorlist.new light state,outofdate,morerecent,extimated,speed
//  step3.a: sudo cp /etc/pacman.d/mirrorlist /etc/pacman.d/mirrorlist.old
//  step3.b: sudo mv ./mirrorlist.new /etc/pacman.d/mirrorlist
//
//	systemd: yes
//  step0.a: mkdir ~/.config/ghostmirror
//  step0.b: edit /etc/pacman.conf
//	         [core]
//	         Include = /home/vbextreme/.config/ghostmirror/mirrorlist
//
//	         [extra]
//	         Include = /home/vbextreme/.config/ghostmirror/mirrorlist
//	         
//    step1: ghostmirror -PoclLS Italy,Germany,France ~/.config/ghostmirror/mirrorlist 30 state,outofdate,morerecent,ping
//    step2: ghostmirror -PoDumlsS  ~/.config/ghostmirror/mirrorlist ~/.config/ghostmirror/mirrorlist light state,outofdate,morerecent,estimated,speed
//    step3: forget about the mirrors.
//
//
//1m27s
//9.6s

__private unsigned COLORS[][6] = { 
	{   1, 196, 226, 190,  48,  46 }, // red to green
	{  46,  48, 190, 226, 196,   1 }, // green to red
	{   1, 196, 226, 190,  48,  46 }, // red to green
	{  23,  24,  25,  26,  27,  46 }, // for colorize value have only good result
	{  46, 226,   1,   1,   1,   1 }, // retry color, green yellow red...
	{   1, 196, 226, 190,  48,  46 }, // red to green
	{   1, 196, 226, 190,  48,  46 }, // red to green
	{  46,  48, 190, 226, 196,   1 }  // green to red
};
__private double CMAP[][6] = {
	{ 97.5, 98.0, 98.5,  99.0,  99.5, 100.0 },
	{  0.3,  0.6,  1.0,   1.2,   1.5,   2.0 },
	{ 93.0, 93.5, 94.0,  94.5,  95.0, 100.0 },
	{  0.5,  1.0,  2.0,   3.0,   4.0,   5.0 },
	{  1.0,  2.0,  3.0,   4.0,   5.0,   6.0 }, 
	{  1.0,  1.5,  2.0,   3.0,   6.5,  10.0 },
	{  2.0,  3.0,  3.0,   4.0,   9.0,  10.0 },
	{ 50.0,100.0,150.0, 200.0, 250.0, 500.0 }
};
__private unsigned CERR = 1;
__private unsigned CSTATUS[] = { 230, 118, 1 };
__private unsigned CBG = 238;


option_s OPT[] = {
	{'a', "--arch"           , "select arch, default 'x86_64'"                                      , OPT_STR  , 0, 0}, 
	{'m', "--mirrorfile"     , "use mirror file instead of downloading mirrorlist"                  , OPT_STR  , 0, 0},
	{'c', "--country"        , "select country from mirrorlist"                                     , OPT_ARRAY | OPT_STR  , 0, 0},
	{'C', "--country-list"   , "show all possibile country"                                         , OPT_NOARG, 0, 0},
	{'u', "--uncommented"    , "use only uncommented mirror"                                        , OPT_NOARG, 0, 0},
	{'d', "--downloads"      , "set numbers of parallel download"                                   , OPT_NUM  , 0, 0},
	{'O', "--timeout"        , "set timeout in seconds for not reply mirror"                        , OPT_NUM  , 0, 0},
	{'p', "--progress"       , "show progress, default false"                                       , OPT_NOARG, 0, 0},
	{'P', "--progress-colors", "same -p but with colors"                                            , OPT_NOARG, 0, 0},
	{'o', "--output"         , "enable table output"                                                , OPT_NOARG, 0, 0},
	{'s', "--speed"          , "test speed for downloading pkg, light/normal/heavy"                 , OPT_STR  , 0, 0},
	{'S', "--sort"           , "sort result for any of fields, mutiple fields supported"            , OPT_ARRAY | OPT_STR, 0, 0},
	{'l', "--list"           , "create a file with list of mirrors, stdout as arg for output here"  , OPT_STR, 0, 0},
	{'L', "--max-list"       , "set max numbers of output mirrors"                                  , OPT_NUM, 0, 0},
	{'T', "--type"           , "select mirrors type, http,https,all"                                , OPT_ARRAY | OPT_STR, 0, 0},
	{'i', "--investigate"    , "investigate on mirror, mode: outofdate/error/all"                   , OPT_ARRAY | OPT_STR, 0, 0},
	{'D', "--systemd"        , "auto manager systemd.timer"                                         , OPT_NOARG, 0, 0},
	{'t', "--time"           , "specific your preferite time in hh:mm:ss"                           , OPT_STR, 0, 0},
	{'f', "--fixed-time"     , "use fixed OnCalenda, systemd-analyzer calendar ..., for test valid" , OPT_STR, 0, 0},
	{'h', "--help"           , "display this"                                                       , OPT_END | OPT_NOARG, 0, 0}
};

__private void print_ifcompare_color(mirrorStatus_e status){
	if( status == MIRROR_COMPARE ){
		term_bg(CBG);
		term_bold();
	}
}

__private void print_table_header(char** colname, unsigned* colsize, unsigned count, int color){
	fputs("┌", stdout);
	for( unsigned i = 0; i < count - 1; ++i ){
		print_repeats(colsize[i], "─");
		fputs("┬", stdout);	
	}
	print_repeats(colsize[count-1], "─");
	fputs("┐\n", stdout);
	
	fputs("│", stdout);
	for( unsigned i = 0; i < count; ++i ){
		const unsigned len = strlen(colname[i]);
		const unsigned left  = colsize[i] > len ? (colsize[i] - len) / 2: 0;
		const unsigned right = colsize[i] > left+len ? colsize[i] - (left+len): 0;
		print_repeat(left, ' ');
		if( color >= 0 ){
			term_fg(208);
			term_bold();
		}
		printf("%s", colname[i]);
		if( color >= 0 ) term_fg(0);
		print_repeat(right, ' ');
		fputs("│", stdout);
	}

	fputs("\n├", stdout);
	for( unsigned i = 0; i < count - 1; ++i ){
		print_repeats(colsize[i], "─");
		fputs("┼", stdout);	
	}
	print_repeats(colsize[count-1], "─");
	fputs("┤\n", stdout);
}

__private void print_table_end(unsigned* colsize, unsigned count){
	fputs("└", stdout);
	for( unsigned i = 0; i < count - 1; ++i ){
		print_repeats(colsize[i], "─");
		fputs("┴", stdout);
	}
	print_repeats(colsize[count-1], "─");
	fputs("┘\n", stdout);
}

__private unsigned double_color_map(double v, unsigned palette){
	if( isnan(v) || isinf(v) ) return CERR;
	for( unsigned i = 0; i < sizeof_vector(CMAP[0]); ++i ){
		if( v < CMAP[palette][i]  ) return COLORS[palette][i];
	}
	return COLORS[palette][sizeof_vector(COLORS[0])-1];
}

__private void print_double_field(double val, mirrorStatus_e status, unsigned size, int colormode){
	const unsigned left  = (size - 7) / 2;
	const unsigned right = size - (left+7);

	if( colormode >= 0 ){
		colormode = double_color_map(val, colormode);
		term_fg(colormode);
		print_ifcompare_color(status);
	}
	print_repeat(left, ' ');
	if( isnan(val) || isinf(val) ){
		printf("  err  ");
	}
	else{
		printf("%6.2f%%",val);
	}
	print_repeat(right, ' ');
	if( colormode >= 0 ) term_fg(0);
	fputs("│", stdout);
}

/*
__private void print_unsigned_field(unsigned val, mirrorStatus_e status, unsigned size, int colormode){
	const unsigned left  = (size - 2) / 2;
	const unsigned right = size - (left+2);
	if( colormode >= 0 ){
		colormode = double_color_map(val, colormode);
		colorfg_set(colormode);
		print_ifcompare_color(status);
	}
	print_repeat(left, ' ');
	printf("%2u",val);
	print_repeat(right, ' ');
	if( colormode >= 0 ) colorfg_set(0);
	fputs("│", stdout);
}
*/
__private void print_status(mirrorStatus_e status, unsigned size, int color){
	static const char* mstate[] = {"success", "compare", "error"};
	if( color >= 0 ){
		term_fg(CSTATUS[status]);
		print_ifcompare_color(status);
	}
	printf("%-*s", size, mstate[status]);
	if( color >= 0 ) term_fg(0);
	fputs("│", stdout);
}

__private void print_str(const char* str, mirrorStatus_e status, unsigned size, int color){
	if( !str || *str == 0 ) str = "unknow";
	if( color >= 0 ){
		term_fg(CSTATUS[status]);
		print_ifcompare_color(status);
	}
	printf("%-*s", size, str);
	if( color >= 0 ) term_fg(0);
	fputs("│", stdout);
}

__private void print_speed(double val, mirrorStatus_e status, unsigned size, int colormode){
	const unsigned left  = (size - 10) / 2;
	const unsigned right = size - (left+10);
	if( colormode >= 0 ){
		colormode = double_color_map(val, colormode);
		term_fg(colormode);
		print_ifcompare_color(status);
	}
	print_repeat(left, ' ');
	printf("%5.1fMiB/s",val);
	print_repeat(right, ' ');
	if( colormode >= 0 ) term_fg(0);
	fputs("│", stdout);
}

__private void print_ping(long val, mirrorStatus_e status, unsigned size, int colormode){
	const unsigned len = val < 0 ? 5 : 8;
	const unsigned left  = (size - len) / 2;
	const unsigned right = size - (left+len);
	if( colormode >= 0 ){
		if( val < 0 ){
			term_fg(CERR);
		}
		else{
			colormode = double_color_map(val / 1000.0, colormode);
			term_fg(colormode);
		}
		print_ifcompare_color(status);
	}
	print_repeat(left, ' ');
	if( val < 0 ){
		fputs("error", stdout);
	}
	else{
		printf("%6.1fms",val / 1000.0);
	}
	print_repeat(right, ' ');
	if( colormode >= 0 ) term_fg(0);
	fputs("│", stdout);
}

__private void print_stability(unsigned val, mirrorStatus_e status, unsigned size, int colormode){
	const unsigned left  = (size - 4) / 2;
	const unsigned right = size - (left+4);
	if( colormode >= 0 ){
		colormode = double_color_map(val, colormode);
		term_fg(colormode);
		print_ifcompare_color(status);
	}
	print_repeat(left, ' ');
	printf("%2dgg", val);
	print_repeat(right, ' ');
	if( colormode >= 0 ) term_fg(0);
	fputs("│", stdout);
}

__private void print_bool(int value, mirrorStatus_e status, unsigned size, int color){
	unsigned len = value ? 4 : 5;
	const unsigned left  = (size - len) / 2;
	const unsigned right = size - (left+len);
	if( color >= 0 ){
		term_fg( value && color ? CERR : CSTATUS[0]);
		if( status == MIRROR_COMPARE ){
			term_bg(CBG);
			term_bold();
		}
	}
	print_repeat(left, ' ');
	printf("%s", value ? "true": "false");
	print_repeat(right, ' ');
	if( color >= 0 ) term_fg(0);
	fputs("│", stdout);
}

__private void print_cmp_mirrors(mirror_s* mirrors, int colors){
	unsigned mlUrl     = 6;
	unsigned mlCountry = 7;
	mforeach(mirrors, i){
		if( mirrors[i].url ){
			const size_t len = strlen(mirrors[i].url);
			if( len > mlUrl ) mlUrl = len;
		}
		if( mirrors[i].country ){
			const size_t len = strlen(mirrors[i].country);
			if( len > mlCountry ) mlCountry = len;
		}
	}

	colors = colors ? 0 : -1000;
	char* tblname[]    = { "country", "mirror",  "proxy",  "state", "outofdate", "uptodate", "morerecent",  "speed",    "ping", "estimated" };
	unsigned tblsize[] = { mlCountry,    mlUrl,        5,        9,           9,          9,           10,       12,         9,           9 };
	unsigned tblcolor[]= {  colors+0, colors+0, colors+1, colors+0,    colors+1,   colors+0,     colors+3, colors+5,  colors+7,    colors+6 };
	print_table_header(tblname, tblsize, sizeof_vector(tblsize), colors);
	
	mforeach(mirrors, i){
		fputs("│", stdout);
		print_str(mirrors[i].country, mirrors[i].status, tblsize[0], tblcolor[0]);
		print_str(mirrors[i].url, mirrors[i].status, tblsize[1], tblcolor[1]);
		print_bool(mirrors[i].isproxy, mirrors[i].status, tblsize[2], tblcolor[2]);
		print_status(mirrors[i].status, tblsize[3], tblcolor[3]);
		print_double_field(mirrors[i].outofdate  * 100.0 / mirrors[i].total, mirrors[i].status, tblsize[4], tblcolor[4]);
		print_double_field(mirrors[i].uptodate   * 100.0 / mirrors[i].total, mirrors[i].status, tblsize[5], tblcolor[5]);
		print_double_field(mirrors[i].morerecent * 100.0 / mirrors[i].total, mirrors[i].status, tblsize[6], tblcolor[6]);
		//print_unsigned_field(mirrors[i].retry, mirrors[i].status, tblsize[7], tblcolor[7]);
		print_speed(mirrors[i].speed, mirrors[i].status, tblsize[7], tblcolor[7]);
		print_ping(mirrors[i].ping, mirrors[i].status, tblsize[8], tblcolor[8]);
		print_stability(mirrors[i].estimated, mirrors[i].status, tblsize[9], tblcolor[9]);
		fputc('\n', stdout);
	}

	print_table_end(tblsize, sizeof_vector(tblsize));
}

__private void print_list(mirror_s* mirrors, const char* where, unsigned max){
	__free char* dwhere = NULL;
	__free char* tmp = NULL;
	
	FILE* out = stdout;
	if( strcmp(where, "stdout") ){
		dwhere = path_explode(where);
		tmp = str_printf("%s.gm.bak", dwhere);
		out = fopen(tmp, "w");
	}
	if( !out ) die("on open file: '%s' with error: %s", tmp, strerror(errno));
	
	time_t expired = time(NULL);
	struct tm* sexpired = gmtime(&expired);
	fprintf(out, "# lastsync<dd.mm.yyyy> %02u.%02u.%u\n", sexpired->tm_mday, sexpired->tm_mon+1, sexpired->tm_year + 1900);
	const char* lastCountry = "";
	const unsigned count = mem_header(mirrors)->len;
	for(unsigned i = 0; i < count && i < max; ++i ){
		if( strcmp(mirrors[i].country, lastCountry) ){
			lastCountry = mirrors[i].country;
			fprintf(out, "## %s\n", mirrors[i].country);
		}
		fprintf(out, "Server=%s/$repo/os/$arch\n", mirrors[i].url);
	}
	if( strcmp(where, "stdout") ){
		fclose(out);
		rename(tmp, dwhere);
	}
}

__private unsigned cast_mirror_type(unsigned type, const char* name){
	static const char* typename[] = { "all", "http", "https"};
	static const unsigned typeval[] = { 0xF, MIRROR_TYPE_HTTP, MIRROR_TYPE_HTTPS };
	for( unsigned i = 0; i < sizeof_vector(typename); ++i ){
		if( !strcmp(typename[i], name) ) return type | typeval[i];
	}
	die("unknow type name: %s", name);
}

__private unsigned cast_speed_type(const char* name){
	static const char* typename[] = { "light", "normal", "heavy" };
	for( unsigned i = 0; i < sizeof_vector(typename); ++i ){
		if( !strcmp(name, typename[i]) ) return i+1;
	}
	dbg_warning("no speed selected with option");
	return 0;
}

__private char* merge_sort(optValue_u* value, const unsigned count){
	char* out = MANY(char, 12*10);
	for( unsigned i = 0; i < count; ++i ){
		unsigned lenval = strlen(value[i].str);
		dbg_info("value: %s[%u]", value[i].str, lenval);
		out = mem_upsize(out, lenval + 1);
		unsigned len = mem_header(out)->len;
		memcpy(&out[len], value[i].str, lenval);
		out[len + lenval] = ',';
		mem_header(out)->len += lenval + 1;
	}
	out[--mem_header(out)->len] = 0;
	return out;
}


#include <notstd/delay.h>
int main(int argc, char** argv){
	notstd_begin();
	
	__argv option_s* opt = argv_parse(OPT, argc, argv);
	if( opt[O_h].set ) argv_usage(opt, argv[0]);
	
	argv_default_str(OPT, O_a, DEFAULT_ARCH);
	argv_default_num(OPT, O_d, cpu_cores());
	argv_default_num(OPT, O_O, DEFAULT_TOUT);
	argv_default_str(OPT, O_m, NULL);
	argv_default_str(OPT, O_S, NULL);
	argv_default_str(OPT, O_T, "all");
	argv_default_num(OPT, O_L, ULONG_MAX);
	argv_default_str(OPT, O_s, "");
	
	www_begin(opt[O_d].value->ui);
	gzip_init(opt[O_d].value->ui);
	__free char* mirrorlist     = mirror_loading(opt[O_m].value->str, opt[O_O].value->ui);
	__free char* safemirrorlist = opt[O_m].set ? mirror_loading(NULL, opt[O_O].value->ui) : str_dup(mirrorlist, 0);
	
	if( opt[O_P].set ) opt[O_p].set = 1;
	
	if( opt[O_D].set && systemd_restart_count() > SYSTEMD_SERVICE_RETRY_MAX ) systemd_timer_set(1, opt);
	
	if( opt[O_C].set ){
		country_list(mirrorlist);
		exit(0);
	}
	
	unsigned mirrorType = 0;
	mforeach(opt[O_T].value, i){
		mirrorType = cast_mirror_type(mirrorType, opt[O_T].value[i].str);
	}
	
	mirror_s* mirrors = MANY(mirror_s, 10);
	mirror_s local;
	dbg_info("load local database");
	database_local(&local, opt[O_a].value->str);
	if( mem_header(opt[O_c].value)->len < 1 ) {
		mirrors = mirrors_country(mirrors, mirrorlist, safemirrorlist, NULL, opt[O_a].value->str, opt[O_u].set, mirrorType);
	}
	else{
		mforeach(opt[O_c].value, i){
			mirrors = mirrors_country(mirrors, mirrorlist, safemirrorlist, opt[O_c].value[i].str, opt[O_a].value->str, opt[O_u].set, mirrorType);
		}
	}
	if( mem_header(mirrors)->len < 1 ) die("not find any valid mirrors");
	
	if( opt[O_p].set ){
		progress_enable(1+opt[O_P].set);
		term_status_line_begin();
	}
	mirrors_update(&local, mirrors, opt[O_d].value->ui, opt[O_O].value->ui, cast_speed_type(opt[O_s].value->str));
	if( opt[O_s].set ) mirrors_speed(mirrors, opt[O_a].value->str);
	term_status_line_end();
	
	mirrors_stability(mirrors);

	if( opt[O_o].set ){
		mirrors_sort_reset();
		if( opt[O_S].set ){
			for( unsigned i = 0; i < opt[O_S].set; ++i ){
				add_sort_mode(opt[O_S].value[i].str);
			}
			mirrors_sort(mirrors);
		}
		print_cmp_mirrors(mirrors, opt[O_P].set);
	}

	if( opt[O_l].set ){
		mirrors_sort_reset();
		add_sort_mode("country");
		mirrors_sort(mirrors);
		print_list(mirrors, opt[O_l].value->str, opt[O_L].value->ui);
	}

	// this is a manual test for check motivation than outofdate exists with morerecent
	/*
	mirror_s* local = NULL;
	const unsigned count = mem_header(mirrors)->len;
	for(unsigned i = 0; i < count; ++i ){
		if( mirrors[i].status == MIRROR_COMPARE  ){
			local = &mirrors[i];
			break;
		}
	}
	if( !local ) die("fail to find MIRROR_COMPARE");
	for(unsigned im = 0; im < count; ++im ){
		printf("%s\n", mirrors[im].url);
		if( mirrors[im].status != MIRROR_UNKNOW ) continue;
		if( mirrors[im].morerecent || mirrors[im].outofdate ){
			for( unsigned ir = 0; ir < 2; ++ir ){
				unsigned const dbcount = mem_header(local->repo[ir].db)->len;
				for( unsigned i = 0; i < dbcount; ++i){
					pkgdesc_s* tpk = mem_bsearch(mirrors[im].repo[ir].db, &local->repo[ir].db[i], pkgname_cmp);
					if( tpk ){
						int ret = pkg_vercmp(local->repo[ir].db[i].version, tpk->version);
						switch( ret ){
							case -1: printf("  # '%s' > '%s'\n", tpk->version, local->repo[ir].db[i].version); break;
							case  1: printf("  !'%s' < '%s'\n", tpk->version, local->repo[ir].db[i].version); break;
							case  0: break;
							default: die("WTF");
						}
					}
				}
			}
		}
	}
	*/
	
	if( opt[O_i].set ) investigate_mirrors(mirrors, &opt[O_i]);
	
	if( opt[O_D].set ){
		if( !opt[O_l].set ) die("for start daemon required output list, -l");
		if( !opt[O_s].set ) die("required speed test, --speed");
		if( !opt[O_S].set ) die("required any tipe of sort, --sort");
		__free char* where = path_explode(opt[O_l].value->str);
		__free char* sorta = merge_sort(opt[O_S].value, opt[O_S].set);
		systemd_timer_set(mirrors[0].estimated, opt);
	}
	
	gzip_end();
	www_end();
	return 0;
}


















