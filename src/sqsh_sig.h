#ifndef sqsh_sig_h_included
#define sqsh_sig_h_included
#include <signal.h>

#if !defined(SIGCHLD) && defined(SIGCLD)
#   define SIGCHLD   SIGCLD
#endif

/*
 * Handy type definition for a sqsh signal handler. 
 */
typedef void (sig_handle_t) _ANSI_ARGS(( int, void* ));

/*
 * The following flags may be used when installing a signal handler
 * to configure its behavior.
 */
#define SIG_F_CHAIN     (1<<0)    /* Chain this call to head of list */

/*-- Special signal handlers --*/
#define SIG_H_ERR       ((sig_handle_t*)-1)
#define SIG_H_DFL       ((sig_handle_t*)1)
#define SIG_H_IGN       ((sig_handle_t*)2)
#define SIG_H_POLL      ((sig_handle_t*)3)

/*-- Prototypes --*/
int  sig_install _ANSI_ARGS(( int, sig_handle_t*, void*, int ));
int  sig_save    _ANSI_ARGS(( void ));
int  sig_restore _ANSI_ARGS(( void ));
int  sig_poll    _ANSI_ARGS(( int ));
int  sig_clear   _ANSI_ARGS(( int ));

#endif /* sqsh_sig_h_included */
