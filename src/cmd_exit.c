/*
 * cmd_exit.c - User command to leave current read-eval-print loop
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
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_exit.c,v 1.1.1.1 2004/04/07 12:35:02 chunkm0nkey Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_exit:
 *
 * Returns CMD_EXIT to the current read-eval-print loop (see cmd_loop.c).
 */
int cmd_exit( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	/*
	 * sqsh-2.1.7 - Feature to provide for an exit code.
	 *
	 * \exit [n]: n is a positive integer value that is used as a return code
	 * to the shell. It is your own responsibility to use return codes that
	 * your shell can deal with, e.g. 0 <= n <= 255.
	 * The value will be saved in the exit_value sqsh environment variable. At the
	 * time of termination the value will be retrieved and used in sqsh_exit().
	 */
	if( argc > 2 ) {
		fprintf( stderr, "\\exit: Too many arguments; Use: \\exit [n]\n" ) ;
		return CMD_FAIL ;
	}

	if( argc == 2 ) {
		if (env_set (g_env, "exit_value", argv[1]) != True)
		{
			fprintf( stderr, "sqsh: \\exit [n]: Invalid argument (%s)\n", argv[1] ) ;
			return CMD_FAIL ;
		}
	}

	return CMD_EXIT ;
}

/*
 * cmd_abort:
 *
 * Returns CMD_ABORT to the current read-eval-print loop (see cmd_loop.c).
 */
int cmd_abort( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	/*
	 * This is probably unnecessary, but could save your ass if
	 * you didn't mean to type abort.
	 */
	if( argc != 1 ) {
		fprintf( stderr, "\\abort: Too many arguments; Use: \\abort\n" ) ;
		return CMD_FAIL ;
	}

	return CMD_ABORT ;
}
