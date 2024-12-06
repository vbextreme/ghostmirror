#include <notstd/core.h>
#include <notstd/str.h>
#include <notstd/opt.h>

#include <gm/www.h>
#include <gm/archive.h>
#include <gm/arch.h>

#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#define DEFAULT_THREADS 4
#define DEFAULT_TOUT    20
#define DEFAULT_ARCH    "x86_64"

//TODO
//  sync=0 when morerecent>0, because ls is based on filename, if mirror is more recent the package have different version and can't find.
//  many mirror are proxy and move you request in other mirror, some time append than link to url is broken in main mirror (generally motivation for 404)
//  it's possible to get follow location for check if wrong location?
//
//TODO error if not able to get local mirror
//
//	0.8.1 is proxy and add url to table
//  0.8.2 add num error when error is set in www, better add error option and print table for possible issues with better error descript
//  0.9.0 systemd auto mirroring
//  0.9.1 documentation
//  0.9.2 scanbuild
//  0.9.3 valgrind
//  1.0.0 first release?

__private unsigned COLORS[][6] = { 
	{   1, 196, 226, 190,  48,  46 }, // red to green
	{  46,  48, 190, 226, 196,   1 }, // green to red
	{   1, 196, 226, 190,  48,  46 }, // red to green
	{  23,  24,  25,  26,  27,  46 }, // for colorize value have only good result
	{  46, 226,   1,   1,   1,   1 }, // retry color, green yellow red...
	{   1, 196, 226, 190,  48,  46 }, // red to green
	{   1, 226,  48,  46,  46,  46 }, // green to red								  

};
__private double CMAP[][6] = {
	{ 97.5, 98.0, 98.5,  99.0,  99.5, 100.0 },
	{  0.3,  0.6,  1.0,   1.2,   1.5,   2.0 },
	{ 93.0, 93.5, 94.0,  94.5,  95.0, 100.0 },
	{  0.5,  1.0,  2.0,   3.0,   4.0,   5.0 },
	{  1.0,  2.0,  3.0,   4.0,   5.0,   6.0 }, 
	{  1.0,  1.5,  2.0,   3.0,   6.5,  10.0 },
	{ 98.0, 99.0, 99.5, 100.0, 100.0, 100.0 }
};
__private unsigned CERR = 1;
__private unsigned CSTATUS[] = { 230, 118, 1 };
__private unsigned CBG = 238;

__private double STABILITYMAP[] = { 98.0, 99.0, 99.5, 100.0 };
__private unsigned DAYMAP[]     = {    1,    2,    5,    10 };

typedef enum{
	O_a,
	O_m,
	O_c,
	O_C,
	O_u,
	O_t,
	O_o,
	O_p,
	O_P,
	O_s,
	O_S,
	O_l,
	O_L,
	O_T,
	O_h
}OPT_E;

option_s OPT[] = {
	{'a', "--arch"           , "select arch, default 'x86_64'"                                      , OPT_STR  , 0, 0}, 
	{'m', "--mirrorfile"     , "use mirror file instead of downloading mirrorlist"                  , OPT_STR  , 0, 0},
	{'c', "--country"        , "select country from mirrorlist"                                     , OPT_ARRAY | OPT_STR  , 0, 0},
	{'C', "--country-list"   , "show all possibile country"                                         , OPT_NOARG, 0, 0},
	{'u', "--uncommented"    , "use only uncommented mirror"                                        , OPT_NOARG, 0, 0},
	{'t', "--threads"        , "set numbers of parallel download, default '4'"                      , OPT_NUM  , 0, 0},
	{'o', "--timeout"        , "set timeout in seconds for not reply mirror, default '20's"         , OPT_NUM  , 0, 0},
	{'p', "--progress"       , "show progress, default false"                                       , OPT_NOARG, 0, 0},
	{'P', "--progress-colors", "same -p but with colors"                                            , OPT_NOARG, 0, 0},
	{'s', "--speed"          , "test speed for downloading one pkg, light, normal, heavy"           , OPT_STR  , 0, 0},
	{'S', "--sort"           , "sort result for any of fields, mutiple fields supported"            , OPT_ARRAY | OPT_STR, 0, 0},
	{'l', "--list"           , "create a file with list of mirrors, stdout as arg for output here"  , OPT_STR, 0, 0},
	{'L', "--max-list"       , "set max numbers of output mirrors"                                  , OPT_NUM, 0, 0},
	{'T', "--type"           , "select mirrors type, http,https,all"                                , OPT_ARRAY | OPT_STR, 0, 0},
	{'h', "--help"           , "display this"                                                       , OPT_END | OPT_NOARG, 0, 0}
};

__private void colorfg_set(unsigned color){
	if( !color ){
		fputs("\033[0m", stdout);
	}
	else{
		printf("\033[38;5;%um", color);
	}
}

__private void colorbg_set(unsigned color){
	if( !color ){
		fputs("\033[0m", stdout);
	}
	else{
		printf("\033[48;5;%um", color);
	}
}

__private void bold_set(void){
	fputs("\033[1m", stdout);
}

__private void print_repeats(unsigned count, const char* ch){
	if( !count ) return;
	while( count --> 0 ) fputs(ch, stdout);
}

