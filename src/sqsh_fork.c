/*
 * sqsh_fork.c - Create a child process
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
#include "sqsh_global.h"
#include "sqsh_error.h"
#include "sqsh_fork.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_fork.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * sqsh_fork():
 *
 * This function behaves exactly like the standard UNIX fork with the
 * exception that it resets any necessary globals variables within the
 * context of the child process.
 */
pid_t sqsh_fork()
{
	pid_t  child_pid ;
	
	switch( (child_pid = fork()) ) {

		case -1 :     /* Some sort of error has ocurred */
			sqsh_set_error( errno, "fork: %s", strerror(errno) ) ;
			return -1 ;

		case 0 :      /* Child process */
			/*
			 * It would be bad news for the child process to carry 
			 * around the same CS_CONNECTION pointer as the parent, who
			 * knows what SQL server would do!
			 */
			g_connection = NULL;
			g_context    = NULL;

			/*
			 * We don't need to touch g_cmdset or g_env because these
			 * should be inherited by the child process.  Also, g_sqlbuf
			 * should be inherited, especially because the child has
			 * been created in order to do something with it.
			 */

			/*
			 * Since the child process doesn't have any children of it's
			 * own (per se), we don't want it to inherit the children of
			 * the parent process.
			 */
			jobset_clear(g_jobset) ;
			break ;

		default :    /* Parent process */
			break ;
	}

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return child_pid ;
}
