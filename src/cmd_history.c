/*
 * cmd_history.c - User command to display buffer history
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
#include "sqsh_global.h"
#include "sqsh_history.h"
#include "sqsh_cmd.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_history.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_history:
 */
int cmd_history( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	hisbuf_t   *hb ;
	char       *line ;
	char       *nl ;

	/*-- Check the arguments --*/
	if( argc != 1 ) {
		fprintf( stderr, "Use: \\history\n" ) ;
		return CMD_FAIL ;
	}

	/*
	 * Since we want to print our history from oldest to newest we
	 * will traverse the list backwards. Note, I don't like having
	 * cmd_history() play with the internals of the history structure,
	 * but it makes life much easier.
	 */
	for( hb = g_history->h_end; hb != NULL; hb = hb->hb_prv ) {

		line = hb->hb_buf ;
		while( (nl = strchr( line, '\n' )) != NULL ) {
			if( line == hb->hb_buf ) {
				printf( "(%d) %*.*s\n", hb->hb_nbr, nl - line, nl - line, line ) ;
			} else {
				printf( "     %*.*s\n", nl - line, nl - line, line ) ;
			}

			line = nl + 1 ;
		}

		if( *line != '\0' )
			printf( "     %s\n", line ) ;

	}

	return CMD_LEAVEBUF ;
}
