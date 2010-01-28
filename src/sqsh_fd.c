/*
 * sqsh_fd.c - Routines for safely manipulating file descriptors
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
#include <sys/stat.h>
#include "sqsh_config.h"
#include "sqsh_sigcld.h"
#include "sqsh_env.h"
#include "sqsh_global.h"
#include "sqsh_error.h"
#include "sqsh_fd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_fd.c,v 1.1.1.1 2004/04/07 12:35:03 chunkm0nkey Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * The following defines are used by sqsh_popen() to describe 
 * the mode that it is workin in.
 */
#define POPEN_WR     1
#define POPEN_RD     2
#define POPEN_RDWR   3

/*
 * The following are the possible operations that may be performed
 * upon file descriptors.  More complex operations, such as dup2(2)
 * are merely combinations of these actions.
 */
#define FOP_OPEN     1   /* Explicit open(2) or dup(2) call */
#define FOP_CLOSE    2   /* Explicit close(2) or dup2(2) call */
#define FOP_SAVE     3   /* Pushed onto stack every time a sqsh_fsave() */
                         /* Is called.  A subsequent sqsh_frestore() */
                         /* pops back to the most recent FOP_SAVE */

/*
 * The following data structure represents an operation on a file.
 * multiple instances are maintained in a stack to represent a 
 * complete history of operations on file descriptors.  Thus,
 * all operations may be reveresed by popping off of the stack.
 */
typedef struct fop_st {
	int      fop_type ;      /* One of FOP_OPEN or FOP_CLOSE */
	int      fop_fd ;        /* The file descriptor that was acted opon */
	int      fop_backup ;    /* If the operation was FOP_CLOSE, this is the */
	                         /* dup(2)'ed copy of the closed descriptor */
	struct fop_st *fop_prv ; /* These are maintained in a backwards linked */
	                         /* list in order to maintain a stack */
} fop_t ;

/*
 * Various types of file descriptors that sqsh is currently 
 * controlling.  A type of FD_UNKNOWN means that sqsh could
 * has not yet attempted to determine the disposition of the
 * file descriptor upon startup.
 */
#define FD_UNKNOWN   -1     /* Hasn't been determined if it is open */
#define FD_UNUSED     0     /* Closed file descriptor */
#define FD_FILE       1     /* Talking to a file */
#define FD_PIPE       2     /* Talking to a process */

/*
 * The following contains state information for the a file descriptor.
 * This structure is maintained in an array indexed by file descriptor
 * number.
 */
typedef struct fd_st {
	int    fd_type ;         /* Is this an FD_UNUSED, FD_PIPE, or FD_FILE */
	pid_t  fd_pid ;          /* For fd's of type FD_PIPE, pid of pipe proc */
} fd_t ;

/*-- Static Globals --*/

/*
 * sg_fd_array: When initialized points to an array of fd_t information
 *              indexed by fd (file descriptor) number.  This is initalized
 *              by sqsh_fdinit() and is never free'd.
 */
static fd_t      *sg_fd_array  = NULL ;  /* Array of file descriptors */

/*
 * sg_maxfd:    Initialized by sqsh_fdinit() to be the maximum number of
 *              file descriptors that may be allocated by a process.  This
 *              is also the total length of sg_fd_array, above.
 */
static int       sg_maxfd     = -1 ;    /* Maximum number of file descriptors */

/*
 * sg_fop_stack: Normally is NULL.  If sqsh_fsave() is called, then each
 *               file operation is logged on this stack and popped back
 *               off during sqsh_frestore().
 */
static fop_t    *sg_fop_stack = NULL ;  /* File operation stack */

/*
 * sg_in_fops:   Used to determine if a file operation was done by a user
 *               or during the process of pushing and popping operations
 *               internally.  See fop_push() and fop_pop() for more detail.
 */
static int       sg_in_fops   = False ; /* True/False are we in fop_pop? */

/*
 * sg_sigcld:    This structure is used as a handle on the set of pids
 *               that are currently outstanding via sqsh_popen() calls.
 *               It is initialized by sqsh_fdinit() and is never deallocated.
 */
static sigcld_t *sg_sigcld = NULL ;


/*-- Internal prototypes --*/
static int sqsh_fdinit _ANSI_ARGS(( void )) ;
static int sqsh_fdtype _ANSI_ARGS(( int ));
static int fop_push    _ANSI_ARGS(( int fop_type, int fop_fd )) ;
static int fop_pop     _ANSI_ARGS(( void )) ;


