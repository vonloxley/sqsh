/*
 * sqsh_sig.c - Generic signal handling.
 *
 * Copyright (C) 1995, 1996 by Scott C. Gray
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, write to the Free Software
 * Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * You may contact the author :
 *   e-mail:  gray@voicenet.com
 *            grays@xtend-tech.com
 *            gray@xenotropic.com
 */
#include <stdio.h>
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_sig.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_sig.c,v 1.3 2001/11/08 21:57:08 gray Exp $";
USE(RCS_Id)
#endif

#if defined(NSIG)
#define SQSH_NSIGS NSIG+1
#else
#define SQSH_NSIGS 33
#endif

#if defined(DEBUG)
#  if defined(HAVE_SIGACTION)
#     define  SIG_TYPE  "POSIX"
#  else
#     define  SIG_TYPE  "BSD"
#  endif
#endif

/*
 * hndl_t: Defines an instance of a signal handler.  These structures
 *        are maintained in an array indexed by signal number (see
 *        sg_sigs[], below). Within the array, each entry contains
 *        a linked list of these according to how signals are chained
 *        together.
 */
typedef struct _hndl_t {
	int             h_signo;       /* Signal number */
	int             h_depth;       /* Depth in save/restore stack */
	int             h_flags;       /* Flags installed for handler */
	int             h_count;       /* Total signal received */
	void           *h_data;        /* User data */
	sig_handle_t   *h_handler;     /* Actual signal handler */
	struct _hndl_t *h_prv;         /* Previous handler in stack */
} hndl_t;

/*-- Local Globals --*/
static struct {
	RETSIGTYPE    (*s_installed)(); /* Which OS handler is installed */
	hndl_t        *s_hndl;          /* User handlers to call */
} sg_sigs[SQSH_NSIGS];

static int    sg_depth = 0;           /* Depth within save/restore stack */

/*-- Prototypes --*/
static RETSIGTYPE sig_handler();

/*
 *  sig_install():
 *  
 *  Installs user defined signal handler to be called when signo
 *  signal is raised by the OS. 
 */
int sig_install( signo, handler, user_data, flags )
	int            signo;
	sig_handle_t  *handler;
	void          *user_data;
	int            flags;
{
	hndl_t        *h;
	RETSIGTYPE   (*sig_func)();

#if defined(HAVE_SIGACTION)
	struct sigaction sa;
#endif

	/*
	 *  If we are not chaining this signal handler to the previous
	 *  one, and we are at the same save/restore depth as the
	 *  previous one, then we can simply replace the existing one
	 *  rather than chaining to it.
	 */
	if ((flags & SIG_F_CHAIN)            == 0    &&   /* Not chained */
	     sg_sigs[signo].s_hndl          != NULL  &&   /* There is a handler */
	     sg_sigs[signo].s_hndl->h_depth == sg_depth)  /* At current depth */	
	{
		DBG(sqsh_debug(DEBUG_SIG, 
			"sig_install: %s signal %d: Replacing user handler %p with %p\n",
			SIG_TYPE, signo, sg_sigs[signo].s_hndl->h_handler, handler);)
		h = sg_sigs[signo].s_hndl;
	}
	else
	{
		DBG(sqsh_debug(DEBUG_SIG, 
			"sig_install: %s signal %d: Installing user handler %p\n",
			SIG_TYPE, signo, handler);)

		/*
		 * Allocate a signal handler structure to repesent our signal handler
		 * and what it is trying to do.
		 */
		h = (hndl_t*)malloc( sizeof(hndl_t) );
		if (h == NULL)
		{
			sqsh_set_error( SQSH_E_NOMEM, NULL );
			return False;
		}

		h->h_prv               = sg_sigs[signo].s_hndl;
		sg_sigs[signo].s_hndl = h;
	}

	h->h_signo     = signo;
	h->h_depth     = sg_depth;
	h->h_flags     = flags;
	h->h_count     = 0;
	h->h_data      = user_data;
	h->h_handler   = handler;

	if (handler == SIG_H_IGN)
	{
		sig_func = SIG_IGN;
	}
	else if (handler == SIG_H_DFL)
	{
		sig_func = SIG_DFL;
	}
	else
	{
		sig_func = sig_handler;
	}

	/*
	 * Compare the OS signal handler that we want to install vs. the
	 * one that is actually installed.  If they are different then
	 * do the installation.
	 */
	if (sg_sigs[signo].s_installed != sig_func)
	{
		sg_sigs[signo].s_installed = sig_func;

		DBG(sqsh_debug(DEBUG_SIG, 
			"sig_install: %s signal %d: Installing OS handler %p\n",
			SIG_TYPE, signo, sig_func);)

#if defined(HAVE_SIGACTION)

		if (sigaction( signo, NULL, &sa ) == -1)
		{
			DBG(sqsh_debug(DEBUG_SIG, 
				"sig_install: %s signal %d: Error fetching OS handler: %s\n",
				SIG_TYPE, signo, strerror(errno));)
		}

		/*
		 * Make sure that the handler will not be de-installed after 
		 * being called (this makes life a little easier).
		 */
#if defined(SA_RESETHAND)
		sa.sa_flags &= ~(SA_RESETHAND);
#elif defined(SA_ONESHOT)
		sa.sa_flags &= ~(SA_ONESHOT);
#endif

		sa.sa_handler = sig_func;

		if (sigaction( signo, &sa, NULL ) == -1)
		{
			DBG(sqsh_debug(DEBUG_SIG, 
				"sig_install: %s signal %d: Error installing OS handler: %s\n",
				SIG_TYPE, signo, strerror(errno));)
		}

#else /* BSD_SIGNALS */

		if (signal( signo, sig_func ) == SIG_ERR)
		{
			DBG(sqsh_debug(DEBUG_SIG, 
				"sig_install: %s signal %d: Error OS installing handler: %s\n",
				SIG_TYPE, signo, strerror(errno));)
		}

#endif /* BSD_SIGNALS */

	}

	return True;
}

