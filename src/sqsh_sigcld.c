/*
 * sqsh_sigcld.c - Queueing and retrieving of SIGCHLD events
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
#include "sqsh_sigcld.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_sigcld.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * Determine which version of the SIGCHLD macro to use.
 */
#if !defined(SIGCHLD)
#  define SIGCHLD   SIGCLD
#endif

/*-- Static Globals --*/
/*
 * sg_sigcld_list: This list contains one entry for every unique pid being
 *                 monitored by all sigcld_t's allocated with sigcld_create.
 *                 It is maintained predominantly by the functions 
 *                 signode_alloc() and signode_free().
 */
static signode_t  *sg_sigcld_list = NULL ;

/*
 * sg_sigcld_init: Used by sigcld_init() (an internal function) to determine
 *                 if the sigcld sub-system has been initialized.
 */
static int         sg_sigcld_init = False ;

/*-- Prototypes --*/
static int        sigcld_init     _ANSI_ARGS(( void )) ;
static void       sigcld_handler  _ANSI_ARGS(( int, void* )) ;
static pid_t      sigcld_wait_all _ANSI_ARGS(( sigcld_t*, int*, int )) ;
static int        sigcld_set      _ANSI_ARGS(( pid_t, int )) ;
static signode_t* signode_alloc   _ANSI_ARGS(( pid_t )) ;
static int        signode_free    _ANSI_ARGS(( signode_t* )) ;

/*
 * sigcld_create():
 *
 * Creates a sigcld context.  That is, a handle on a particular set
 * of pid's that are to be monitored for the child signal event.
 */
sigcld_t* sigcld_create()
{
	sigcld_t *s ;

	/*-- Intialize the system if necessary --*/
	if( sigcld_init() == False )
		return False ;

	/*-- Allocate the data structure --*/
	if( (s = (sigcld_t*)malloc(sizeof(sigcld_t))) == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return NULL ;
	}

	/*-- And initialize its components --*/
	s->s_pids = NULL ;

	return s;
}

/*
 * sigcld_block():
 *
 * Blocks all death-of-child signals from being recieved by
 * the current process.  The sigcld_unblock() function should
 * be used to de-queue all pending signals.
 */
int sigcld_block()
{
	sig_install( SIGCHLD, SIG_H_DFL, (void*)NULL, 0 ) ;

	return True ;
}

/*
 * sigcld_unblock():
 *
 * If sigcld_block() has been called, this function re-installes
 * the necessary signal handler to cope with incoming death-of-child
 * signals.
 */
int sigcld_unblock()
{
	sig_install( SIGCHLD, sigcld_handler, (void*)NULL, 0 ) ;

	return True ;
}

/*
 * sigcld_watch():
 *
 * Registers pid as a process-of-interest to watch for a sigcld event.
 * True is returned upon success, False upon failure.
 */