/*
 * sqsh_open():
 *
 * Note, this is one of the few functions that has both ANSI and K&R
 * style declarations.  Apparently ANSI some compilers don't like the 
 * ANSI prototype and the K&R declaration with types smaller than a
 * word (such as mode_t).
 */
#if defined(__ansi__)
int sqsh_open( char *filename, int flags, mode_t mode )
#else
int sqsh_open( filename, flags, mode )
	char   *filename ;
	int     flags ;
	mode_t  mode ;
#endif
{
	int fd ;

	/*-- Make sure everything is initalized --*/
	if( sqsh_fdinit() == False )
		return -1 ;

	/*-- Go ahead and attempt to open the file --*/
	if( mode <= 0 )
		mode = 0666 ;
	
	if( (fd = open( filename, flags, mode )) == -1 ) {
		sqsh_set_error( errno, "sqsh_open: %s", strerror( errno ) ) ;
		return -1 ;
	}

	/*
	 * Now that we have the file descriptor open, lets register this
	 * action so that it may be undone.
	 */
	if( fop_push( FOP_OPEN, fd ) == False ) {
		close( fd ) ;
		return -1 ;
	}

	/*
	 * Now, fill in our array of fd information so we have an accurate
	 * reflection of this file descriptor.
	 */
	sg_fd_array[fd].fd_type = FD_FILE ;
	sg_fd_array[fd].fd_pid  = -1 ;
	return fd ;
}

void sqsh_fdprint()
{
#if defined(DEBUG)
	int    i ;

	/*-- Initialize the fd_array, if necessary --*/
	if( sqsh_fdinit() == False ) {
		sqsh_debug( DEBUG_FD, "sqsh_fdprint: %s\n", sqsh_get_errstr() ) ;
		return ;
	}

	for( i = 0; i < sg_maxfd; i++ ) {
		switch( sg_fd_array[i].fd_type ) {
			case FD_UNUSED :
				break ;
			case FD_PIPE :
				sqsh_debug( DEBUG_FD, "%-03d = FD_PIPE (pid %d)\n",
				            i, sg_fd_array[i].fd_pid ) ;
				break ;
			case FD_FILE :
				sqsh_debug( DEBUG_FD, "%-03d = FD_FILE\n", i ) ;
				break ;
			default :
				sqsh_debug( DEBUG_FD, "%-03d = UNKNOWN\n", i ) ;
		}
	}
#endif
	return ;
}

/*
 * sqsh_maxfd():
 *
 * Provides a common, "portable", interface for requesting the
 * maximum number of file descriptors available.
 */
int sqsh_maxfd()
{
	/*-- Initialize the fd_array, if necessary --*/
	if( sqsh_fdinit() == False )
		return -1 ;

	return sg_maxfd ;
}

/*
 * sqsh_dup():
 *
 * Creates new file descriptor that points to the same stream as fd.
 */
