/*
** cmd_for.c - Loopy logic.
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
#include "sqsh_buf.h"
#include "sqsh_readline.h"
#include "sqsh_getopt.h"
#include "sqsh_stdin.h"
#include "cmd.h"
#include "dsp.h"
#include "cmd_misc.h"
#include "cmd_input.h"

int cmd_for( argc, argv )
	int     argc;
	char   *argv[];
{
	varbuf_t         *for_buf;
	int               ret;
	int               exit_status;
	char             *var_name;
	int               i;

	/*
	** Since we will be temporarily replacing some of our global
	** settings, we want to set up a save-point to which we can
	** restore when we are done.
	*/
	env_tran( g_env );

	/*
	** Before we go any further, read the reaminder of the input
	** from the user (up to \done).
	*/
	for_buf = varbuf_create( 512 );

	if ((ret = cmd_body_input( for_buf )) != CMD_CLEARBUF)
	{
		varbuf_destroy( for_buf );
		env_rollback( g_env );
		return(ret);
	}

	/*
	** Don't need to protect things any more.
	*/
	env_rollback( g_env );

	if (argc < 4 || strcmp(argv[2], "in") != 0)
	{
		fprintf( stderr, "use: \\for <variable> in <word> [<word> ...]\n" );
		varbuf_destroy( for_buf );
		return(CMD_FAIL);
	}

	var_name = argv[1];

	for (i = 3; i < argc; i++)
	{
		/*
		** Set the variable to the current argument.
		*/
		env_set( g_env, var_name, argv[i] );

		/*
		** Redirect stdin to the buffer, then process it. 
		*/
		sqsh_stdin_buffer( varbuf_getstr( for_buf ), -1 );
			ret = cmd_input();
		sqsh_stdin_pop();

		if (ret == CMD_FAIL || 
			ret == CMD_ABORT ||
			ret == CMD_BREAK ||
			ret == CMD_RETURN)
		{
			varbuf_destroy( for_buf );
			return(ret);
		}
	}

	varbuf_destroy( for_buf );
	return(CMD_LEAVEBUF);
}
