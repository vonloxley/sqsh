/*
** cmd_while.c - Looping
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
#include "sqsh_tok.h"
#include "sqsh_args.h"
#include "cmd.h"
#include "dsp.h"
#include "cmd_misc.h"
#include "cmd_input.h"

/*
** Prototypes.
*/
static int  cmd_while_exec    _ANSI_ARGS(( char*, char*, varbuf_t* ));
static void while_sigint_flag _ANSI_ARGS(( int, void* ));

/*
** sg_got_sigint: Flag to be set to true upon receipt of SIGINT.
*/
int sg_got_sigint = False;

/*
** cmd_while():
**
** Perform conditional loop.  Note that \while is a very
** special case in that argv[1] will always be the entire
** command line.
*/
int cmd_while( argc, argv )
	int     argc;
	char   *argv[];
{
	varbuf_t  *while_buf;
	varbuf_t  *expand_buf;
	int        ret;


	sg_got_sigint = False;

	/*
	** Save away current signal handler set, and install
	** a new sigint handler. We do this now to make sure
	** that we don't return without feeing memory.
	*/
	sig_save();
	sig_install( SIGINT, while_sigint_flag, (void*)NULL, 0 );

	/*
	** Allocate a buffer for the input.
	*/
	if ((while_buf = varbuf_create( 512 )) == NULL)
	{
		fprintf( stderr, "\\func: Memory allocation failure\n" );

		sig_restore();
		return(CMD_FAIL);
	}

	/*
	** Allocate a buffer to expand the command line.
	*/
	if ((expand_buf = varbuf_create( 128 )) == NULL)
	{
		fprintf( stderr, "\\func: Memory allocation failure\n" );
		varbuf_destroy( while_buf );

		sig_restore();
		return(CMD_FAIL);
	}

	env_tran(g_env);

	/*
	** Read the body of the command up to the \done.
	*/
	if ((ret = cmd_body_input( while_buf )
		!= CMD_CLEARBUF))
	{
		varbuf_destroy( while_buf );
		varbuf_destroy( expand_buf );
		env_rollback(g_env);
		sig_restore();
		return(ret);
	}

	env_rollback(g_env);

	/*
	** Do the while loop.
	*/
	ret = cmd_while_exec( argv[1], varbuf_getstr(while_buf), expand_buf );

	/*
	** Destroy the buffers.
	*/
	varbuf_destroy( while_buf );
	varbuf_destroy( expand_buf );

	sig_restore();

	/*
	** And return.
	*/
	return(ret);
}

static int cmd_while_exec( while_expr, while_body, expand_buf )
	char     *while_expr;
	char     *while_body;
	varbuf_t *expand_buf;
{
	args_t    *args;
	int        ret;
	int        exit_status;
	tok_t     *tok;

	do
	{
		if (sg_got_sigint == True)
		{
			return(CMD_INTERRUPTED);
		}

		/*
		** The \while loop must expand its command line during
		** each iteration, then parse the results, then perform
		** the test.
		*/

		/*
		** First, expand the command line.
		*/
		if (sqsh_expand( while_expr, expand_buf, EXP_COLUMNS ) == False)
		{
			fprintf( stderr, "\\while: Unable to expand command line: %s\n",
				sqsh_get_errstr() );
			return(CMD_FAIL);
		}

		args = args_create( 16 );

		if (args == NULL)
		{
			fprintf( stderr, "\\while: Memory allocation failure\n" );
			return(CMD_FAIL);
		}

		/*
		** Perform tokenization of command line.
		*/
		if (sqsh_tok( varbuf_getstr(expand_buf), &tok, (int)0 ) == False)
		{
			fprintf( stderr, "\\while: Error tokenizing command line: %s\n",
				sqsh_get_errstr() );
			args_destroy( args );
			return(CMD_FAIL);
		}

		while (tok->tok_type != SQSH_T_EOF)
		{
			if (args_add( args, sqsh_tok_value(tok) ) == False)
			{
				fprintf( stderr, "\\while: Error adding to argument list: %s\n",
					sqsh_get_errstr() );
				args_destroy( args );
				return(CMD_FAIL);
			}

			if (sqsh_tok( NULL, &tok, (int)0 ) == False)
			{
				fprintf( stderr, "\\while: Error tokenizing command line: %s\n",
					sqsh_get_errstr() );
				args_destroy( args );
				return(CMD_FAIL);
			}
		}

		/*
		** Now, execute the while expression. Note that both \if and \while
		** share the same function to do this.
		*/
		if ((ret = cmd_if_exec( args->a_argc - 1, &(args->a_argv[1]),
			&exit_status )) == CMD_FAIL)
		{
			args_destroy( args );
			return(ret);
		}
		args_destroy( args );

		/*
		** Upon success, execute the contents of the body.
		*/
		if (exit_status == 0)
		{
			/*
			** Redirect the stdin to the new buffer.
			*/
			sqsh_stdin_buffer( while_body, -1 );

			/*
			** And execute it.
			*/
			ret = cmd_input();

			/*
			** Restore stdin.
			*/
			sqsh_stdin_pop();

			if (ret == CMD_EXIT || 
				ret == CMD_ABORT ||
				ret == CMD_RETURN)
			{
				return(ret);
			}

			if (ret == CMD_BREAK)
			{
				return(CMD_LEAVEBUF);
			}
		}
	}
	while (exit_status == 0);

	return(CMD_LEAVEBUF);
}

static void while_sigint_flag( sig, data )
	int   sig;
	void *data;
{
	sg_got_sigint = True;
}
