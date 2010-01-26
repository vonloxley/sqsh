/*
 * cmd_jobs.c - User command to display executing background jobs
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
#include "sqsh_cmd.h"
#include "sqsh_job.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_jobs.c,v 1.1.1.1 2004/04/07 12:35:02 chunkm0nkey Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_jobs:
 */
int cmd_jobs( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	int    i ;
	job_t *j ;
	char   date_str[64] ;
	time_t cur_time ;

	/*
	 * This is probably unnecessary, but could save your ass if
	 * you didn't mean to type exit.
	 */
	if( argc > 2 || (argc == 2 && strcmp (argv[1], "-i") != 0)) {
		fprintf( stderr, "Use: \\jobs [-i]\n" ) ;
		return CMD_FAIL ;
	}

	/*
	 * Retrieve the current time to figure out how long the jobs have
	 * been running.
	 */
	time(&cur_time) ;

	/*
	 * The following should really be implemented through some function
	 * interfaces to the job module. However this is really easier,
	 * it just doesn't appeal to my better nature.
	 */
 	for( i = 0; i < g_jobset->js_hsize; i++ ) {
 		for( j = g_jobset->js_jobs[i]; j != NULL; j = j->job_nxt ) {

 			cftime( date_str, "%d-%b-%y %H:%M:%S", &j->job_start ) ;

			if( j->job_flags & JOB_DONE )
				printf( "Job #%d: %s (done - %d secs)\n", j->job_id, date_str,
						  (int)(j->job_end - j->job_start) ) ;
			else
				printf( "Job #%d: %s (%d secs)\n", j->job_id, date_str,
						  (int)(cur_time - j->job_start) ) ;
			if (argc == 2 && strcmp ( argv[1], "-i") == 0) {
				printf( "\tFlags    : %d\n", j->job_flags ) ;
				printf( "\tOutput   : %s\n", j->job_output ) ;
				printf( "\tStatus   : %d\n", j->job_status ) ;
				printf( "\tChild pid: %d\n", j->job_pid ) ;
			}
 		}
	}

	return CMD_LEAVEBUF ;
}
