#include <notstd/core.h>
#include <notstd/str.h>
#include <notstd/opt.h>

#include <gm/www.h>
#include <gm/archive.h>
#include <gm/arch.h>

#include <unistd.h>
#include <fcntl.h>

#define DEFAULT_THREADS 16
#define DEFAULT_TOUT    5
#define DEFAULT_ARCH    "x86_64"

//TODO
//	not like info
//	--speed find download speed, workcase first mirror
//	--like  find all mirror like current mirror, workcase fallbackmirror
//		if mark status != UNKNOW with !like mirrro? in compare_db?
//	--list  create a list with Server=mirror
//	--showcountry display all country
//	try:
//		--country <country> --speed --like [>=] --list <fout>
//		for generate a list of mirror?
//

typedef enum{
	O_a,
	O_m,
	O_c,
	O_u,
	O_d,
	O_s,
	O_i,
	O_t,
	O_o,
	O_p,
	O_h
}OPT_E;

option_s OPT[] = {
	{'a', "--arch"        , "select arch, default 'x86_64'"                              , OPT_STR  , 0, 0}, 
	{'m', "--mirrorfile"  , "use mirror file instead of downloading mirrorlist"          , OPT_STR  , 0, 0},
	{'c', "--country"     , "select country from mirrorlist"                             , OPT_ARRAY | OPT_STR  , 0, 0},
	{'u', "--uncommented" , "use only uncommented mirror"                                , OPT_NOARG, 0, 0},
	{'d', "--comparedb"   , "compare database for find outofdate mirror"                 , OPT_NOARG, 0, 0},
	{'s', "--sync"        , "check if '*' mirror is sync or '>' or '='"                  , OPT_STR  , 0, 0},
	{'i', "--info"        , "print result"                                               , OPT_NOARG, 0, 0},
	{'t', "--threads"     , "set numbers of parallel download, default '16'"             , OPT_NUM  , 0, 0},
	{'o', "--timeout"     , "set timeout in seconds for not reply mirror, default '5's"  , OPT_NUM  , 0, 0},
	{'p', "--progress"    , "show progress, default false"                               , OPT_NOARG, 0, 0},
	{'h', "--help"        , "display this"                                               , OPT_END | OPT_NOARG, 0, 0}
};

__private void print_cmp_mirrors(mirror_s* mirrors){
	puts(" < ammount of outofdate, mirror db are version minor local db");
	puts(" = ammount of uptodate, mirror db are equal version with local db");
	puts(" > ammount to sync, mirror db are version major of local db");
	puts(" ? ammount missing package, mirror db not have this pkg");
	puts(" ! ammount extras package, mirror db have this pkg but local db not have");
	puts(" @ ammount checked package, mirror successfull checked package");
	puts("");

	mforeach(mirrors, i){
		if( mirrors[i].status == MIRROR_LOCAL ) continue;
		if( mirrors[i].status == MIRROR_ERR   ){
			//printf("<<eeeeeee<< ==eeeeeee== >>eeeeeee>> ??eeeeeee?? !!eeeeeee!! @eeeeeee@ %s\n", mirrors[i].url);	
			continue;
		}
		const double ood = mirrors[i].outofdatepkg * 100.0 / mirrors[i].totalpkg;
		const double utd = mirrors[i].uptodatepkg  * 100.0 / mirrors[i].totalpkg;
		const double ats = mirrors[i].syncdatepkg * 100.0 / mirrors[i].totalpkg;
		const double amp = mirrors[i].notexistspkg * 100.0 / mirrors[i].totalpkg;
		const double aep = mirrors[i].extrapkg * 100.0 / mirrors[i].totalpkg;
		const double acp = mirrors[i].checked * 100.0 / mirrors[i].totalpkg;
		
		printf("<<%6.2f%%<< ==%6.2f%%== >>%6.2f%%>> ??%6.2f%%?? !!%6.2f%%!! @%6.2f&&@ %s", ood, utd, ats, amp, aep, acp, mirrors[i].url);
		puts("");
		//printf("Server = %s/$repo/os/$arch\n", mirrors[i].url);
	}
}

int main(int argc, char** argv){
	__argv option_s* opt = argv_parse(OPT, argc, argv);
	if( opt[O_h].set ) argv_usage(opt, argv[0]);

	argv_default_str(OPT, O_a, DEFAULT_ARCH);
	argv_default_num(OPT, O_t, DEFAULT_THREADS);
	argv_default_num(OPT, O_o, DEFAULT_TOUT);
	argv_default_str(OPT, O_m, NULL);

	www_begin();
	
	__free char* mirrorlist = mirror_loading(opt[O_m].value->str, opt[O_o].value->ui);
	
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
	
	if( opt[O_d].set ) mirrors_cmp_db(mirrors, opt[O_p].set);
	
	if( opt[O_s].set ){
		puts("its a big todo, making many requests to mirrors doesn't seem to be a good idea, you get bad requests or 404");
		puts("need wait a big idea. sorry");
		//mirrors_update_sync(mirrors, *opt[O_s].value->str, opt[O_t].value->ui, opt[O_o].value->ui, opt[O_p].set);
	}
	
	if( opt[O_i].set ) print_cmp_mirrors(mirrors);	
	
	www_end();
	return 0;
}





















