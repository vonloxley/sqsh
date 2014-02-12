/*
 * cmd_run.c - Execute a batch file from the prompt
 *
 * Copyright (C) 2014 by Martin Wesdorp.
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
static char RCS_Id[] = "$Id: cmd_run.c,v 1.3 2014/02/10 14:28:45 mwesdorp Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_run():
 */
int cmd_run( argc, argv )
	int    argc;
	char  *argv[];
{
	FILE        *input_file    = NULL; /* Where input is coming from      */
	int          fn_optind;            /* argv index of filename argument */
	char        *swap_ptr;             /* Used for swapping strings       */
	int          exit_status;

	/*-- Variables settable by command line options --*/
	char        *file_name     = NULL;
	int          have_error    = False;

	/*-- Variables required by sqsh_getopt --*/
	int          c;
	extern int   sqsh_optind;
	extern char *sqsh_optarg;


	/*
	 * Open global environment transaction
	 */
	env_tran( g_env );

	/*
	 * Parse the command line options.
	 */
	while ((c = sqsh_getopt( argc, argv, "efhnpm:i:" )) != EOF)
	{
		switch (c)
		{
			case 'e' :
				if (env_put( g_env, "echo", "1", ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\run: -e: %s\n", sqsh_get_errstr() );
					have_error = True;
				}
				break;

			case 'f' :
				if (env_put( g_env, "footers", "0", ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\run: -f: %s\n", sqsh_get_errstr() );
					have_error = True;
				}
				break;

			case 'h' :
				if (env_put( g_env, "headers", "0", ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\run: -h: %s\n", sqsh_get_errstr() );
					have_error = True;
				}
				break;

			case 'n' :
				if (env_put( g_env, "expand", "0", ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\run: -n: %s\n", sqsh_get_errstr() );
					have_error = True;
				}
				break;

			case 'p' :
				if (env_put( g_env, "statistics", "1", ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\run: -p: %s\n", sqsh_get_errstr() );
					have_error = True;
				}
				break;

			case 'm' :
				if (env_put( g_env, "style", sqsh_optarg, ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\run: -m: %s\n", sqsh_get_errstr() );
					have_error = True;
				}
				break;

			case 'i' :
				file_name = sqsh_optarg;
				break;

			default :
				fprintf(stderr, "\\run: %s\n", sqsh_get_errstr() );
				have_error = True;
		}
	}

	/*
	 * Check that a file is provided.
	 */
	if( file_name == NULL || have_error )
       	{
		fprintf( stderr, "Use: \\run [-e] [-f] [-h] [-n] [-p] [-m style] -i filename [optional script parameters ...]\n" );
		fprintf( stderr, "     -e          Run the script file with echo on\n" );
		fprintf( stderr, "     -f          Suppress footers\n" );
		fprintf( stderr, "     -h          Suppress headers\n" );
		fprintf( stderr, "     -n          Disable SQL buffer variable expansion\n" );
		fprintf( stderr, "     -p          Report runtime statistics\n" );
		fprintf( stderr, "     -m style    Specify output style {bcp|csv|horiz|html|meta|none|pretty|vert}\n" );
		fprintf( stderr, "     -i filename SQL file to run\n" );
		env_rollback( g_env );
		return CMD_FAIL;
	}

	/*
	 * If there are any arguments left on the command line, they need to be put on the
	 * argument stack. Make sure that the new argv[0] on the stack points to the filename.
	 */
	for ( fn_optind = 0; fn_optind < argc && argv[fn_optind] != file_name; fn_optind++ );
	if (sqsh_optind-1 != fn_optind)
	{
		swap_ptr = argv[sqsh_optind-1];
		argv[sqsh_optind-1] = file_name;
		argv[fn_optind]     = swap_ptr;
	}
	g_func_args[g_func_nargs].argc = argc - sqsh_optind + 1;
	g_func_args[g_func_nargs].argv = &(argv[sqsh_optind-1]);
	g_func_nargs++;

	/*
	 * Open the file for input and make it stdin.
	 */
	if ((input_file = fopen( (char*) file_name, "r" )) == NULL)
	{
		fprintf( stderr, "\\run: %s: %s\n", (char*) file_name, strerror( errno ) );
		g_func_nargs--;
		env_rollback( g_env );
		return CMD_FAIL;
	}
	env_put ( g_env, "script", file_name, ENV_F_TRAN);
	sqsh_stdin_file( input_file );

	/*
	 * Make sure we have a valid connection.
	 */
	if ((jobset_run( g_jobset, "\\connect", &exit_status )) == -1 || exit_status == CMD_FAIL)
	{
		fprintf( stderr, "\\run: Unable to (re)connect\n" );
		sqsh_stdin_pop();
		fclose( input_file );
		g_func_nargs--;
		env_rollback( g_env );
		return CMD_FAIL;
	}

	/*
	 * Start processing the batch file. Ignore the return value.
	 */
	(void) cmd_input();

	/*
	 * Pop the argument stack.
	 * Reset stdin to what it used to be.
	 * Close the open batch file.
	 * Rollback the global environment to its original state.
	 */
	sqsh_stdin_pop();
	fclose( input_file );
	g_func_nargs--;
	env_rollback( g_env );

	/*
	 *                     Set appropriate return and exit values.
	 * If script used \exit, then $exit_value may be set and we need to assign this value to $?.
	 * If \return was used, then "$?" may already be set and we can leave it that way.
	 */
	env_get ( g_env, "exit_value", &swap_ptr );
	if (*swap_ptr != '0' )
	{
		env_set ( g_internal_env, "?",  swap_ptr );
		env_set ( g_env, "exit_value", "0" );
	}

	return CMD_LEAVEBUF;
}
