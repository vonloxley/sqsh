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
#include "sqsh_cmd.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_exit.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
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
	 * This is probably unnecessary, but could save your ass if
	 * you didn't mean to type exit.
	 */
	if( argc != 1 ) {
		fprintf( stderr, "Too many arguments to exit" ) ;
		return CMD_FAIL ;
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
		fprintf( stderr, "Too many arguments to abort" ) ;
		return CMD_FAIL ;
	}

	return CMD_ABORT ;
}