/*
 * sig_save():
 *
 * Saves the current status of all of the installed signal handlers
 * for restoration following a call to sig_restore().  With this
 * all additions or removals of signal handlers made since a call
 * to sig_save() will be undone with a call to sig_restore().
 */
int sig_save()
{
	++sg_depth;

	DBG(sqsh_debug(DEBUG_SIG, "sig_save: Save depth now %d\n",
	    sg_depth);)

	return sg_depth;
}

/*
 * sig_restore():
 *
 * Restores all signal handlers to the state in which they were 
 * installed since sig_save() was last called.  If sig_restore()
 * is called one more time than sig_save() then all signals handlers
 * are restored to the OS SIG_DFL handler.
 */
int sig_restore()
{
	int      i;
	int      count;
	hndl_t  *h, *t;

#if defined(HAVE_SIGACTION)
	struct sigaction sa;
#endif

	for (i = 0; i < SQSH_NSIGS; i++)
	{
		h = sg_sigs[i].s_hndl;
		count = 0;
		while (h != NULL && h->h_depth == sg_depth)
		{
			DBG(sqsh_debug(DEBUG_SIG,
				"sig_restore: %s signal %d: Removing user handler %p\n", 
				SIG_TYPE, i, h->h_handler);)

			t = h->h_prv;
			free( h );
			h = t;
			++count;
		}
		sg_sigs[i].s_hndl = h;

		if (count > 0 && sg_sigs[i].s_hndl == NULL)
		{
			DBG(sqsh_debug(DEBUG_SIG,
				"sig_restore: %s signal %d: No handlers, restoring to SIG_DFL\n",
				SIG_TYPE, i);)
			
			if (sg_sigs[i].s_installed != SIG_DFL)
			{
				sg_sigs[i].s_installed = SIG_DFL;

				DBG(sqsh_debug(DEBUG_SIG, 
					"sig_install: %s signal %d: Installing OS handler %p\n",
					SIG_TYPE, i, SIG_DFL);)

#if defined(HAVE_SIGACTION)
				if (sigaction( i, NULL, &sa ) == -1)
				{
					DBG(sqsh_debug(DEBUG_SIG, 
						"sig_install: %s signal %d: Error OS fetching handler: %s\n",
						SIG_TYPE, i, strerror(errno));)
				}
				sa.sa_handler = SIG_DFL;

#if defined(SA_ONESHOT)
				sa.sa_flags  &= ~(SA_ONESHOT);
#elif defined(SA_RESETHAND)
				sa.sa_flags  &= ~(SA_RESETHAND);
#endif

				if (sigaction( i, &sa, NULL ) == -1)
				{
					DBG(sqsh_debug(DEBUG_SIG, 
					  "sig_install: %s signal %d: Error OS installing handler: %s\n",
						SIG_TYPE, i, strerror(errno));)
				}

#else  /* !HAVE_SIGACTION */

				if (signal( i, SIG_DFL ) == SIG_ERR)
				{
					DBG(sqsh_debug(DEBUG_SIG, 
					  "sig_install: %s signal %d: Error OS installing handler: %s\n",
						SIG_TYPE, i, strerror(errno));)
				}
#endif
			}
		}
	}

	--sg_depth;

	DBG(sqsh_debug(DEBUG_SIG, "sig_restore: Save depth now %d\n",
	    sg_depth);)

	return sg_depth;
}

/*
 * sig_poll():
 *
 * Returns the number of times signo has been received since the
 * last call to sig_poll.  Note that a signal handler (even SIG_IGN)
 * *must* be installed in order for this to return valid information.
 */