int sqsh_dup( fd )
	int fd ;
{
	int new_fd ;

	/*-- Initialize the fd_array, if necessary --*/
	if( sqsh_fdinit() == False )
		return -1 ;

	/*-- Always check your parameters --*/
	if( fd < 0 || fd > sg_maxfd ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	if( (new_fd = dup(fd )) == -1 ) {
		sqsh_set_error( errno, "dup(%d): %s", fd, strerror(errno) ) ;
		return -1 ;
	}

	/*-- Save the operation --*/
	if( (fop_push( FOP_OPEN, fd )) == False ) {
		close( new_fd ) ;
		return -1 ;
	}

	/*
	 * The dup'ed file descriptor inherits all of the attributes of
	 * the parent file descriptor.
	 */
	sg_fd_array[new_fd].fd_type = sg_fd_array[fd].fd_type ;
	sg_fd_array[new_fd].fd_pid  = sg_fd_array[fd].fd_pid ;

	return new_fd ;
}

int sqsh_dup2( orig_fd, copy_fd )
	int   orig_fd ;
	int   copy_fd ;
{
	/*-- Initialize the fd_array, if necessary --*/
	if( sqsh_fdinit() == False )
		return -1 ;

	/*-- Always check your parameters --*/
	if( orig_fd < 0 || orig_fd > sg_maxfd || 
		 copy_fd < 0 || copy_fd > sg_maxfd ){
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	/*
	 * If orig_fd and copy_fd are identical, there is nothing to
	 * do here.
	 */
	if( orig_fd == copy_fd ) {
		return copy_fd ;
	}

	/*
	 * Now, call sqsh_close() on the copy_fd so that we can clean
	 * up whatever copy_fd is holding (it may be a pipe or something).
	 * Hmmm...should I be checking my error condition here?
	 */
	sqsh_close( copy_fd ) ;

	/*
	 * Now, go ahead and perform the dup2().
	 */
	if( dup2( orig_fd, copy_fd ) == -1 ) {
		sqsh_set_error( errno, "dup2(%d,%d): %s", orig_fd, copy_fd,
		                strerror(errno) ) ;
		return -1 ;
	}

	/*
	 * Since copy_fd is now really just a dup(2) of orig_fd, we mark
	 * this as a fresh open.  If sqsh_frestore() is called then
	 * copy_fd will be closed.
	 */
	if( (fop_push( FOP_OPEN, copy_fd )) == False ) {
		close( copy_fd ) ;
		return -1 ;
	}

	/*
	 * The new copy of the old file descriptor inherits all of the
	 * attributes of the original, with the exception of fd_orig.
	 */
	sg_fd_array[copy_fd].fd_type = sg_fd_array[orig_fd].fd_type ;
	sg_fd_array[copy_fd].fd_pid  = sg_fd_array[orig_fd].fd_pid ;

	return copy_fd ;
}

/*
 * sqsh_popen():
 *
 * Has basically the same behaviour as popen.
 */
int sqsh_popen( shell_job, mode, outfd, infd )
	char     *shell_job;
	char     *mode;
	int      *outfd;
	int      *infd;
{
	int     fd = -1;
	int     pfd1[2];
	int     pfd2[2];
	pid_t   pid;
	int     m;
	char   *shell;

	/*-- Initialize the fd_array, if necessary --*/
	if (sqsh_fdinit() == False)
	{
		return -1 ;
	}
	
	/*-- Always check your arguments --*/
	if (shell_job == NULL || mode == NULL)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return -1;
	}

	if (strcmp(mode, "r") == 0)
	{
		m = POPEN_RD;
	}
	else if (strcmp(mode, "w") == 0)
	{
		m = POPEN_WR;
	}
	else if (strcmp(mode, "rw") == 0 ||
	         strcmp(mode, "wr") == 0)
	{
		m = POPEN_RDWR;

		if (infd == NULL || outfd == NULL)
		{
			sqsh_set_error( SQSH_E_BADPARAM, NULL );
			return -1;
		}
	}
	else
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return -1;
	}

	if (pipe(pfd1) < 0)
	{
		sqsh_set_error( errno, "pipe: %s", strerror( errno ) ) ;
		return -1 ;
	}

	if (m == POPEN_RDWR)
	{
		if (pipe(pfd2) < 0)
		{
			close( pfd1[0] );
			close( pfd1[1] );
			sqsh_set_error( errno, "pipe: %s", strerror( errno ) ) ;
			return -1 ;
		}
	}

	/*
	 * Fork, and create a child process in which the popen process
	 * will be execed.
	 */
	switch (pid = fork()) {

		case -1 :                 /* Error condition */
			sqsh_set_error( errno, "fork: %s", strerror( errno ) ) ;
			close(pfd1[0]) ;
			close(pfd1[1]) ;

			if (m == POPEN_RDWR)
			{
				close( pfd2[0] );
				close( pfd2[1] );
			}
			return -1 ;

		case 0 :                  /* Child process */
			/*
			 * If the parent wants to write to us, then we need to attach
			 * the pipe to our standard in.
			 */
			switch (m)
			{
				case POPEN_RD:
					/*-- Close the write side of the pipe --*/
					close( pfd1[0] );

					/*-- Attach the reading end of the pipe to our stdin --*/
					if (pfd1[1] != fileno(stdout))
					{
						dup2( pfd1[1], fileno(stdout));
					}
					close( pfd1[1] );
					break;

				case POPEN_WR:
					/*
					 * The parent wishes to read from us, so we need to attach
					 * the reading side of the pipe to our standard out.
					 */
					close(pfd1[1]);

					if (pfd1[0] != fileno(stdin))
					{
						dup2( pfd1[0], fileno(stdin) );
					}
					close( pfd1[0] );
					break;

				case POPEN_RDWR:
					close(pfd1[1]);
					close(pfd2[0]);

					if (pfd1[0] != fileno(stdin))
					{
						dup2( pfd1[0], fileno(stdin) );
					}
					close( pfd1[0] );

					if (pfd2[1] != fileno(stdout))
					{
						dup2( pfd2[1], fileno(stdout) );
					}
					close( pfd2[1] );
					break;
			}


			/*
			** Look up which shell we are to be using.
			*/
			env_get( g_env, "SHELL", &shell );
			if (shell == NULL || *shell == '\0')
			{
				shell = SQSH_SHELL;
			}

			execl( shell, "sh", "-c", shell_job, (char*)0 ) ;
			_exit(127) ;

		default:   /* Parent process */

			switch (m)
			{
				case POPEN_RD:
					fd = pfd1[0] ;
					close(pfd1[1]) ;

					sg_fd_array[fd].fd_type = FD_PIPE ;
					sg_fd_array[fd].fd_pid  = pid ;

					if (fop_push( FOP_OPEN, fd ) == False)
					{
						close( fd );
						return -1;
					}

					if (infd != NULL)
					{
						*infd = fd;
					}

					DBG(sqsh_debug(DEBUG_FD, 
			      	"sqsh_popen: Started process %d (%s) on fd %d\n",
			          (int)pid, shell_job, fd );)
					break;
				
				case POPEN_WR:
					fd = pfd1[1] ;
					close(pfd1[0]) ;

					sg_fd_array[fd].fd_type = FD_PIPE ;
					sg_fd_array[fd].fd_pid  = pid ;

					if (fop_push( FOP_OPEN, fd ) == False)
					{
						close( fd );
						return -1;
					}

					if (outfd != NULL)
					{
						*outfd = fd;
					}
					DBG(sqsh_debug(DEBUG_FD, 
			      	"sqsh_popen: Started process %d (%s) on fd %d\n",
			          (int)pid, shell_job, fd );)
					break;
				
				case POPEN_RDWR:
					fd = pfd1[1];
					close( pfd1[0] );
					close( pfd2[1] );

					sg_fd_array[pfd1[1]].fd_type = FD_FILE;
					sg_fd_array[pfd1[1]].fd_pid  = pid;
					sg_fd_array[pfd2[0]].fd_type = FD_PIPE;
					sg_fd_array[pfd2[0]].fd_pid  = pid;

					if (fop_push( FOP_OPEN, pfd1[1] ) == False)
					{
						close( pfd1[1] );
						close( pfd2[0] );
						return -1;
					}

					if (fop_push( FOP_OPEN, pfd2[0] ) == False)
					{
						sqsh_close( pfd1[1] );
						close( pfd2[0] );
						return -1;
					}

					*outfd = pfd1[1];
					*infd  = pfd2[0];

					DBG(sqsh_debug(DEBUG_FD, 
			      	"sqsh_popen: Started process %d (%s) out = %d, in = %d\n",
			          (int)pid, shell_job, pfd1[1], pfd2[0] );)
					break;

				default:
					break;
			}

			/*-- Tell the sigcld sub-system to watch for child process --*/
			sigcld_watch( sg_sigcld, pid ) ;
			sigcld_unblock () ; /* sqsh-2.1.7 - Force unblock/enable signal handler */
			return( fd ) ;

	} /* switch (fork()) */
	/* NOTREACHED */
}