__private void print_repeat(unsigned count, const char ch){
	if( !count ) return;
	while( count --> 0 ) fputc(ch, stdout);
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
			colorfg_set(208);
			bold_set();
		}
		printf("%s", colname[i]);
		if( color >= 0 ) colorfg_set(0);
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
		colorfg_set(colormode);
		if( status == MIRROR_LOCAL ){
			colorbg_set(CBG);
			bold_set();
		}
	}

	print_repeat(left, ' ');
	printf("%6.2f%%",val);
	print_repeat(right, ' ');
	if( colormode >= 0 ) colorfg_set(0);
	fputs("│", stdout);
}

__private void print_unsigned_field(unsigned val, mirrorStatus_e status, unsigned size, int colormode){
	const unsigned left  = (size - 2) / 2;
	const unsigned right = size - (left+2);
	if( colormode >= 0 ){
		colormode = double_color_map(val, colormode);
		colorfg_set(colormode);
		if( status == MIRROR_LOCAL ){
			colorbg_set(CBG);
			bold_set();
		}
	}
	print_repeat(left, ' ');
	printf("%2u",val);
	print_repeat(right, ' ');
	if( colormode >= 0 ) colorfg_set(0);
	fputs("│", stdout);
}

__private void print_status(mirrorStatus_e status, unsigned size, int color){
	static const char* mstate[] = {"success", "local", "error"};
	if( color >= 0 ){
		colorfg_set(CSTATUS[status]);
		if( status == MIRROR_LOCAL ){
			colorbg_set(CBG);
			bold_set();
		}
	}
	printf("%-*s", size, mstate[status]);
	if( color >= 0 ) colorfg_set(0);
	fputs("│", stdout);
}

__private void print_str(const char* str, mirrorStatus_e status, unsigned size, int color){
	if( !str || *str == 0 ) str = "unknow";
	if( color >= 0 ){
		colorfg_set(CSTATUS[status]);
		if( status == MIRROR_LOCAL ){
			colorbg_set(CBG);
			bold_set();
		}
	}
	printf("%-*s", size, str);
	if( color >= 0 ) colorfg_set(0);
	fputs("│", stdout);
}

__private void print_speed(double val, mirrorStatus_e status, unsigned size, int colormode){
	const unsigned left  = (size - 10) / 2;
	const unsigned right = size - (left+10);
	if( colormode >= 0 ){
		colormode = double_color_map(val, colormode);
		colorfg_set(colormode);
		if( status == MIRROR_LOCAL ){
			colorbg_set(CBG);
			bold_set();
		}
	}
	print_repeat(left, ' ');
	printf("%5.1fMiB/s",val);
	print_repeat(right, ' ');
	if( colormode >= 0 ) colorfg_set(0);
	fputs("│", stdout);
}

__private unsigned stability_to_day(double val){
	for( unsigned i = 0; i < sizeof_vector(STABILITYMAP); ++i ){
		if( val < STABILITYMAP[i] ) return DAYMAP[i];
	}
	return DAYMAP[sizeof_vector(STABILITYMAP)-1];
}

__private void print_stability(double val, mirrorStatus_e status, unsigned size, int colormode){
	const unsigned left  = (size - 4) / 2;
	const unsigned right = size - (left+4);
	if( colormode >= 0 ){
		colormode = double_color_map(val, colormode);
		colorfg_set(colormode);
		if( status == MIRROR_LOCAL ){
			colorbg_set(CBG);
			bold_set();
		}
	}
	print_repeat(left, ' ');
	printf("%2dgg", stability_to_day(val));
	print_repeat(right, ' ');
	if( colormode >= 0 ) colorfg_set(0);
	fputs("│", stdout);
}

__private void print_bool(int value, mirrorStatus_e status, unsigned size, int color){
	unsigned len = value ? 4 : 5;
	const unsigned left  = (size - len) / 2;
	const unsigned right = size - (left+len);
	if( color >= 0 ){
		colorfg_set( value && color ? CERR : CSTATUS[0]);
		if( status == MIRROR_LOCAL ){
			colorbg_set(CBG);
			bold_set();
		}
	}
	print_repeat(left, ' ');
	printf("%s", value ? "true": "false");
	print_repeat(right, ' ');
	if( color >= 0 ) colorfg_set(0);
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
	char* tblname[]    = { "country", "mirror",  "proxy",  "state", "outofdate", "uptodate", "morerecent",   "sync",  "retry", "speed", "extimated" };
	unsigned tblsize[] = { mlCountry,    mlUrl,        5,        9,           9,          9,           10,        9,        7,      12,          9  };
	unsigned tblcolor[]= {  colors+0, colors+0, colors+1, colors+0,    colors+1,   colors+0,     colors+3, colors+2, colors+4, colors+5,   colors+6 };
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
		print_double_field(mirrors[i].sync       * 100.0 / mirrors[i].total, mirrors[i].status, tblsize[7], tblcolor[7]);
		print_unsigned_field(mirrors[i].retry, mirrors[i].status, tblsize[8], tblcolor[8]);
		print_speed(mirrors[i].speed, mirrors[i].status, tblsize[9], tblcolor[9]);
		print_stability(mirrors[i].stability, mirrors[i].status, tblsize[10], tblcolor[10]);
		fputc('\n', stdout);
	}

	print_table_end(tblsize, sizeof_vector(tblsize));
}

