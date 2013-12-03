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
#include "sqsh_getopt.h"
#include "sqsh_global.h"
#include "sqsh_history.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "cmd.h"
#include "sqsh_expand.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_history.c,v 1.8 2013/05/07 21:18:02 mwesdorp Exp $" ;
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
	/*
	 * sqsh-2.1.7: Feature to display extended buffer info like datetime
	 * of last buffer access and buffer usage count.
	 * Show only most recent number of history buffers by request.
	*/
	extern int        sqsh_optind;          /* Required by sqsh_getopt */
	extern char*      sqsh_optarg;          /* Required by sqsh_getopt */
	struct tm  *ts;
	int         shownum;
	int         ch;
	int         show_info  = False;
	int         have_error = False;
	char        dttm[32];
	char        hdrinfo[64];
	char       *datetime   = NULL;
	char       *cp;
	char        fmt[64];


	/*
	 * Initialize number of history buffers to show to the total number of
	 * available buffers.
	*/
	shownum = g_history->h_nitems;

	/*
	 * Check the arguments
	*/
	while ((ch = sqsh_getopt( argc, argv, "ix:" )) != EOF)
	{
		switch (ch)
		{
			case 'i' :
				show_info = True;
				break;

			case 'x' :
				if ((shownum = atoi(sqsh_optarg)) <= 0)
				{
					fprintf( stderr, "\\history: Invalid value for option -x (%s)\n", sqsh_optarg );
					have_error = True;
				}
				break;

			default :
				fprintf( stderr, "\\history: %s\n", sqsh_get_errstr() );
				have_error = True;
		}
	}

	/*
	 * If there are any errors on the command line, or there are
	 * any options left over then we have an error.
	*/
	if( (argc - sqsh_optind) > 0 || have_error)
	{
		fprintf( stderr,
			"Use: \\history [-i] [-x number]\n"
			"     -i         Request additional history buffer information\n"
			"     -x number  Show most recent number of history buffers\n" );

		return CMD_FAIL;
	}

	/*
	 * sqsh-2.2.0 - Since the datetime format string may contain [] to filter out seconds for
	 * smalldatetime datatypes, we have to remove these brackets here. Also replace the %u format
	 * specifier with 000 when specified in the format string.
	 * sqsh-2.5: Strip of .%q and [] from the datetime format string.
	 */
	if (show_info == True)
	{
		env_get( g_env, "datetime", &datetime);
		if (datetime == NULL || *datetime == '\0' || (strcmp(datetime,"default") == 0))
			strcpy (fmt, "%Y%m%d %H:%M:%S");
		else
		{
			for (cp = fmt; *datetime != '\0'; datetime++)
			{
				if (*datetime == '.' && *(datetime+1) == '%' && *(datetime+2) == 'q')
				{
					datetime += 2;
				}
				else if (*datetime != '[' && *datetime != ']')
					*cp++ = *datetime;
			}
			*cp = '\0';
		}
	}

	/*
	 * Since we want to print our history from oldest to newest we
	 * will traverse the list backwards. Note, I don't like having
	 * cmd_history() play with the internals of the history structure,
	 * but it makes life much easier.
	*/
	if ( shownum < g_history->h_nitems )
		for( hb = g_history->h_start; hb != NULL && shownum > 1; hb = hb->hb_nxt, shownum--);
	else
		hb = g_history->h_end;

	for( ; hb != NULL; hb = hb->hb_prv ) {

		line = hb->hb_buf ;
		while( (nl = strchr( line, '\n' )) != NULL ) {
			if( line == hb->hb_buf ) {
				if (show_info == True)
				{
					ts  = localtime( &hb->hb_dttm );
					strftime( dttm, sizeof(dttm), fmt, ts );
					sprintf( hdrinfo, "(%2d - %2d/%s) ",
						hb->hb_nbr, hb->hb_count, dttm ) ;
				}
				else
					sprintf( hdrinfo, "(%d) ", hb->hb_nbr ) ;

				printf( "%s%*.*s\n", hdrinfo, (int) (nl - line), (int) (nl - line), line ) ;

			} else {
				printf( "%*s%*.*s\n", (int) strlen(hdrinfo), " ", (int) (nl - line), (int) (nl - line), line ) ;
			}

			line = nl + 1 ;
		}

		if( *line != '\0' )
			printf( "     %s\n", line ) ;

	}
	return CMD_LEAVEBUF ;
}


