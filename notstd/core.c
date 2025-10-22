#include <notstd/type.h>
#include <notstd/compiler.h>
#include <notstd/mth.h>
#include <notstd/memory.h>

void notstd_begin(void){
	mth_random_begin();
	mem_begin();
}

void notstd_end(void){
	//deadpoll_end();
}
