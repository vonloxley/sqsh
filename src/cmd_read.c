/*
 * cmd_read.c - Read a line of input from a user
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
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "sqsh_getopt.h"
#include "sqsh_readline.h"
#include "sqsh_stdin.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_read.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_read:
 *
 * Request input from a user, placing the contents into a variable.
 */
int cmd_read( argc, argv )
	int    argc;
	char  *argv[];
{
	extern int sqsh_optind;
	char   *cptr;
	char   *var_name;
	char   *var_value;
	int     ch;
	char   *str = NULL;
	char    input[512];
	int     r;

	int  append       = False;
	int  keep_newline = False;
	int  have_error   = False;
	int  hide_output  = False;

	while ((ch = sqsh_getopt( argc, argv, "nah" )) != EOF)
	{
		switch (ch)
		{
			case 'h' :
				hide_output = True;
				break;
			case 'a' :
				append = True;
				break;
			case 'n' :
				keep_newline = True;
				break;
			default :
				fprintf( stderr, "\\read: %s\n", sqsh_get_errstr() );
				have_error = True;
		}
	}

	/*
	 * There should only be on more argument left on the command line,
	 * so if there are more, or if an error was found above, then
	 * print out a usage message.
	 */
	if( have_error || (argc - sqsh_optind) != 1 ) {
		fprintf( stderr, "Use: \\read [-n] [-a] [-h] var_name\n" );
		return CMD_FAIL;
	}

	var_name = argv[sqsh_optind];

	if (hide_output == True)
	{
		r = sqsh_getinput( "", input, sizeof(input), 0  );
		if (r < 0)
		{
			if (r == -1)
			{
				fprintf( stderr, "\\read: %s\n", sqsh_get_errstr() );
			}
			return CMD_FAIL;
		}
	}
	else
	{
		if (sqsh_stdin_fgets( input, sizeof(input) ) == NULL)
		{
			fprintf( stderr, "\\read: %s\n", strerror(errno) );
			return CMD_FAIL;
		}

		str = strchr( input, '\n' );

		if (str != NULL)
		{
			str = '\0';
		}
	}

	str = input;

	/*
	 * Unless the user requests otherwise, strip the new-line from
	 * the string.
	 */
	if (keep_newline == False)
	{
		if ((cptr = strchr( str, '\n' )) != NULL)
			*cptr = '\0';
	}

	/*
	 * If the user didn't type anything, then there is no reason
	 * to keep on processing.
	 */
	if (*str == '\0')
		return CMD_LEAVEBUF;

	/*
	 * Unless the user requests otherwise, replace the current contents
	 * of the variable with what the user typed in.
	 */
	if (append == False)
	{
		if (env_set( g_env, var_name, str ) == False)
		{
			fprintf( stderr, "\\read: %s: %s\n", var_name, sqsh_get_errstr() );
			return CMD_FAIL;
		}
		return CMD_LEAVEBUF;
	}

	/*
	 * Retrieve the current value of the variable.  If the variable doesn't
	 * exist, then we perform a normal set.
	 */
	if (env_get( g_env, var_name, &var_value) == 0 ||
	    var_value == NULL )
	{
		if (env_set( g_env, var_name, str ) == False)
		{
			fprintf( stderr, "\\read: %s: %s\n", var_name, sqsh_get_errstr() );
			return CMD_FAIL;
		}
		return CMD_LEAVEBUF;
	}

	/*
	 * Create a buffer large enough to hold the current contents of the
	 * variable plus the new string the user typed.
	 */
	cptr = (char*)malloc( (strlen(var_value)+strlen(str)+1) * sizeof(char) );
	if (cptr == NULL)
	{
		fprintf( stderr, "\\read: Memory allocation failure\n" );
		return CMD_FAIL;
	}

	sprintf( cptr, "%s%s", var_value, str );
	if (env_set( g_env, var_name, cptr ) == False)
	{
		fprintf( stderr, "\\read: %s: %s\n", var_name, sqsh_get_errstr() );
		free( cptr );
		return CMD_FAIL;
	}

	free( cptr );
	return CMD_LEAVEBUF;
}