int sig_poll( signo )
	int signo;
{
	int c;

	if (sg_sigs[signo].s_hndl != NULL)
	{
		c = sg_sigs[signo].s_hndl->h_count;
		sg_sigs[signo].s_hndl->h_count = 0;

		DBG(sqsh_debug(DEBUG_SIG, "sig_poll: %d calls to signal %d\n",
		               c, signo);)

		return c;
	}

	DBG(sqsh_debug(DEBUG_SIG, "sig_poll: No calls to signal %d\n",
						signo);)

	return 0;
}

/*
 * sig_clear():
 */
int sig_clear( signo )
	int signo;
{
	if (sg_sigs[signo].s_hndl != NULL)
	{
		sg_sigs[signo].s_hndl->h_count = 0;

		DBG(sqsh_debug(DEBUG_SIG, "sig_poll: Clearing call count for %d\n",
		               signo);)
		return True;
	}

	DBG(sqsh_debug(DEBUG_SIG, "sig_poll: No call count to clear for %d\n",
						signo);)

	return False;
}

static void sig_handler( signo )
	int  signo;
{
	hndl_t   *h;
#if defined(HAVE_SIGACTION) && \
    !defined(SA_RESETHAND)    && \
	 !defined(SA_ONESHOT)
	struct sigaction sa;
#endif

	/*
	 * Make sure that sigint causes a ^C to be displayed.
	 */
	if (signo == SIGINT)
	{
		fputs( "^C\n", stderr );
		fflush( stderr );
	}

	/*
	 * This shouldn't happen.  If sig_install() hasn't been called
	 * or this signal is not valid, or there is no handler installed
	 * then don't do anything.
	 */
	if (signo >= SQSH_NSIGS || sg_sigs[signo].s_hndl == NULL)
	{
#if !defined(HAVE_SIGACTION) && defined(SYSV_SIGNALS)
		signal( signo, sig_handler );
#endif
		return;
	}

#if defined(HAVE_SIGACTION) && \
    !defined(SA_RESETHAND)    && \
	 !defined(SA_ONESHOT)
	DBG(sqsh_debug(DEBUG_SIG, 
		"sig_handler: %s signal %d: Restoring OS handler 0x%p\n",
		SIG_TYPE, signo, sig_handler);)

	sigaction( signo, NULL, &sa );
	sa.sa_handler = sig_handler;
	sigaction( signo, &sa, NULL );
#endif

#if !defined(HAVE_SIGACTION) && \
	 defined(SYSV_SIGNALS)
	DBG(sqsh_debug(DEBUG_SIG, 
		"sig_handler: %s signal %d: Restoring OS handler for SYSV behavior\n",
		SIG_TYPE, signo);)
	signal( signo, sig_handler );
#endif

	for (h = sg_sigs[signo].s_hndl; h != NULL; h = h->h_prv)
	{
		++h->h_count;

		if (h->h_handler == NULL ||
			h->h_handler == SIG_H_POLL ||
			h->h_handler == SIG_H_IGN ||
			h->h_handler == SIG_H_DFL ||
			h->h_handler == SIG_H_ERR)
		{
			DBG(sqsh_debug(DEBUG_SIG, 
				"sig_handler: %s signal %d: No user handler needs calling\n",
				 SIG_TYPE, signo);)

			if ((h->h_flags & SIG_F_CHAIN) == 0)
				break;
			continue;
		}

		DBG(sqsh_debug(DEBUG_SIG, 
			"sig_handler: %s signal %d: Calling user handler %p\n",
		    SIG_TYPE, signo, h->h_handler);)

		h->h_handler( signo, h->h_data );

		if ((h->h_flags & SIG_F_CHAIN) == 0)
		{
			break;
		}
	}

}

#if defined(SIG_TEST)

static void handler1 _ANSI_ARGS(( int, void* ));
static void handler2 _ANSI_ARGS(( int, void* ));

main()
{
	int x = 1;
	int y = 2;
	int  i;

	sqsh_debug_level( DEBUG_SIG );

	sig_install( SIGINT, handler1, (void*)&y, 0 );
	sig_save();
	sig_install( SIGINT, handler2, (void*)&x, SIG_F_CHAIN );

	fprintf( stdout, "Hit ^C\n" );
	for (i = 0; i < 10; i++)
	{
		fprintf( stdout, "." ); fflush(stdout);
		sleep(2);
	}
	sig_restore();

	for (i = 0; i < 10; i++)
	{
		fprintf( stdout, "." ); fflush(stdout);
		sleep(2);
	}

	exit(0);
}

static void handler1( sig, user_data )
	int    sig;
	void  *user_data;
{
	int   x;

	x = *((int*)user_data);
	fprintf(stderr, "handler1: user_data = %d\n", x);
	fflush( stderr );
}

static void handler2( sig, user_data )
	int    sig;
	void  *user_data;
{
	int   x;

	x = *((int*)user_data);
	fprintf(stderr, "handler2: user_data = %d\n", x);
	fflush( stderr );
}

#endif
