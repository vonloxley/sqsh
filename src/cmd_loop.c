/*
 * cmd_loop.c - Primary read-eval-print loop (heart of system)
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
#include "sqsh_varbuf.h"
#include "sqsh_env.h"
#include "sqsh_error.h"
#include "sqsh_cmd.h"
#include "sqsh_job.h"
#include "sqsh_getopt.h"
#include "sqsh_readline.h"
#include "sqsh_stdin.h"
#include "cmd.h"
#include "cmd_misc.h"
#include "cmd_input.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_loop.c,v 1.2 2013/04/18 11:54:43 mwesdorp Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_loop():
 */
int cmd_loop( argc, argv )
	int    argc;
	char  *argv[];
{
	FILE        *input_file = NULL; /* Where input is coming from */
	int          ret;
	int          exit_status;

	/*-- Variables settable by command line options --*/
	char        *file_name     = NULL;
	varbuf_t    *sql_buf       = NULL;
	int          ignore_nofile = False;
	int          do_connect    = True;
	int          have_error    = False;

	/*-- Variables required by sqsh_getopt --*/
	int          c;
	extern int   sqsh_optind;
	extern char *sqsh_optarg;

	/*
	 * Parse the command line options.  Currently there aren't many
	 * of them, just one.
	 */
	while ((c = sqsh_getopt( argc, argv, "ine:" )) != EOF)
	{
		switch (c)
		{
			case 'i' :
				ignore_nofile = True;
				break;
			case 'n' :
				do_connect = False;
				break;
			case 'e' :
				do_connect = False;

				if (sql_buf == NULL)
				{
					if ((sql_buf = varbuf_create( strlen(sqsh_optarg) + 6 )) == NULL)
					{
						fprintf( stderr, "varbuf_create: %s\n", sqsh_get_errstr() );
						return CMD_FAIL;
					}
				}

				varbuf_strcat(sql_buf, sqsh_optarg);
				varbuf_strcat(sql_buf, "\n");

				break;
			default :
				fprintf(stderr, "\\loop: %s\n", sqsh_get_errstr() );
				have_error = True;
		}
	}

	/*
	 * If -e was supplied, then automagically stick a \go
	 * on the end of the whole mess.
	 */
	if (sql_buf != NULL)
	{
		varbuf_strcat( sql_buf, "\\go\n" );
	}

	/*
	 * If there are any arguments left on the command line, then 
	 * we have an error.
	 */
	if( (argc - sqsh_optind) > 1 || have_error ) {
		fprintf( stderr, "Use: \\loop [-i] [-n] [-e cmd | file]\n" );
		return CMD_FAIL;
	}

	if( argc != sqsh_optind )
	{
		if (sql_buf)
		{
			fprintf( stderr,
				"\\loop: Cannot specify -e option and a file name.\n" );
			return CMD_FAIL;
		}
		file_name = argv[sqsh_optind];
	}

	/*
	 * Unless we are asked explicitly not to connect to the database,
	 * we run the connect command to do so.
	 */
	if (do_connect)
	{
		if (jobset_run( g_jobset, "\\connect", &exit_status ) == -1 || exit_status == CMD_FAIL)
		{
			return CMD_FAIL;
		}
	}

	/*
	 * If the user requests that we read from a file, then we need
	 * to open the file to be read from.
	 */
	if (file_name != NULL)
	{
		if ((input_file = fopen( (char*)file_name, "r" )) == NULL)
		{
			if (ignore_nofile)
				return CMD_LEAVEBUF;
			fprintf( stderr, "\\loop: %s: %s\n", (char*)file_name,
						strerror( errno ) );
			return CMD_FAIL;
		}

		/*
		 * Set current input as the new file.
		 */
		sqsh_stdin_file( input_file );
	}
	else if (sql_buf != NULL)
	{
		/*
		 * If the input is to come from the -e flag, then point the
		 * "stdin" to that.
		 */
		sqsh_stdin_buffer( varbuf_getstr(sql_buf), -1 );
	}

	/*
	 * Ok, now read input from the user(?), fortunately, this takes care
	 * of dealing with interrupts for us, so we don't need to worry about
	 * them.
	 */
	ret = cmd_input();

	/*
	 * Destroy the SQL buffer used by the -e command
	 */
	if (sql_buf != NULL)
	{
		sqsh_stdin_pop();
		varbuf_destroy(sql_buf);
	}

	/*
	 * If we opened a file to read, then close it. There is a brief period
	 * here where an interrupt could trash us, but lets hope for the best.
	 */
	if (input_file != NULL)
	{
		/*
		 * Let the stdin be what it used to be.
		 */
		sqsh_stdin_pop();
		fclose( input_file );
	}

	return ret;
}
