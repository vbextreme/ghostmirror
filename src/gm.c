#include <notstd/core.h>
#include <notstd/str.h>
#include <notstd/opt.h>

#include <gm/www.h>
#include <gm/archive.h>
#include <gm/arch.h>

#include <unistd.h>
#include <fcntl.h>

#define DEFAULT_THREADS 4
#define DEFAULT_TOUT    20
#define DEFAULT_ARCH    "x86_64"

//TODO
//  sync=0 when morerecent>0, because ls is based on filename, if mirror is more recent the package have different version and can't find.
//
//  0.5.2 colorized output
//	0.6.0 add support to ftp
//  0.7.0 --speed slow/normal/fast
//  0.7.1 documentation
//  0.7.2 scanbuild
//  0.7.3 valgrind
//  0.7.4 makepkg
//  0.8.0 ?possible way to add tollerance to sorting data?
//  1.0.0 first release?

typedef enum{
	O_a,
	O_m,
	O_c,
	O_C,
	O_u,
	O_t,
	O_o,
	O_p,
	O_s,
	O_S,
	O_l,
	O_h
}OPT_E;

option_s OPT[] = {
	{'a', "--arch"        , "select arch, default 'x86_64'"                                      , OPT_STR  , 0, 0}, 
	{'m', "--mirrorfile"  , "use mirror file instead of downloading mirrorlist"                  , OPT_STR  , 0, 0},
	{'c', "--country"     , "select country from mirrorlist"                                     , OPT_ARRAY | OPT_STR  , 0, 0},
	{'C', "--country-list", "show all possibile country"                                         , OPT_NOARG, 0, 0},
	{'u', "--uncommented" , "use only uncommented mirror"                                        , OPT_NOARG, 0, 0},
	{'t', "--threads"     , "set numbers of parallel download, default '4'"                      , OPT_NUM  , 0, 0},
	{'o', "--timeout"     , "set timeout in seconds for not reply mirror, default '20's"         , OPT_NUM  , 0, 0},
	{'p', "--progress"    , "show progress, default false"                                       , OPT_NOARG, 0, 0},
	{'s', "--speed"       , "test speed for downloading one pkg"                                 , OPT_NOARG, 0, 0},
	{'S', "--sort"        , "sort result for any of fields, mutiple fields supported"             , OPT_ARRAY | OPT_STR, 0, 0},
	{'l', "--list"        , "create a file with list of mirrors, stdout as arg for output here"  , OPT_STR, 0, 0},
	{'h', "--help"        , "display this"                                                       , OPT_END | OPT_NOARG, 0, 0}
};

__private void print_repeat(unsigned count, char ch){
	while( count --> 0 ) putchar(ch);
}

__private void print_repeats(unsigned count, const char* ch){
	while( count --> 0 ) fputs(ch, stdout);
}

__private void print_cmp_mirrors(mirror_s* mirrors){
	unsigned maxlen = 0;
	mforeach(mirrors, i){
		size_t len = strlen(mirrors[i].url);
		if( len > maxlen ) maxlen = len;
	}

	//country
	fputs("┌", stdout);
	fputs("──────────┬", stdout);
	//url
	print_repeats(maxlen, "─");
	fputs("┬", stdout);
	//value
	for( unsigned i = 0; i < 8; ++i ) fputs("──────────┬", stdout);
	fputs("──────────┐", stdout);
	putchar('\n');
	
	fputs("│ country  ", stdout);
	fputs("│", stdout);
	unsigned part = maxlen / 2 - strlen("mirror") / 2;
	print_repeat(part, ' ');
	fputs("mirror", stdout);
	part +=6;
	if( part < maxlen ) part = maxlen-part;
	print_repeat(part, ' ');
	fputs("│", stdout);
	fputs("   state  │", stdout);
	fputs("outofdate │", stdout);
	fputs(" uptodate │", stdout);
	fputs("morerecent│", stdout);
	fputs("not exists│", stdout);
	fputs("newversion│", stdout);
	fputs("   sync   │", stdout);
	fputs("  speed   │", stdout);
	fputs("  retry   │", stdout);

	putchar('\n');

	fputs("├", stdout);
	print_repeats(10, "─");
	fputs("┼", stdout);
	print_repeats(maxlen, "─");
	fputs("┼", stdout);
	for( unsigned i = 0; i < 8; ++i ) fputs("──────────┼", stdout);
	fputs("──────────┤", stdout);
	putchar('\n');

	mforeach(mirrors, i){
		printf("│%-10.10s│", mirrors[i].country);
		printf("%-*s│", maxlen, mirrors[i].url);
		if( mirrors[i].status == MIRROR_ERR   ){
			fputs("   error  │", stdout);
		}
		else if( mirrors[i].status == MIRROR_LOCAL ){
			fputs("  current │", stdout);
		}
		else{
			fputs("  success │", stdout);
		}
		const double ood = mirrors[i].outofdatepkg * 100.0 / mirrors[i].totalpkg;
		const double utd = mirrors[i].uptodatepkg  * 100.0 / mirrors[i].totalpkg;
		const double ats = mirrors[i].syncdatepkg * 100.0 / mirrors[i].totalpkg;
		const double amp = mirrors[i].notexistspkg * 100.0 / mirrors[i].totalpkg;
		const double aep = mirrors[i].extrapkg * 100.0 / mirrors[i].totalpkg;
		const double acp = mirrors[i].checked * 100.0 / mirrors[i].totalpkg;

		printf(" %6.2f%%  │ %6.2f%%  │ %6.2f%%  │ %6.2f%%  │ %6.2f%%  │ %6.2f%%  │%5.1fmib/s│    %2d    │\n", ood, utd, ats, amp, aep, acp, mirrors[i].speed, mirrors[i].retry);
	}

	fputs("└", stdout);
	print_repeats(10, "─");
	fputs("┴", stdout);
	print_repeats(maxlen, "─");
	fputs("┴", stdout);
	for( unsigned i = 0; i < 8; ++i ) fputs("──────────┴", stdout);
	fputs("──────────┘", stdout);
	putchar('\n');

}

