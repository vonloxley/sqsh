/*
** cmd_func.c - User defined functions
**
** Copyright (C) 1999 by Scott C. Gray
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, write to the Free Software
** Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
**
** You may contact the author :
**   e-mail:  gray@voicenet.com
**            grays@xtend-tech.com
**            gray@xenotropic.com
*/
#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include <sys/stat.h>
#include "sqsh_config.h"
#include "sqsh_global.h"
#include "sqsh_expand.h"
#include "sqsh_error.h"
#include "sqsh_varbuf.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "sqsh_job.h"
#include "sqsh_sig.h"
#include "sqsh_getopt.h"
#include "sqsh_buf.h"
#include "sqsh_stdin.h"
#include "sqsh_readline.h"
#include "cmd.h"
#include "dsp.h"
#include "cmd_misc.h"
#include "cmd_input.h"

/*
** cmd_func():
**
** Define a sqsh function.
*/
int cmd_func( argc, argv )
	int     argc;
	char   *argv[];
{
	char        *func_name;
	varbuf_t    *input_buf;
	int          ret;
	/* extern char *sqsh_optarg; */
	extern int   sqsh_optind;
	int          ch;
	int          do_export = False;
	int          have_error = False;

	while ((ch = sqsh_getopt( argc, argv, "x" )) != EOF) 
	{
		switch (ch) 
		{
			case 'x' :
				do_export = True;
				break;
			default :
				fprintf( stderr, "\\func: -%c: Invalid option\n",
					(int)ch );
				have_error = True;
		}
	}

	if ((argc - sqsh_optind) != 1)
	{
		fprintf( stderr, "use: \\func [-x] <name>\n" );
		fprintf( stderr, "         <body>\n" );
		fprintf( stderr, "     \\done\n" );
		return(CMD_FAIL);
	}

	func_name = argv[sqsh_optind];

	/*
	** Allocate a buffer for the input.
	*/
	if ((input_buf = varbuf_create( 512 )) == NULL)
	{
		fprintf( stderr, "\\func: Memory allocation failure\n" );
		return(CMD_FAIL);
	}

	env_tran(g_env);

	/*
	** Get the body of the function. Note that this function
	** is defined in cmd_do.c.
	*/
	if ((ret = cmd_body_input( input_buf )
		!= CMD_CLEARBUF))
	{
		varbuf_destroy( input_buf );
		env_rollback(g_env);
		return(ret);
	}

	env_rollback(g_env);

	/*
	** Attempt to add the new function in the table.
	*/
	if (funcset_add( g_funcset, func_name, varbuf_getstr(input_buf) )
		== False)
	{
		fprintf( stderr, "\\func: %s\n", sqsh_get_errstr() );
		varbuf_destroy( input_buf );
		return(CMD_FAIL);
	}

	/*
	** If we are to export this function, then we want to
	** also create it as a command.
	*/
	if (do_export == True)
	{
		if (cmdset_add( g_cmdset, func_name, cmd_call )
			== False)
		{
			fprintf( stderr, "\\func: Error exporting %s: %s\n",
				func_name, sqsh_get_errstr() );
		}
	}

	varbuf_destroy( input_buf );
	return(CMD_CLEARBUF);
}

/*
** cmd_call():
**
** Call function previously created with cmd_func().
*/
int cmd_call( argc, argv )
	int     argc;
	char   **argv;
{
	func_t   *f;
	char     *func_name;
	int       ret;

	/*
	** If the name of the command run was \call, then the first
	** argument must be the name of the function.
	*/
	if (strcmp( argv[0], "\\call" ) == 0)
	{
		if (argc < 2)
		{
			fprintf( stderr, "Use: \\call <func_name> [args ...]\n" );
		}

		func_name = argv[1];

		argv = &(argv[1]);
		--argc;
	}
	else
	{
		func_name = argv[0];
	}

	/*
	** Try to track down the function.
	*/
	f = funcset_get( g_funcset, func_name );

	if (f == NULL)
	{
		fprintf( stderr, "\\call: Erro calling %s: %s\n",
			(char*)func_name, sqsh_get_errstr() );
		return(CMD_FAIL);
	}

	/*
	** Now, push our arguments onto the stack of current arguments
	** to functions.  These will be referenced by sqsh_expand()
	** when expanding $[0-9] values.
	*/
	g_func_args[g_func_nargs].argc = argc;
	g_func_args[g_func_nargs].argv = argv;
	++g_func_nargs;

	/*
	** Set the current stdin to be the body of our function.
	*/
	sqsh_stdin_buffer( f->func_body, -1 );

	/*
	** The default return value is 0.
	*/
	env_set( g_internal_env, "?", "0" );

	/*
	** Now, ship the current command body off to cmd_input() to
	** let it take care of the rest.
	*/
	ret = cmd_input();

	/*
	** If a return was issued, then don't pass it on up.
	*/
	if (ret == CMD_RETURN)
	{
		ret = CMD_LEAVEBUF;
	}

	/*
	** Restore stdin.
	*/
	sqsh_stdin_pop();

	/*
	** Pop our arguments off the stack.
	*/
	--g_func_nargs;

	/*
	** And return.
	*/
	return(ret);
}