int sigcld_watch( s, pid )
	sigcld_t *s ;
	pid_t     pid ;
{
	signode_t  *sn ;

	/*-- Check parameters --*/
	if( s == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*
	 * First, check to see if this context is already watching this
	 * particular pid.  If they are already watching it, then our
	 * job is done here.
	 */
 	for( sn = s->s_pids; sn != NULL; sn = sn->sn_nxt ) {
 		if( sn->sn_node->sp_pid == pid )
 			return True ;
	}

	/*
	 * Nope, we aren't already watching this pid, so we need to
	 * allocate a new signode_t to be added to our internal list
	 */
 	if( (sn = signode_alloc( pid )) == NULL )
 		return False ;

	/*
	 * Link the newly created node into our list.
	 */
	sn->sn_nxt = s->s_pids ;
	s->s_pids = sn ;

	return True ;
}

/*
 * sigcld_unwatch():
 *
 * Removes pid from the list of pids to watch in pid.
 */
int sigcld_unwatch( s, pid )
	sigcld_t *s ;
	pid_t     pid ;
{
	signode_t  *sn, *sn_prv ;

	/*-- Check your parameters --*/
	if( s == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*
	 * Attempt to track down the node to be removed from our list.
	 */
	sn_prv = NULL ;
	for( sn = s->s_pids; sn != NULL && sn->sn_node->sp_pid != pid;
	     sn = sn->sn_nxt )
		sn_prv = sn ;

	/*
	 * If the caller attempts to remove a pid that wasn't registered
	 * with sigcld_watch(), then we have an error condition.
	 */
	if( sn == NULL ) {
		sqsh_set_error( SQSH_E_EXIST, NULL ) ;
		return False ;
	}

	/*-- Remove and Deallocate --*/
	if( sn_prv != NULL )
		sn_prv->sn_nxt = sn->sn_nxt ;
	else
		s->s_pids = sn->sn_nxt ;

	if( signode_free( sn ) == False )
		return False ;

	return True ;
}

/*
 * sigcld_wait():
 *
 */
pid_t sigcld_wait( s, pid, exit_status, wait_type )
	sigcld_t  *s ;
	pid_t      pid ;
	int       *exit_status ;
	int        wait_type ;
{
	signode_t  *sn ;
	signode_t  *sn_prv ;
	pid_t       got_pid ;

	/*-- Check parameters --*/
	if( s == NULL || exit_status == NULL || 
	    (wait_type != SIGCLD_BLOCK && wait_type != SIGCLD_NONBLOCK) ) {
 		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
 		return -1 ;
	}

	/*
	 * Special case: if pid is a negative value then we are to wait
	 * for all pids within the context of s.
	 */
	if( pid < 0 )
		return sigcld_wait_all( s, exit_status, wait_type ) ;

	/*
	 * Blast through out list of pids to make sure that the one supplied
	 * by the user is actually one that we are supposed to be watching.
	 */
 	sn_prv = NULL ;
 	for( sn = s->s_pids; sn != NULL && sn->sn_node->sp_pid != pid; 
 	     sn = sn->sn_nxt )
  		sn_prv = sn ;

	/*
	 * If the pid doesn't exist in our list of pids then we need to
	 * let the caller know.
	 */
  	if( sn == NULL ) {
  		sqsh_set_error( SQSH_E_EXIST, NULL ) ;
  		return -1 ;
	}

	do {
		/*
		 * If this node has received a signal then the child is dead
		 * and we can report it back to the caller.
		 */
		if( sn->sn_node->sp_signal ) {

			/*-- Record the exist status --*/
			*exit_status = sn->sn_node->sp_status ;

			/*
			 * Remove the signode_t from our list of pid's to keep
			 * an eye on.  I.e. we don't want to double-report
			 * a signal on a particular pid.
			 */
			if( sn_prv )
				sn_prv->sn_nxt = sn->sn_nxt ;
			else
				s->s_pids = sn->sn_nxt ;

			signode_free( (signode_t*)sn ) ;
			return pid ;
		}

		/*
		 * If we are supposed to block until we receive the signal then
		 * go to sleep for a few seconds seconds.
		 */
		if( wait_type == SIGCLD_BLOCK ) {

			/*
			 * Temporarily disable the old signal handler, because
			 * we are going to explicitly wait for the children
			 * ourselves.
			 */
			sig_install( SIGCHLD, SIG_H_IGN, (void*)NULL, 0 ) ;

			do {
				if( (got_pid = waitpid( -1, exit_status, 0 )) != -1 )
					sigcld_set( got_pid, *exit_status ) ;
			} while( got_pid != -1 && got_pid != pid ) ;

			if( got_pid == -1 && errno != EINTR ) {
				sqsh_set_error( errno, "waitpid: %s", strerror(errno) ) ;
				sig_install( SIGCHLD, sigcld_handler, (void*)NULL, 0 ) ;
				return -1 ;
			}

			/*
			 * Restore the original handler, so we will continue to
			 * pick up lost children.
			 */
			sig_install( SIGCHLD, sigcld_handler, (void*)NULL, 0 ) ;

		}

	} while( wait_type == SIGCLD_BLOCK ) ;

	/*
	 * If we reach this point then then user didn't request a blocking
	 * wait and the process hasn't completed, so we need to let the
	 * user know.
	 */
 	return 0 ;
}

/*
 * sigcld_destroy():
 *
 * Destroys s.
 */
int sigcld_destroy( s )
	sigcld_t     *s ;
{
	signode_t   *sn, *t_sn ;

	/*-- Check ye-olde parameters --*/
	if( s == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	sn = s->s_pids ;
	while( sn != NULL ) {
		t_sn = sn->sn_nxt ;
		signode_free( sn ) ;
		sn = t_sn ;
	}

	free( s ) ;
	return True ;
}

/*********************************************************************
 ** INTERNAL FUNCTIONS
 *********************************************************************/

/*
 * sigcld_set():
 *
 * This function is used by sigcld_wait(), sigcld_wait_all(),
 * and sigcld_handler() to mark a pid as having been completed.
 */
static int sigcld_set( pid, exit_status )
	pid_t  pid ;
	int    exit_status ;
{
	signode_t   *sn ;

	DBG(sqsh_debug(DEBUG_SIGCLD, "sigcld_set: Child %d complete\n",(int)pid);)

	if (WIFEXITED(exit_status))
	{
		exit_status=WEXITSTATUS(exit_status);
	}
	else if (WIFSIGNALED(exit_status))
	{
		exit_status=-(WTERMSIG(exit_status));
	}
	else
	{
		exit_status=-255;
	}

	/*
	 * Now that we have the pid, blast through all of the pids that
	 * are registered by all sigcld_t's looking for this pid. Throw
	 * it away if noone is watching for it.
	 */
	for( sn = sg_sigcld_list; sn != NULL && sn->sn_node->sp_pid != pid; 
		  sn = sn->sn_nxt ) ;

	/*
	 * If one or more lists are watching this pid, then we need to
	 * record the fact that the signal ocurred.
	 */
	if( sn != NULL ) {
		sn->sn_node->sp_signal = True ;
		sn->sn_node->sp_status = exit_status ;
	}

	return True ;
}

/*
 * sigcld_handler():
 *
 * This is the heart of the system.  Each time a SIGCHLD event
 * occures, this function is called, registering the event in the
 * global list of pids being watched by all sigcld_t's.  As a 
 * by-product, all sigcld_t's watching for the pid that generated
 * the event will automatically be updated.
 */
static void sigcld_handler( sig, user_data )
	int   sig ;
	void *user_data;
{
	pid_t      pid ;
	int        exit_status ;


	/*
	 * Attempt to find our which pid generated the SIGCHLD signal.  This
	 * is one of they very very few places in the sqsh code that is allowed
	 * to print to stderr or stdout.  Unfortunately there is no elagent
	 * way of returning an error condition from the signal handler
	 * so I just spew it out to the screen.
	 */
	while( (pid = waitpid( -1, &exit_status, WNOHANG )) > 0  ) {

		DBG(sqsh_debug(DEBUG_SIGCLD, "sigcld_handler: Got signal for pid %d\n",
		               (int)pid);)

		sigcld_set( pid, exit_status ) ;
	}


	if( pid == -1 && errno != ECHILD )
		fprintf( stderr, "sigcld_handler: waitpid: %s\n", strerror( errno ) ) ;

}

static pid_t sigcld_wait_all( s, exit_status, wait_type )
	sigcld_t  *s ;
	int       *exit_status ;
	int        wait_type ;
{
	signode_t  *sn, *sn_prv ;
	pid_t       pid ;
	pid_t       got_pid ;

	DBG(sqsh_debug(DEBUG_SIGCLD, "sigcld_wait_all: Waiting with %s\n" ,
	        (wait_type == SIGCLD_BLOCK) ? "SIGCLD_BLOCK" : "SIGCLD_NONBLOCK" );)

	/*
	 * If there are no processes waiting, then, even if we are supposed
	 * to block, return 0.
	 */
	if( s->s_pids == NULL )
		return 0 ;

	do {

		/*
		 * Blast through all of the pids registered with this
		 * sigcld_t structure and check to see if they have
		 * received a signal.
		 */
		sn_prv = NULL ;
		for( sn = s->s_pids; sn != NULL; sn = sn->sn_nxt ) {

			if( sn->sn_node->sp_signal ) {

				pid          = sn->sn_node->sp_pid ;
				*exit_status = sn->sn_node->sp_status ;

				if( sn_prv != NULL )
					sn_prv->sn_nxt = sn->sn_nxt ;
				else
					s->s_pids = sn->sn_nxt ;

				signode_free( sn ) ;

				DBG(sqsh_debug(DEBUG_SIGCLD,"sigcld_wait_all: Child %d complete!\n",
				               (int)pid );)

				return pid ;
			}

			sn_prv = sn ;
		}


		/*
		 * If we are supposed to block until we receive the signal then
		 * go to sleep for a few seconds.  
		 */
		if( wait_type == SIGCLD_BLOCK ) {

			/*
			 * Temporarily disable the old signal handler, because
			 * we are going to explicitly wait for the children
			 * ourselves.
			 */
			sig_install( SIGCHLD, SIG_H_IGN, (void*)NULL, 0 ) ;

			if( (got_pid = waitpid( -1, exit_status, 0 )) > 0 )
				sigcld_set( got_pid, *exit_status ) ;

			/*
			 * Restore the original handler, so we will continue to
			 * pick up lost children.
			 */
			sig_install( SIGCHLD, sigcld_handler, (void*)NULL, 0 ) ;
			
			if( got_pid == -1 && errno != EINTR ) {
				sqsh_set_error( errno, "waitpid: %s", strerror(errno) ) ;
				return -1 ;
			}

		}

	} while( wait_type == SIGCLD_BLOCK ) ;

	/*
	 * If we reach this point then then user didn't request a blocking
	 * wait and the process hasn't completed, so we need to let the
	 * user know.
	 */
 	return 0 ;
}

/*
 * signode_alloc():
 *
 * This internal function allocates a new signode_t for the supplied
 * pid, automatically adding it to the global list of signals, 
 * sg_sigcld_list.
 */
static signode_t* signode_alloc( pid )
	pid_t  pid ;
{
	sigpid_t   *sp ;
	signode_t  *g_sn,       /* Global signode, from sg_sigcld_list */
	           *sn ;        /* Local signode */

	/*
	 * Allocate a new node for the user's list of pids.
	 */
	if( (sn = (signode_t*)malloc(sizeof(signode_t))) == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return NULL ;
	}

	/*
	 * First, attempt to track down this pid in the global list
	 * of pids that are being watched.
	 */
	for( g_sn = sg_sigcld_list; g_sn != NULL; g_sn = g_sn->sn_nxt ) {
		if( g_sn->sn_node->sp_pid == pid )
			break ;
	}

	/*
	 * If it doesn't exist in the global list then we first need to
	 * stick it in there.
	 */
	if( g_sn == NULL ) {

		if( (sp = (sigpid_t*)malloc( sizeof(sigpid_t) )) == NULL ) {
			free( sn ) ;
			sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
			return NULL ;
		}

		/*-- Intialize the data structure --*/
		sp->sp_pid    = pid ;
		sp->sp_signal = False ;
		sp->sp_status = -1 ;

		/*
		 * Now, allocate a new node in the gobal list.
		 */
		if( (g_sn = (signode_t*)malloc(sizeof(signode_t))) == NULL ) {
			free( sn ) ;
			free( sp ) ;
			sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
			return NULL ;
		}

		/*-- Intialize the data structure --*/
		g_sn->sn_node  = sp ;
		g_sn->sn_ref   = 0 ;
		g_sn->sn_nxt   = sg_sigcld_list ;
		sg_sigcld_list = g_sn ;
	}


	/*
	 * Intialize the node for the user.
	 */
	sn->sn_node = g_sn->sn_node ;
	sn->sn_ref  = ++g_sn->sn_ref ;
	sn->sn_nxt  = NULL ;

	return sn ;
}

/*
 * signode_free():
 *
 * Destroys a signode_t structure, automatically updating reference
 * counts or removing entries from sg_sigcld_list.
 */
static int signode_free( sn )
	signode_t    *sn ;
{
	signode_t  *g_sn, *g_prv ;   /* Global signode_t */

	/*
	 * Although it wouldn't hurt to ignore this, I consider it bad
	 * programming practice to attempt to destroy a NULL pointer.
	 */
	if( sn == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*
	 * Traverse the global list of signode_t's until we find the one
	 * that contains the same sigpid_t as sn does.
	 */
 	g_prv = NULL ;
	for( g_sn = sg_sigcld_list; 
	     g_sn != NULL && g_sn->sn_node != sn->sn_node;
	     g_sn = g_sn->sn_nxt )
  		g_prv = g_sn ;

	/*
	 * This should not happen.
	 */
	if( g_sn == NULL ) {
		sqsh_set_error( SQSH_E_EXIST, NULL ) ;
		return False ;
	}

	/*
	 * Decrement the number of lists that are referencing this
	 * node.
	 */
	--g_sn->sn_ref ;

	/*
	 * If this is the last guy referencing the snode from the global
	 * list then we need to pull it off of the global list.
	 */
	if( g_sn->sn_ref == 0 ) {
		if( g_prv != NULL )
			g_prv->sn_nxt = g_sn->sn_nxt ;
		else
			sg_sigcld_list = g_sn->sn_nxt ;
		free( g_sn->sn_node ) ;
		free( g_sn ) ;
	}

	free( sn ) ;
	return True ;
}

/*
 * sigcld_init():
 *
 * This function initializes the sigcld sub-system by setting up
 * its internal signal handler.
 */
static int sigcld_init()
{
	/*-- If we have already been called, return True --*/
	if( sg_sigcld_init )
		return True ;

	sig_install( SIGCHLD, (sig_handle_t*)sigcld_handler, (void*)NULL, 0 );
	sg_sigcld_init = True ;
	return True ;
}

