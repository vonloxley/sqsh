/*
 * cmd_reset.c - User command to clear the current work buffer.
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
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_reset.c,v 1.1.1.1 2004/04/07 12:35:04 chunkm0nkey Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_clear:
 *
 * sqsh-2.1.7 - Clears the current g_sqlbuf without saving the buffer
 * in the history.
 */
int cmd_clear( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	if( argc != 1 ) {
		fprintf( stderr, "Too many arguments to \\clear\n" ) ;
		return CMD_FAIL ;
	}
#if defined(USE_READLINE)
	if (g_interactive)
	        _rl_clear_screen();
#endif
	return CMD_CLEARBUF ;
}

/*
 * cmd_reset:
 *
 * Clears the current g_sqlbuf.
 */
int cmd_reset( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	if( argc != 1 ) {
		fprintf( stderr, "Too many arguments to \\reset\n" ) ;
		return CMD_FAIL ;
	}

	return CMD_RESETBUF ;
}

