/*
 * cmd_show.c - User command to display output of background jobs
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
#include "sqsh_varbuf.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "sqsh_job.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_show.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_show:
 */
int cmd_show( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	char        str[1024] ;
	job_id_t    job_id ;
	char       *output ;
	FILE       *file ;

	/*-- Check the argument count --*/
	if( argc != 2 ) {
		fprintf( stderr, "Use: \\show [job_id]\n" ) ;
		return CMD_FAIL ;
	}

	job_id = (job_id_t)atoi(argv[1]) ;

	switch( jobset_is_done( g_jobset, job_id ) ) {
		case  -1 :  
			fprintf( stderr, "\\show: %s\n", sqsh_get_errstr() ) ;
			return CMD_FAIL ;
			break ;

		case   0 :
			fprintf( stderr, "\\show: Job %d has not completed\n", job_id ) ;
			return CMD_FAIL ;
			break ;

		default :
			break ;
	}


	/*
	 * Request the name of the file in which the jobs output
	 * has been deferred.
	 */
	if( (output = jobset_get_defer( g_jobset, job_id )) == NULL ) {
		fprintf( stderr, "\\show: Unable to retrieve output: %s\n",
		         sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	/*
	 * Attempt to open the output file.
	 */
	if( (file = fopen( output, "r" )) == NULL ) {
		fprintf( stderr, "\\show: Unable to open %s: %s\n",
		         output, strerror(errno) ) ;
		return CMD_FAIL ;
	}

	while( fgets( str, sizeof(str), file ) != NULL )
		fprintf( stdout, "%s", str ) ;

	fclose( file ) ;

	if( (jobset_end( g_jobset, job_id )) == False ) {
		fprintf( stderr, "\\show: Error killing %d, %s\n", 
		         (int)job_id, sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	return CMD_LEAVEBUF ;
}
