/*
 * cmd_shell.c - Execute user command in sub-shell.
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
#include "sqsh_varbuf.h"
#include "sqsh_global.h"
#include "sqsh_cmd.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_shell.c,v 1.2 2014/02/03 10:49:28 mwesdorp Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_shell:
 */
int cmd_shell( argc, argv )
	int    argc;
	char  *argv[];
{
	varbuf_t   *shell_cmd;
	char       *shell;
	char       exit_status_str[10];
	int        exit_status;
	int        i;
	int        r;

	if ((shell_cmd = varbuf_create(64)) == NULL)
	{
		fprintf(stderr, "varbuf_create: %s\n", sqsh_get_errstr() );
		return CMD_FAIL;
	}

	r = 0;
	if (argc == 1)
	{
		env_get( g_env, "SHELL", &shell );
		if (shell == NULL || *shell == '\0')
		{
			shell = SQSH_SHELL;
		}

		r = varbuf_strcat( shell_cmd, shell );
	}
	else
	{
		for ( i = 1; i < argc; i++ )
		{
			if ((r = varbuf_strcat( shell_cmd, argv[i] )) == -1)
			{
				break;
			}

			if ((r = varbuf_charcat( shell_cmd, ' ' )) == -1)
			{
				break;
			}
		}
	}

	if (r == -1)
	{
		fprintf( stderr, "varbuf: %s\n", sqsh_get_errstr() );
		varbuf_destroy( shell_cmd );
		return CMD_FAIL;
	}

	/*
	 * Believe it or not, these two fflush()'s are very important. Since
	 * the child process is expected to perform some sort of output prior
	 * to returning, we want to make sure whatever we have buffered
	 * is written prior to the child's output.
	 */
	fflush( stdout );
	fflush( stderr );

	exit_status = system( varbuf_getstr(shell_cmd) );
	varbuf_destroy( shell_cmd );

	/*
	 * Store the exit status of this command in our "internal" environment.
	 */
	sprintf( exit_status_str, "%d", exit_status );
	env_set( g_internal_env, "?", exit_status_str );

	return CMD_LEAVEBUF;
}

/*
 * cmd_lcd: Change local working directory.
 * sqsh-2.5 - New feature.
 */
int cmd_lcd( argc, argv )
	int    argc;
	char  *argv[];
{
	int         ret;              /* Return value                     */
	char        tmp[512] = "";    /* Tmp storage area                 */
	static char owd[512] = "";    /* Old working directory (previous) */
	char       *sp;               /* String pointer                   */


	if (argc != 2)
	{
		fprintf( stderr, "Use: \\lcd dir_name\n" );
		env_set ( g_internal_env, "?", "1" );
		return CMD_FAIL;
	}

	sp = getcwd ( tmp, sizeof(tmp) );
	sp = ( strcmp ( argv[1], "-" ) == 0 && owd[0] != '\0' ) ? owd : argv[1];
	ret = chdir ( sp );
	if (ret != 0)
	{
		fprintf( stderr, "\\lcd: %s: %s\n", (char*) sp, strerror( errno ) );
		env_set ( g_internal_env, "?", "1" );
		return CMD_FAIL;
	}
	else
		fprintf( stdout, "\\lcd: local directory changed to: %s\n", sp );

	strcpy (owd, tmp);
	env_set ( g_internal_env, "?", "0" );
	return CMD_LEAVEBUF;
}

/*
 * cmd_pwd: Print current working directory.
 * sqsh-2.5 - New feature.
 */
int cmd_pwd( argc, argv )
	int    argc;
	char  *argv[];
{
	char pwd[512];

	if (argc != 1)
	{
		fprintf( stderr, "Use: \\pwd\n" );
		env_set ( g_internal_env, "?", "1" );
		return CMD_FAIL;
	}

	fprintf( stdout, "%s\n", getcwd( pwd, sizeof(pwd) ) );
	env_set ( g_internal_env, "?", "0" );
	return CMD_LEAVEBUF;
}

/*
 * cmd_ls: Show files in current directory.
 * sqsh-2.5 - New feature.
 */
int cmd_ls( argc, argv )
	int    argc;
	char  *argv[];
{
	char       return_value[10];
	int        exit_status;

	if (argc != 1)
	{
		fprintf( stderr, "Use: \\ls\n" );
		env_set ( g_internal_env, "?", "1" );
		return CMD_FAIL;
	}

	exit_status = system ( "ls" );
	sprintf (return_value, "%d", exit_status);
	env_set ( g_internal_env, "?", return_value );
	return CMD_LEAVEBUF;
}