/*
 * sqsh_ptest():
 *
 * Tests to see if file is a sqsh_pclose'able file pointer.  Returns
 * a pid if it is, 0 if it isn't or -1 if an error
 * ocurred.
 */
pid_t sqsh_ptest( fd )
	int fd ;
{
	/*-- Initialize the fd_array, if necessary --*/
	if( sqsh_fdinit() == False )
		return -1 ;

	/*-- Check parameters --*/
	if( fd < 0 || fd > sg_maxfd ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	/*-- See if fileno is a pclose'able fileno --*/
	if( sg_fd_array[fd].fd_type == FD_PIPE )
		return sg_fd_array[fd].fd_pid ;
	return 0 ;
}

/*
 * sqsh_close():
 *
 * Closes file descriptor fd.  If fd is a file descriptor created with
 * sqsh_popen() and is the last file descriptor to reference the pipe,
 * then the descriptor is closed and wait(2) is called called on the
 * child.  If sqsh_save_fd() was called on the fd prior to sqsh_close(),
 * the the file descriptor is backed up prior to closing.
 */
int sqsh_close( fd )
	int   fd ;
{
	int     exit_status ;
	int     i ;
	char    exit_str[16];

	/*-- Initialize the fd_array, if necessary --*/
	if( sqsh_fdinit() == False )
		return -1 ;

	/*-- Check parameters --*/
	if( fd < 0 || fd > sg_maxfd ) {
		return -1 ;
	}

	/*
	 * If the file descriptor was never actually opened then
	 * consider this close a success.
	 */
	if (sg_fd_array[fd].fd_type == FD_UNKNOWN)
	{
		sg_fd_array[fd].fd_type = sqsh_fdtype( fd );
	}

	if (sg_fd_array[fd].fd_type == FD_UNUSED)
	{
		return 1;
	}

	/*
	 * If requested, attempt to push the close operation on the
	 * operation save stack.
	 */
	if( fop_push( FOP_CLOSE, fd ) == False )
		return -1 ;
	
	/*
	 * Just before we close the file descriptor, we want to check to
	 * see if it is a "special" descriptor...stdin, stdout, or stderr.
	 * If it is special, then we want to make sure that the associated
	 * FILE structure has been flushed.
	 */
	if (fd == fileno(stdout))
	{
		fflush( stdout );
	}
	else if (fd == fileno(stderr))
	{
		fflush( stderr );
	}

	/*-- Close it --*/
	close( fd ) ;

	/*-- See if fileno is a pclose'able fileno --*/
	if( sg_fd_array[fd].fd_type == FD_PIPE ) {

		/*
		 * Since this is a pipe, we need to blast through all of the
		 * file descriptors and make sure that noone else has dup(2)'ed
		 * the pipe descriptor, otherwise our close(2) didn't kill the
		 * child.
		 */
		for( i = 0; i < sg_maxfd; i++ ) {
			if( sg_fd_array[i].fd_type == FD_PIPE &&
			    sg_fd_array[i].fd_pid  == sg_fd_array[fd].fd_pid && i != fd )
				break ;
		}

		/*
		 * If i == sg_maxfd then we didn't find another file descriptor
		 * sharing our pipe, so we have actually closed it and may
		 * now wait for the child to die.
		 */
		if( i == sg_maxfd ) {

			DBG(sqsh_debug( DEBUG_FD, "sigcld_close: Waiting for %d to die\n",
			                (int) sg_fd_array[fd].fd_pid );)

			if( sigcld_wait( sg_sigcld,              /* Set of pids */
			                 sg_fd_array[fd].fd_pid, /* Individual pid */
			                 &exit_status,           /* Exit status */
			                 SIGCLD_BLOCK ) == -1 ) {
				return -1;
			}
			sprintf( exit_str, "%d", exit_status );
			env_set( g_internal_env, "?", exit_str );

			DBG(sqsh_debug( DEBUG_FD, "sigcld_close: %d is dead\n",
			                (int) sg_fd_array[fd].fd_pid );)
		} 
	} 

	/*
	 * Mark this file descriptor as being closed.
	 */
	sg_fd_array[fd].fd_type = FD_UNUSED ;
	sg_fd_array[fd].fd_pid  = -1 ;

	return 1 ;
}

/*
 * sqsh_fsave():
 *
 * Saves the current state of all file descriptors.  This causes any future
 * actions involving file descriptors (performed with the appropriate sqsh
 * functions) to be recorded in a stack.  A subsequent call to sqsh_frestore()
 * will cause each action to be popped off of the stack (i.e. a sqsh_close()
 * would be re-opened, and a sqsh_open() would be cloesd), this process would
 * continue until all actions are reversed since the last sqsh_fsave().
 */
int sqsh_fsave()
{
	/*-- Initialize the fd_array, if necessary --*/
	if( sqsh_fdinit() == False )
		return False ;

	/*
	 * Now, record on the save stack this operation.  This way we
	 * can roll back until we hit the most recent FOP_SAVE operation.
	 */
	if( fop_push( FOP_SAVE, -1 ) == False )
		return False ;

	return True ;
}

int sqsh_frestore()
{
	int ret ;

	/*-- Initialize the fd_array, if necessary --*/
	if( sqsh_fdinit() == False )
		return False ;

	while( (ret = fop_pop()) != FOP_SAVE ) {
		if( ret == -1 )
			return False ;
	}

	return True ;
}

#if defined(HAVE_POLL) && defined(HAVE_STROPTS_H)

int sqsh_poll( fds, nfds, timeout )
	struct pollfd  *fds;
	unsigned long   nfds;
	int             timeout;
{
	if (poll( fds, nfds, timeout ) == -1)
	{
		sqsh_set_error( errno, NULL );
		return -1;
	}
	return 0;
}

#else   /* HAVE_SELECT */

int sqsh_poll( fds, nfds, timeout )
	struct pollfd      *fds;
	unsigned long       nfds;
	int                 timeout;
{
	fd_set         readfds;
	fd_set         writefds;
	struct timeval to, *pto;
	int            i;
	int            max_fd = 0;

	FD_ZERO( &readfds );
	FD_ZERO( &writefds );

	for (i = 0; i < (int)nfds; i++)
	{
		max_fd = max(max_fd, fds[i].fd);

		if (fds[i].events & POLLIN)
		{
			FD_SET(fds[i].fd, &readfds);
		}
		if (fds[i].events & POLLOUT)
		{
			FD_SET(fds[i].fd, &writefds);
		}
	}

	if (timeout == 0)
	{
		pto = NULL;
	}
	else
	{
		to.tv_sec  = timeout / 1000000;
		to.tv_usec = timeout % 1000000;
		pto = &to;
	}

	if (select( max_fd+1, &readfds, &writefds, NULL, pto ) == -1)
	{
		sqsh_set_error( errno, NULL );
		return -1;
	}

	for (i = 0; i < nfds; i++)
	{
		fds[i].revents = 0;
		if (FD_ISSET(fds[i].fd, &readfds))
		{
			fds[i].revents |= POLLIN;
		}
		if (FD_ISSET(fds[i].fd, &writefds))
		{
			fds[i].revents |= POLLOUT;
		}
	}

	return 0;
}

#endif /* HAVE_POLL */

/*
 * sqsh_fdtype():
 *  
 * Internal function used to determine the disposition of a file
 * descriptor before any sqsh_fd() operation is performed upon
 * it.  This essentially attempts to fstat() the file descriptor.
 * If EBADF is returned, then the file descriptor is assumed to
 * be closed, otherwise it is assumed to be open and talking to
 * a file.
 */
static int sqsh_fdtype( fd )
	int fd;
{
	struct stat buf;

	if (fstat( fd, &buf ) == -1 && errno == EBADF)
	{
		DBG(sqsh_debug(DEBUG_FD,"sqsh_fdtype: fd %d = FD_UNUSED (%s)\n",
		    fd, strerror(errno));)
		return FD_UNUSED;
	}

	DBG(sqsh_debug(DEBUG_FD,"sqsh_fdtype: fd %d = FD_FILE\n", fd);)

	return FD_FILE;
}

/*
 * sqsh_fdinit():
 *
 * The following function is called at the beginning of just about every
 * sqsh_*() file descriptor function to intialize the global array of
 * file descriptor information (if necessary).  True is returned if
 * the array is already initialized or it is succesfully intialized.
 * False is returned if there are problems.
 */
static int sqsh_fdinit()
{
	int i ;

	/*
	 * If our array of information about the file descriptors hasn't
	 * been initialized then we need to do so.
	 */
	if( sg_fd_array == NULL ) {

		/*
		 * This rather strange statements does the following:  If
		 * _SC_OPEN_MAX is defined then we have the sysconf function
		 * available to request the information we need.  If it isn't
		 * defined, or failes to return a valid number then we take
		 * a guess.
		 */
#ifdef _SC_OPEN_MAX
		if( (sg_maxfd = sysconf( _SC_OPEN_MAX )) == -1 )
#endif
			sg_maxfd = SQSH_MAXFD ;

		/*
		 * Now, allocate our array of file descriptors.
		 */
		if( (sg_fd_array = (fd_t*)malloc(sizeof(fd_t) * sg_maxfd)) == NULL ) {
			sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
			return False ;
		}

		/*
		 * Initially we place each file descriptor in an unknown state.
		 * As we attempt to dup(), dup2(), open() and close() a file
		 * descriptor we will call sqsh_fdtype() to determine the state
		 * of the file descriptor.
		 */
		for( i = 0; i < sg_maxfd; i++ )
		{
			sg_fd_array[i].fd_type = FD_UNKNOWN;
			sg_fd_array[i].fd_pid  = -1;
		}

		/*
		 * Allocate a sigcld_t structure to contain the set of pids
		 * that are outstanding via sqsh_popen() calls.
		 */
	 	if( (sg_sigcld = sigcld_create()) == NULL ) {
	 		sqsh_set_error( sqsh_get_error(), "sigcld_create: %s",
	 		                sqsh_get_errstr() ) ;
			return False ;
		}
	}

	return True ;
}



/*
 * fop_push():
 *
 * If sqsh_fsave() has been called, then the file operation represented by 
 * fop_type acting upon fop_fd is pushed onto the save stack.  If necessary
 * the file descriptor that is acted opon is dup(2)ed and backed up for
 * later restoration.  True is returned if the operation was a success
 * or sqsh_fsave() is hasn't been called.  False is returned upon some sort
 * of failure during the process and sqsh_set_error() is used to indicate
 * the error condition.
 */
static int fop_push( fop_type, fop_fd )
	int fop_type ;
	int fop_fd ;
{
	fop_t    *fop, *fop_nxt ;

	/*
	 * If sg_in_fops is set then we are already in the middle of either a 
	 * fop_push() or a fop_save(), so we don't want this to do anything. 
	 * Also, if the sg_fop_stack is NULL then we know that sqsh_fsave()
	 * hasn't been called...However, sqsh_fsave() itself needs to record
	 * a FOP_SAVE so in this one case we want to allow this function to
	 * be called even if sg_fop_stack is NULL.
	 */
	if( sg_in_fops == True || (sg_fop_stack == NULL && fop_type != FOP_SAVE ) )
		return True ;

	/*
	 * If the operation being performed is a FOP_CLOSE and the file
	 * descriptor being closed was created since the most recent
	 * FOP_SAVE, then all we have to do is erase the fact that the
	 * old file descriptor was ever opened at all; there is nothing
	 * to reverse.
 	 */
 	if( fop_type == FOP_CLOSE ) {

		/*
		 * Traverse down through the stack of file operations until
		 * we either hit the FOP_OPEN that opened this file descriptor,
		 * or until we hit the most recent save point.
		 */
 		fop_nxt = NULL ;
 		for( fop = sg_fop_stack; fop != NULL; fop = fop->fop_prv ) {
 			if( fop->fop_type == FOP_SAVE ||
 			    (fop->fop_type == FOP_OPEN && fop->fop_fd == fop_fd ))
 				break ;
			fop_nxt = fop ;
 		}

		/*
		 * If fop points to the open that created the file descriptor
		 * to be closed, then we need to unlink the FOP_OPEN node from
		 * the stack and destroy it.
		 */
 		if( fop != NULL && fop->fop_type == FOP_OPEN ) {

			DBG(sqsh_debug(DEBUG_FD, 
			               "fop_push(FOP_CLOSE,%d): Reversing FOP_OPEN\n",
			                fop_fd );)

 			if( fop_nxt != NULL )
 				fop_nxt->fop_prv = fop->fop_prv ;
			else
				sg_fop_stack = fop->fop_prv ;

 			free( fop ) ;
 			return True ;
 		}
 	}

	/*
	 * Attempt to allocate a new data structure to push on to the stack.
	 * Notice that all error conditions returned here indicate the function
	 * from which they came.  This was none of the other sqsh fd functions
	 * need to do this.
	 */
	if( (fop = (fop_t*)malloc(sizeof(fop_t))) == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, "fop_push: Memory allocation failure" ) ;
		return False ;
	}

	/*-- Fill in what we can --*/
	fop->fop_type   = fop_type ;
	fop->fop_fd     = fop_fd ;
	fop->fop_backup = -1 ;

	/*
	 * If the operation was a FOP_CLOSE, then we want to create a backup
	 * copy of the file descriptor.  Note that this function should not
	 * actually close the original file descriptor.  That is the responsiblity
	 * of the caller.
	 */
	if( fop_type == FOP_CLOSE ) {
		/*
		 * Attempt to dup the file descriptor that we are about to
		 * close.  Give up if we fail.  Setting the sg_in_fops guarentees
		 * that calling any sqsh_() function will not recursively
		 * cause fop_push() to be called.
		 */
		sg_in_fops = True ;
		if( (fop->fop_backup = sqsh_dup( fop_fd )) == -1 ) {
			free( fop ) ;
			sqsh_set_error( sqsh_get_error(), "fop_push: %s", sqsh_get_errstr() ) ;
			sg_in_fops = False ;
			return False ;
		}
		sg_in_fops = False ;

	}

#ifdef DEBUG
	switch( fop_type ) {
		case FOP_CLOSE :
			sqsh_debug( DEBUG_FD, "fop_push(FOP_CLOSE, %d): Backup in fd %d\n", 
							fop_fd, fop->fop_backup ) ;
			break ;
		case FOP_OPEN :
			sqsh_debug( DEBUG_FD, "fop_push(FOP_OPEN, %d)\n", fop_fd ) ;
			break ;
		case FOP_SAVE :
			sqsh_debug( DEBUG_FD, "fop_push(FOP_SAVE): Adding save-point\n" ) ;
			break ;
	}
#endif /* DEBUG */


	/*-- Now stick the operation on the stack --*/
	fop->fop_prv = sg_fop_stack ;
	sg_fop_stack = fop ;

	return True ;
}

