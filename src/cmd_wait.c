/*
 * cmd_wait.c - User command to wait for background job to complete
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
#include "sqsh_error.h"
#include "sqsh_global.h"
#include "sqsh_cmd.h"
#include "sqsh_job.h"
#include "sqsh_stdin.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_wait.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_wait:
 */
int cmd_wait( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	job_id_t    job_id ;
	job_id_t    res_id ;
	char       *defer_file ;
	int         exit_status ;
	struct stat stat_buf ;

	if( argc != 2 ) {
		fprintf( stderr, "Use: \\wait job_id\n" ) ;
		return CMD_FAIL ;
	}

	job_id = (job_id_t)atoi(argv[1]) ;

	if( (res_id = jobset_wait( g_jobset, job_id, &exit_status,
										JOB_BLOCK )) == -1 ) {
		fprintf( stderr, "\\wait: %s\n", sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	if( res_id == 0 ) {
		fprintf( stderr, "\\wait: No jobs pending\n" ) ;
		return CMD_FAIL ;
	}

	/*
	 * If something completed, we need to notify the user
	 * of this.  Also, we need to check if the command has any output
	 * to speak of.  If it didn't then we can go ahead and terminate
	 * the job.
	 */
	/*-- Get the name of the defer file --*/
	defer_file = jobset_get_defer( g_jobset, job_id ) ;

	/*
	 * If there is a defer_file, then we will stat the file
	 * to get information on it.  If we can't even stat the
	 * file, or if the file is of zero length, then we
	 * pretend that there is no defer file.
	 */
	if( defer_file != NULL ) {
		if( stat( defer_file, &stat_buf ) == -1 )
			defer_file = NULL ;
		else {
			if( stat_buf.st_size == 0 )
				defer_file = NULL ;
		}
	}

	/*
	 * If there is no defer file, then we can go ahead and end
	 * the job and let the user know that it completed without
	 * any output.
	 */
	if( defer_file == NULL ) {

		if (sqsh_stdin_isatty())
			fprintf(stdout, "Job #%d complete (no output)\n", res_id);

		if( jobset_end( g_jobset, res_id ) == False ) {
			fprintf( stderr, "\\wait: Unable to end job %d: %s\n",
						(int)res_id, sqsh_get_errstr() ) ;
		}
	} else {

		/*
		 * If there is output pending then we need to inform the
		 * user.  In this case we don't kill the job.  It is the
		 * responsibility of the user to run the appropriate
		 * command to terminate the job.
		 */
		if (sqsh_stdin_isatty()) {
			fprintf( stdout, "Job #%d complete (output pending)\n",
						res_id ) ;
		}
	}

	return CMD_LEAVEBUF ;
}
