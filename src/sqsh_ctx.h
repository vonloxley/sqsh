#ifndef sqsh_ctx_h_included
#define sqsh_ctx_h_included
#include <ctpublic.h>
#include "sqsh_varbuf.h"
#include "dsp.h"

/*
** Prototypes for manipulating the context.
*/
int sqsh_ctx_push        _ANSI_ARGS(( void ));
int sqsh_ctx_set_args    _ANSI_ARGS(( int, char** ));
int sqsh_ctx_get_args    _ANSI_ARGS(( int*, char*** ));
int sqsh_ctx_set_conn    _ANSI_ARGS(( CS_CONNECTION* ));
int sqsh_ctx_get_conn    _ANSI_ARGS(( CS_CONNECTION** ));
int sqsh_ctx_set_cols    _ANSI_ARGS(( dsp_desc_t* ));
int sqsh_ctx_get_cols    _ANSI_ARGS(( dsp_desc_t** ));
int sqsh_ctx_set_sqlbuf  _ANSI_ARGS(( varbuf_t* ));
int sqsh_ctx_get_sqlbuf  _ANSI_ARGS(( varbuf_t** ));
int sqsh_ctx_pop         _ANSI_ARGS(( void ));

#endif /* sqsh_ctx_h_included */