/*
 * fop_pop():
 *
 * The counter-part to fop_push().  This function undoes the last
 * file operation pushed onto the save-stack via fop_push(), returning
 * the type of action restored as parameter.  Upon error condition
 * -1 is returned.
 */
static int fop_pop()
{
	fop_t   *top ;
	int      fop_type ;

	/*
	 * If we reach the bottom of the stack then we'll pretend that
	 * we hit a save-point.
	 */
	if( sg_fop_stack == NULL )
		return FOP_SAVE ;
	
	/*-- The top off of the stack --*/
	top = sg_fop_stack ;
	sg_fop_stack = sg_fop_stack->fop_prv ;

	/*
	 * Save the type of operation that we just pulled off of the
	 * stack for later use because we shall soon be free(3c)ing 
	 * the fop.
	 */
	fop_type = top->fop_type ;

#ifdef DEBUG
	switch( fop_type ) {
		case FOP_CLOSE :
			sqsh_debug( DEBUG_FD, "fop_pop(FOP_CLOSE): Restoring %d using fd %d\n",
					  top->fop_fd, top->fop_backup ) ;
			break ;
		case FOP_OPEN :
			sqsh_debug( DEBUG_FD, "fop_pop(FOP_OPEN): Closing %d\n", top->fop_fd );
			break ;
		case FOP_SAVE :
			sqsh_debug( DEBUG_FD, "fop_pop(FOP_SAVE): Restoring save-point\n" ) ;
			break ;
	}
#endif /* DEBUG */

	/*
	 * The following switch determines what to do to reverse the
	 * file operation.  Note that we flag, but ignore all error
	 * conditions.  That is, a fop_pop() is *always* succesfull,
	 * (i.e. the operation is removed from the stack).
	 */
	switch( fop_type ) {

		/*
		 * FOP_SAVE is simply an indicator that sqsh_fsave() has been
		 * called and is returned simply to allow sqsh_frestore() to
		 * know when to stop calling fop_pop().
		 */
		case FOP_SAVE :
			break ;
		
		/*
		 * FOP_CLOSE indicates that a file descriptor was to be closed.
		 * If this was the case then we made a backup of the descriptor
		 * in fop->fop_backup which we will use to restore the file
		 * descriptor to its original state.
		 */
		case FOP_CLOSE :

			sg_in_fops = True ;   /* Protect against self recursion */

			/*
			 * Attempt to restore the original file descriptor using
			 * dup2.  This should cause top->fop_fd to be closed and
			 * setting sg_in_fops guarentees that this operation won't
			 * be recorded on the operation stack.
			 */
			if( sqsh_dup2( top->fop_backup, top->fop_fd ) == -1 ) {
				sqsh_set_error( sqsh_get_error(), "fop_pop: %s",
				                sqsh_get_errstr() ) ;
			}

			/*
			 * Now, close the backup file descriptor because we don't need
			 * it any more.  Note that we don't pay any attention to this
			 * close to verify that it worked.  I suppose that we should.
			 */
			sqsh_close( top->fop_backup ) ;

		 	sg_in_fops = False ;

			break ;

		case FOP_OPEN :
			/*
			 * This is easy. If a file descriptor was opened then we
			 * need to close it.
			 */
		 	sg_in_fops = True ;
			if( sqsh_close( top->fop_fd ) == -1 ) {
				sqsh_set_error( sqsh_get_error(), "fop_pop: sqsh_close: %s",
				                sqsh_get_errstr() ) ;
			}
		 	sg_in_fops = False ;
			break ;
		
		default :
			sqsh_set_error( SQSH_E_EXIST, "fop_pop: Unknown operation" ) ;
			break ;
	}


	/*-- Clean up and get out of here --*/
	free( top ) ;
	return fop_type ;
}
