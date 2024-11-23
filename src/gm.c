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

//commit:

//TODO
//	--like  find all mirror like current mirror, workcase fallbackmirror
//		if mark status != UNKNOW with !like mirrro? in compare_db?
//	--list  create a list with Server=mirror
//	try:
//		--country <country> --speed --like [>=] --list <fout>
//		for generate a list of mirror?
//

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
	O_h
}OPT_E;

option_s OPT[] = {
	{'a', "--arch"        , "select arch, default 'x86_64'"                              , OPT_STR  , 0, 0}, 
	{'m', "--mirrorfile"  , "use mirror file instead of downloading mirrorlist"          , OPT_STR  , 0, 0},
	{'c', "--country"     , "select country from mirrorlist"                             , OPT_ARRAY | OPT_STR  , 0, 0},
	{'C', "--country-list", "show all possibile country"                                 , OPT_NOARG, 0, 0},
	{'u', "--uncommented" , "use only uncommented mirror"                                , OPT_NOARG, 0, 0},
	{'t', "--threads"     , "set numbers of parallel download, default '4'"              , OPT_NUM  , 0, 0},
	{'o', "--timeout"     , "set timeout in seconds for not reply mirror, default '20's" , OPT_NUM  , 0, 0},
	{'p', "--progress"    , "show progress, default false"                               , OPT_NOARG, 0, 0},
	{'s', "--speed"       , "test speed for downloading one pkg"                         , OPT_NOARG, 0, 0},
	{'h', "--help"        , "display this"                                               , OPT_END | OPT_NOARG, 0, 0}
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

	fputs("┌", stdout);
	print_repeats(maxlen, "─");
	fputs("┬", stdout);
	for( unsigned i = 0; i < 7; ++i ) fputs("──────────┬", stdout);
	fputs("──────────┐", stdout);
	putchar('\n');

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
	putchar('\n');

	fputs("├", stdout);
	print_repeats(maxlen, "─");
	fputs("┼", stdout);
	for( unsigned i = 0; i < 7; ++i ) fputs("──────────┼", stdout);
	fputs("──────────┤", stdout);
	putchar('\n');


	mforeach(mirrors, i){
		printf("│%-*s│", maxlen, mirrors[i].url);
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
		const double spe = mirrors[i].speed;

		printf(" %6.2f%%  │ %6.2f%%  │ %6.2f%%  │ %6.2f%%  │ %6.2f%%  │ %6.2f%%  │%5.1fmib/s│\n", ood, utd, ats, amp, aep, acp, spe);
		//printf("Server = %s/$repo/os/$arch\n", mirrors[i].url);
	}

	fputs("└", stdout);
	print_repeats(maxlen, "─");
	fputs("┴", stdout);
	for( unsigned i = 0; i < 7; ++i ) fputs("──────────┴", stdout);
	fputs("──────────┘", stdout);
	putchar('\n');

}

int main(int argc, char** argv){
	__argv option_s* opt = argv_parse(OPT, argc, argv);
	if( opt[O_h].set ) argv_usage(opt, argv[0]);

	argv_default_str(OPT, O_a, DEFAULT_ARCH);
	argv_default_num(OPT, O_t, DEFAULT_THREADS);
	argv_default_num(OPT, O_o, DEFAULT_TOUT);
	argv_default_str(OPT, O_m, NULL);

	www_begin();
/*
 * DBG print
mirror_s* mirrors = MANY(mirror_s, 16);
mem_header(mirrors)->len = 3;
mem_zero(mirrors);
mirrors[0].url = "https://packages.oth-regensburg.de/archlinux";
mirrors[0].totalpkg = 100;
mirrors[1].url = "https://packages.oth-regensburg.it/archlinux";
mirrors[2].url = "https://packages.oth-regensburg.it";

print_cmp_mirrors(mirrors);	
die("");
*/

	__free char* mirrorlist = mirror_loading(opt[O_m].value->str, opt[O_o].value->ui);

	if( opt[O_C].set ){
		country_list(mirrorlist);
		exit(0);
	}

	mirror_s* mirrors = NULL;
	if( opt[O_c].set ){
		mforeach(opt[O_c].value, i){
			mirrors = mirrors_country(mirrors, mirrorlist, opt[O_c].value[i].str, opt[O_a].value->str, opt[O_u].set);
		}
	}
	else{
		mirrors = mirrors_country(mirrors, mirrorlist, NULL, opt[O_a].value->str, opt[O_u].set);
	}
	
	mirrors_update(mirrors, opt[O_p].set, opt[O_t].value->ui, opt[O_o].value->ui);	
	mirrors_cmp_db(mirrors, opt[O_p].set);

	if( opt[O_s].set ) mirrors_speed(mirrors, opt[O_a].value->str, opt[O_p].set);	

	print_cmp_mirrors(mirrors);	
	
	www_end();
	return 0;
}





















