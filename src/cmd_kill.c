/*
 * cmd_kill.c - User command to kill background job
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
#include "sqsh_global.h"
#include "sqsh_cmd.h"
#include "sqsh_job.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_kill.c,v 1.1.1.1 2004/04/07 12:35:02 chunkm0nkey Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_kill:
 */
int cmd_kill( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	job_id_t    job_id ;

	/*-- Check the argument count --*/
	if( argc != 2 ) {
		fprintf( stderr, "Use: \\kill job_id\n" ) ;
		return CMD_FAIL ;
	}

	/*
	 * sqsh-2.1.7 - Check for valid job number.
	 */
	if ((job_id = atoi(argv[1])) <= 0) {
		fprintf( stderr, "\\kill: Invalid job_id %s\n", argv[1] ) ;
		return CMD_FAIL ;
	}

	if( (jobset_end( g_jobset, job_id )) == False ) {
		fprintf( stderr, "\\kill: %s\n", sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	return CMD_LEAVEBUF ;
}
