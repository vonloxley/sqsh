/*
 * cmd_set.c - User command to set environment variables
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
#include <stdlib.h>
#include <ctype.h>
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_global.h"
#include "sqsh_varbuf.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_set.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_set:
 */
int cmd_set( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	int     export = False;
	char   *var_name ;
	char   *var_value ;
	char   *cptr ;
	int     i ;
	var_t  *v ;

	/*
	 * If there were no arguments supplied, then simply dump the
	 * output of all of the variables in the system.
	 */
	if (argc == 1)
	{
		for (i = 0; i < g_env->env_hsize; i++)
		{
			for (v = g_env->env_htable[i]; v != NULL; v = v->var_nxt)
			{
				printf( "%s=%s\n", v->var_name, 
				        (v->var_value == NULL) ? "NULL" : v->var_value );
			}
		}
		return CMD_LEAVEBUF;
	}

	/*
	 * If the first argument is a -x, then we want to export the
	 * variables as we go along.
	 */
	if (strcmp( argv[1], "-x" ) == 0)
	{
		i = 2;
		export = True;
	}
	else
	{
		i = 1;
	}

	/*
	 * Now, process any variables on the command line.
	 */
	for (; i < argc; i++)
	{
		/*
		 * Attempt to track down the '=' in the variable
		 * assignment.  If there isn't one, then we have
		 * an error.
		 */
		cptr = strchr( argv[i], '=' );

		if (cptr == NULL || *cptr != '=')
		{
			fprintf( stderr, "\\set: Expected '=' in '%s'\n", argv[i] );
			return CMD_FAIL;
		}
		*cptr = '\0';

		var_name  = argv[i];
		var_value = cptr + 1;

		/*
		 * Just for grins, lets make sure that the variable name
		 * contains allowable characters.  It probably wouldn't
		 * kill us to let bad ones through, but hey...
		 */
		for (cptr = var_name; *cptr != '\0'; ++cptr)
		{
			if (!(*cptr == '_' || isalnum((int)*cptr)))
			{
				fprintf( stderr, "%s: Invalid variable name\n", var_name );
				*(var_value - 1) = '=';
				return CMD_FAIL ;
			}
		}

		/*
		 * Attempt to set the internal sqsh variable to the value
		 * supplied.  If we can't, then give up.
		 */
		if (env_set( g_env, var_name, var_value ) == False)
		{
			fprintf( stderr, "%s: %s\n", var_name, sqsh_get_errstr() );
			*(var_value - 1) = '=';
			return CMD_FAIL;
		}

		/*
		 * If the user asked to have the variable exported, then
		 * we want to call putenv() to push it out to any programs
		 * that may be run by sqsh.
		 */
		if (export == True)
		{
			sqsh_setenv( var_name, var_value );
		}

		*(var_value - 1) = '=';
	}

	return CMD_LEAVEBUF ;
}