__private void print_list(mirror_s* mirrors, const char* where){
	FILE* out = strcmp(where, "stdout") ? fopen(where, "w") : stdout;
	if( !out ) die("on open file: '%s' with error: %s", where, strerror(errno));
	
	mforeach(mirrors, i){
		const double ood = mirrors[i].outofdatepkg * 100.0 / mirrors[i].totalpkg;
		const double utd = mirrors[i].uptodatepkg  * 100.0 / mirrors[i].totalpkg;
		const double ats = mirrors[i].syncdatepkg * 100.0 / mirrors[i].totalpkg;
		const double amp = mirrors[i].notexistspkg * 100.0 / mirrors[i].totalpkg;
		const double aep = mirrors[i].extrapkg * 100.0 / mirrors[i].totalpkg;
		const double acp = mirrors[i].checked * 100.0 / mirrors[i].totalpkg;
		fprintf(out, "#* '%s' %.2f%% %.2f%% %.2f%% %.2f%% %.2f%% %.2f%% %.1f %d\n", mirrors[i].country, ood, utd, ats, amp, aep, acp, mirrors[i].speed, mirrors[i].retry);
		fprintf(out, "Server=%s/$repo/os/$arch\n", mirrors[i].url);
	}

	if( strcmp(where, "stdout") ) fclose(out);
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

	www_begin();
/*
 * DBG print
mirror_s* mirrors = MANY(mirror_s, 16);
mem_header(mirrors)->len = 3;
mem_zero(mirrors);
mirrors[0].url = "https://packages.oth-regensburg.de/archlinux";
mirrors[0].outofdatepkg = 0;
mirrors[0].uptodatepkg = 10;
mirrors[0].syncdatepkg = 0;
mirrors[0].totalpkg    = 6;
mirrors[0].speed = 3.0;
mirrors[1].url = "https://packages.oth-regensburg.it/archlinux";
mirrors[1].outofdatepkg = 3;
mirrors[1].uptodatepkg = 5;
mirrors[1].syncdatepkg = 2;
mirrors[1].totalpkg    = 10;
mirrors[1].speed = 7.0;
mirrors[2].url = "https://packages.oth-regensburg.it";
mirrors[2].outofdatepkg = 0;
mirrors[2].uptodatepkg = 7;
mirrors[2].syncdatepkg = 3;
mirrors[2].speed = 7.0;
mirrors[2].totalpkg = 10;
print_cmp_mirrors(mirrors);	
die("");
*/

	__free char* mirrorlist     = mirror_loading(opt[O_m].value->str, opt[O_o].value->ui);
	__free char* safemirrorlist = opt[O_m].set ? mirror_loading(NULL, opt[O_o].value->ui) : str_dup(mirrorlist, 0);

	if( opt[O_C].set ){
		country_list(mirrorlist);
		exit(0);
	}

	mirror_s* mirrors = NULL;
	if( opt[O_c].set ){
		mforeach(opt[O_c].value, i){
			mirrors = mirrors_country(mirrors, mirrorlist, safemirrorlist, opt[O_c].value[i].str, opt[O_a].value->str, opt[O_u].set);
		}
	}
	else{
		mirrors = mirrors_country(mirrors, mirrorlist, safemirrorlist, NULL, opt[O_a].value->str, opt[O_u].set);
	}
	
	mirrors_update(mirrors, opt[O_p].set, opt[O_t].value->ui, opt[O_o].value->ui);	
	mirrors_cmp_db(mirrors, opt[O_p].set);

	if( opt[O_s].set ) mirrors_speed(mirrors, opt[O_a].value->str, opt[O_p].set);	

	if( opt[O_S].set ){
		for( unsigned i = 0; i < opt[O_S].set; ++i ){
			add_sort_mode(opt[O_S].value[i].str);
		}
		mirrors_sort(mirrors);
	}

	print_cmp_mirrors(mirrors);	
	
	if( opt[O_l].set ) print_list(mirrors, opt[O_l].value->str);

	www_end();
	return 0;
}





