__private void print_list(mirror_s* mirrors, const char* where, unsigned max){
	FILE* out = strcmp(where, "stdout") ? fopen(where, "w") : stdout;
	if( !out ) die("on open file: '%s' with error: %s", where, strerror(errno));
	
	const unsigned count = mem_header(mirrors)->len;
	for(unsigned i = 0; i < count && i < max; ++i ){
		fprintf(out, "#* '%s'", mirrors[i].country);
		fprintf(out, " %f\n", mirrors[i].speed);
		fprintf(out, "Server=%s/$repo/os/$arch\n", mirrors[i].url);
	}
	if( strcmp(where, "stdout") ) fclose(out);
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
		if( !strcmp(name, typename[i]) ) return i;
	}
	die("unknow type name: %s", name);
}

int main(int argc, char** argv){
	notstd_begin();
	
	__argv option_s* opt = argv_parse(OPT, argc, argv);
	if( opt[O_h].set ) argv_usage(opt, argv[0]);

	argv_default_str(OPT, O_a, DEFAULT_ARCH);
	argv_default_num(OPT, O_t, DEFAULT_THREADS);
	argv_default_num(OPT, O_o, DEFAULT_TOUT);
	argv_default_str(OPT, O_m, NULL);
	argv_default_str(OPT, O_S, NULL);
	argv_default_str(OPT, O_T, "all");
	argv_default_num(OPT, O_L, ULONG_MAX);

	www_begin();

/*
mirror_s* mirrors = MANY(mirror_s, 16);
mem_header(mirrors)->len = 4;
mem_zero(mirrors);
mirrors[0].url = "https://uptodate";
mirrors[0].uptodate   = 100;
mirrors[0].total      = 100;
mirrors[0].proxy      = "https://improxy";
mirrors[0].speed = 3;
mirrors[0].status = MIRROR_LOCAL;
mirrors[1].url = "https://morerecent";
mirrors[1].uptodate   = 98;
mirrors[1].morerecent = 2;
mirrors[1].total      = 100;
mirrors[1].speed = 2.7;
mirrors[2].url = "https://outofdate";
mirrors[2].outofdate  = 2;
mirrors[2].uptodate   = 98;
mirrors[2].total      = 100;
mirrors[2].speed = 3.3;
mirrors[3].url = "https://sync";
mirrors[3].sync       = 94;
mirrors[3].uptodate   = 100;
mirrors[3].total      = 100;
mirrors[3].retry      = 3;
mirrors[3].speed = 7.0;
mirrors[3].status = MIRROR_ERR;

mirrors_stability(mirrors);
add_sort_mode("extimated");
mirrors_sort(mirrors);
print_cmp_mirrors(mirrors,1);	
die("");
*/	
	__free char* mirrorlist     = mirror_loading(opt[O_m].value->str, opt[O_o].value->ui);
	__free char* safemirrorlist = opt[O_m].set ? mirror_loading(NULL, opt[O_o].value->ui) : str_dup(mirrorlist, 0);
	
	if( opt[O_P].set ) opt[O_p].set = 1;

	if( opt[O_C].set ){
		country_list(mirrorlist);
		exit(0);
	}

	unsigned mirrorType = 0;
	if( opt[O_T].set ){
		for( unsigned i = 0; i < opt[O_T].set; ++i ){
			mirrorType = cast_mirror_type(mirrorType, opt[O_T].value[i].str);
		}
	}
	else{
		mirrorType = cast_mirror_type(mirrorType, opt[O_T].value->str);
	}

	mirror_s* mirrors = NULL;
	if( opt[O_c].set ){
		mforeach(opt[O_c].value, i){
			mirrors = mirrors_country(mirrors, mirrorlist, safemirrorlist, opt[O_c].value[i].str, opt[O_a].value->str, opt[O_u].set, mirrorType);
		}
	}
	else{
		mirrors = mirrors_country(mirrors, mirrorlist, safemirrorlist, NULL, opt[O_a].value->str, opt[O_u].set, mirrorType);
	}
	
	mirrors_update(mirrors, opt[O_p].set, opt[O_t].value->ui, opt[O_o].value->ui);	
	mirrors_cmp_db(mirrors, opt[O_p].set);
	if( opt[O_s].set ) mirrors_speed(mirrors, opt[O_a].value->str, opt[O_p].set, cast_speed_type(opt[O_s].value->str));
	mirrors_stability(mirrors);

	if( opt[O_S].set ){
		for( unsigned i = 0; i < opt[O_S].set; ++i ){
			add_sort_mode(opt[O_S].value[i].str);
		}
		mirrors_sort(mirrors);
	}
	
	print_cmp_mirrors(mirrors, opt[O_P].set);
	if( opt[O_l].set ) print_list(mirrors, opt[O_l].value->str, opt[O_L].value->ui);

	www_end();
	return 0;
}





















