/*
 * cmd_alias.c - Sqsh command wrappers around cmdset alias functions
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
#include "sqsh_cmd.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_alias.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_alias:
 */
int cmd_alias( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	char         *cptr ;
	aliasnode_t  *a ;
	int           widest = 0 ;
	int           len ;

	if( argc > 2 ) {
		fprintf( stderr, "Use: \\alias [alias=command]\n" ) ;
		return CMD_FAIL ;
	}

	if( argc == 1 ) {
		for( a = (aliasnode_t*)avl_first( g_alias->alias_entries ) ;
		     a != NULL ;
			  a = (aliasnode_t*)avl_next( g_alias->alias_entries ) ) {
			len = strlen( a->an_name ) ;
			widest = max(widest, len) ;
		}

		for( a = (aliasnode_t*)avl_first( g_alias->alias_entries ) ;
		     a != NULL ;
			  a = (aliasnode_t*)avl_next( g_alias->alias_entries ) ) {
			printf("%-*s = %s\n", widest, a->an_name, a->an_body ) ;
		}
		return CMD_LEAVEBUF ;
	}

	if( (cptr = strchr(argv[1], '=')) == NULL ) {
		fprintf( stderr, "Use: \\alias [alias=command]\n" ) ;
		return CMD_FAIL ;
	}

	*cptr = '\0' ;
	if( alias_add( g_alias, argv[1], cptr+1 ) == False ) {
		fprintf( stderr, "\\alias: %s\n", sqsh_get_errstr() ) ;
		*cptr = '=' ;
		return CMD_FAIL ;
	}

	*cptr = '=' ;
	return CMD_LEAVEBUF ;
}

/*
 * cmd_unalias:
 */
int cmd_unalias( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	if( argc != 2 ) {
		fprintf( stderr, "Use: \\unalias alias\n" ) ;
		return CMD_FAIL ;
	}

	if( alias_remove( g_alias, argv[1] ) == False ) {
		fprintf( stderr, "\\unalias: %s\n", sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	return CMD_LEAVEBUF ;
}
