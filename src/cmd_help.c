/*
 * cmd_help.c - User command to display help information
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
#include <sys/param.h>
#include "sqsh_config.h"
#include "sqsh_global.h"
#include "sqsh_error.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "cmd_help.c,v 1.1.1.1 1996/02/14 02:36:43 gray Exp" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_help:
 */
int cmd_help( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	FILE    *fp ;
	char     help_file[SQSH_MAXPATH+1] ;
	char     help_line[512] ;
	char    *help_dir ;
	cmd_t   *c ;
	int      max_len ;
	int      len ;
	int      nperline ;
	char    *width ;
	int      count ;

	/*-- Always check your arguments --*/
	if( argc > 2 ) {
		fprintf( stderr, "Use: \\help [command]\n" ) ;
		return CMD_FAIL ;
	}

	/*
	 * If the user requests help on a particular command then we need
	 * to see if there is a help file on the command.
	 */
	if( argc == 2 ) {
		/*
		 * First ast the environment prior to falling back on the hard
		 * coded default.
		 */
		env_get( g_env, "help_dir", &help_dir ) ;
		if( help_dir == NULL || *help_dir == '\0' )
			help_dir = SQSH_HELP ;

		printf(
			"\\help: Sorry, individual help pages haven't been written yet.\n" ) ;
		printf(
			"\\help: Please refer to the manual page for more information.\n" ) ;

		return CMD_LEAVEBUF ;

		/*
		 * First, check to see if the help file actually exists.  If it
		 * doesn't then we are done.
		 */
		sprintf( help_file, "%s/%s.1", help_dir, argv[1] ) ;

		if( (fp = fopen( help_file, "r" )) == NULL ) {
			fprintf( stderr, "\\help: Unable to access %s: %s\n",
			         help_file, strerror(errno) ) ;
			return CMD_FAIL ;
		}

		/*
		 * Now, simply read lines from the help file and display them
		 * to the screen. It's as simple as that!
		 */
	 	while( fgets( help_line, sizeof(help_line), fp ) != NULL )
	 		fputs( help_line, stdout ) ;
		fclose( fp ) ;

		return CMD_LEAVEBUF ;
	}

	/*
	 * Nope, the user just asked for general help on the commands.
	 */
 	printf( "Available commands:\n" ) ;

	/*
	 * First, determine the length of the longest command.
	 */
 	max_len = 1 ;
	for( c = (cmd_t*)avl_first( g_cmdset->cs_cmd ) ;
	     c != NULL ;
		  c = (cmd_t*)avl_next( g_cmdset->cs_cmd ) ) {
 		len = strlen( c->cmd_name ) ;
 		max_len = max( max_len, len ) ;
	}

	/*
	 * Now, print the commands out in the prettiest fashion possible.
	 */
 	env_get( g_env, "width", &width ) ;

 	if( width == NULL )
 		len = 80 ;
	else {
		len = atoi( width ) ;
		if( len < 40 )
			len = 40 ;
	}

 	nperline = len / (max_len + 2) ;
 	count = 1 ;
	for( c = (cmd_t*)avl_first( g_cmdset->cs_cmd ) ;
	     c != NULL ;
		  c = (cmd_t*)avl_next( g_cmdset->cs_cmd ) ) {

		if( (count % nperline) == 0 )
			printf( "%-*s\n", max_len, c->cmd_name ) ;
		else
			printf( "%-*s  ", max_len, c->cmd_name ) ;
		++count ;
	}

	if( (count % nperline) == 0 )
		printf( "\n" ) ;

	printf("\nUse '\\help [command]' for more details\n" ) ;

	return CMD_LEAVEBUF ;
}
