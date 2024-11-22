#ifndef __NOTSTD_LIST_H__
#define __NOTSTD_LIST_H__

#include <notstd/core.h>

/**************************/
/*** simple linked list ***/
/**************************/

#define inherit_ls(T) struct { T* next; }

//iterate to list
//list_s* head = NULL;
//ls_push(....);
//foreach_ls(head, it){
//	printf("%p\n", it);
//}
#define lsforeach(L, IT) for(typeof(L) IT = (L); IT; (IT) = (IT->next))

#define ls_ctor(L) ({\
	typeof(L) _l_ = (L);\
	_l_->next = NULL;\
   	_l_;\
})

#define ls_push(H, N) ({\
	(N)->next = (H);\
	(H) = (N);\
})

#define ls_pop(H) ({\
	typeof(H) __ret__ = (H);\
	(H)=(H)->next;\
	__ret__->next = NULL;\
	__ret__;\
})

#define ls_extract(H, N) ({\
	typeof(H *) _pn_ = &(H);\
	typeof(N)   _eq_ = (N);\
	for(; *_pn_ && *_pn_ != _eq_ ; _pn_ = &((*_pn_)->next) );\
	if( *_pn_ ) *_pn_ = (*_pn_)->next;\
	_eq_->next = NULL;\
	_eq_;\
})


/***************************************/
/*** double linked list concatenated ***/
/***************************************/

#define inherit_ld(T) struct { T* next; T* prev; }

#define ldforeach(L, IT) for( typeof(L) IT = (L); IT; IT = IT->next == (L) ? NULL: IT->next )

#define ld_ctor(L) ({\
	typeof(L) _l_ = (L);\
	_l_->next = _l_;\
	_l_->prev = _l_;\
   	_l_;\
})

#define ld_after(L, N) ({\
	typeof(L) _l_ = (L);\
	typeof(N) _n_ = (N);\
	_l_->next->prev = _n_->prev;\
	_n_->prev->next = _l_->next;\
	_n_->prev = _l_;\
	_l_->next = _n_;\
	_n_;\
})

#define ld_before(L, N) ({\
	typeof(L) _l_ = (L);\
	typeof(N) _n_ = (N);\
	typeof(L) _t_ = _n_->prev;\
	_t_->next = _l_;\
	_l_->prev->next = _n_;\
	_n_->prev = _l_->prev;\
	_l_->prev = _t_;\
	_n_;\
})

#define ld_extract(N) ({\
	typeof(N) _n_ = (N);\
	_n_->next->prev = _n_->prev;\
	_n_->prev->next = _n_->next;\
	_n_->next = _n_->prev = _n_;\
	_n_;\
})


#endif
