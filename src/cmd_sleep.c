/*
 * cmd_sleep.c - User command to sleep for specified # of seconds
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
static char RCS_Id[] = "$Id: cmd_sleep.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_sleep:
 */
int cmd_sleep( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	int  seconds ;

	/*
	 * This is probably unnecessary, but could save your ass if
	 * you didn't mean to type exit.
	 */
	if( argc != 2 ) {
		fprintf( stderr, "Use: \\sleep [seconds]\n" ) ;
		return CMD_FAIL ;
	}

	seconds = atoi(argv[1]) ;
	if( seconds <= 0 ) {
		fprintf( stderr, "\\sleep: Invalid number of seconds %d\n", seconds ) ;
		return CMD_FAIL ;
	}

	sleep(seconds) ;
	return CMD_LEAVEBUF ;
}