/*
 * cmd_hist_load:
 *
 * Load the history from a file.
 *
 */
int cmd_hist_load( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	char       str[16];
	char      *history;
	varbuf_t  *exp_buf;


	/*
	 * Only one argument allowed.
	 */
	if( argc > 2 ) {
		fprintf( stderr, "\\hist-load: Too many arguments; Use: \\hist-load [filename]\n" ) ;
		return CMD_FAIL ;
	}

	if (argc == 2)
		history = argv[1];
	else
		env_get( g_env, "history", &history );

	/*
	 * Check if the history has been created and a history file is provided.
	 */
	if ( g_history != NULL && history != NULL && *history != '\0' )
	{
		exp_buf = varbuf_create( 512 );

		if (exp_buf == NULL)
		{
			fprintf( stderr, "\\hist-load: %s\n", sqsh_get_errstr() );
		}
		else
		{
			if (sqsh_expand( history, exp_buf, 0 ) == False)
			{
				fprintf( stderr, "\\hist-load: Error expanding $history: %s\n",
					sqsh_get_errstr() );
			}
			else
			{
				if (history_load( g_history, varbuf_getstr(exp_buf) ) == True)
					fprintf( stdout, "\\hist-load - History buffer loaded from %s\n",
							varbuf_getstr(exp_buf) );
				else
					fprintf( stderr, "\\hist-load - Error: Failed to load history from %s\n",
							varbuf_getstr(exp_buf) );

				sprintf( str, "%d", history_get_nbr(g_history) );
				env_set( g_env, "histnum", str );
			}
			varbuf_destroy( exp_buf );
		}
	}
	return CMD_LEAVEBUF ;
}


/*
 * cmd_hist_save:
 *
 * Write the history out to a file.
 *
 */
int cmd_hist_save( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	char       str[16];
	char      *history;
	varbuf_t  *exp_buf;


	/*
	 * Only one argument allowed.
	 */
	if( argc > 2 ) {
		fprintf( stderr, "\\hist-save: Too many arguments; Use: \\hist-save [filename]\n" ) ;
		return CMD_FAIL ;
	}

	if (argc == 2)
		history = argv[1];
	else
		env_get( g_env, "history", &history );

	/*
	 * If the history has been created, and it contains items, and a history file is provided
	 * then we write the history out to this file.
	 */
	if ( g_history != NULL && history != NULL && *history != '\0' && history_get_nitems( g_history ) > 0 )
	{
		exp_buf = varbuf_create( 512 );

		if (exp_buf == NULL)
		{
			fprintf( stderr, "\\hist-save: %s\n", sqsh_get_errstr() );
		}
		else
		{
			if (sqsh_expand( history, exp_buf, 0 ) == False)
			{
				fprintf( stderr, "\\hist-save: Error expanding $history: %s\n",
					sqsh_get_errstr() );
			}
			else
			{
				if (history_save( g_history, varbuf_getstr(exp_buf) ) == True)
					fprintf( stdout, "\\hist-save - History buffer saved to %s\n",
							varbuf_getstr(exp_buf) );
				else
					fprintf( stderr, "\\hist-save - Error: Failed to write history to %s\n",
							varbuf_getstr(exp_buf) );

				sprintf( str, "%d", history_get_nbr(g_history) );
				env_set( g_env, "histnum", str );
			}
			varbuf_destroy( exp_buf );
		}
	}
	return CMD_LEAVEBUF ;
}
