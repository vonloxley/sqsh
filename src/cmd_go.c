/*
 * cmd_go.c - User command to send current work buffer to database and
 *            process results.
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
#include "sqsh_expand.h"
#include "sqsh_error.h"
#include "sqsh_varbuf.h"
#include "sqsh_getopt.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "sqsh_buf.h"
#include "sqsh_filter.h"
#include "sqsh_stdin.h"
#include "cmd.h"
#include "cmd_misc.h"
#include "dsp.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_go.c,v 1.4 2010/02/25 10:50:47 mwesdorp Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/* sqsh-2.1.7 - New functions IgnoreCommentArgs and argv_shift */
static int IgnoreCommentArgs _ANSI_ARGS(( int, char ** )) ;
static void argv_shift _ANSI_ARGS(( int, char **, int )) ;

/*
 * The following macro is used to convert a start time and end
 * time into total elapsed number of seconds, to the decimal
 * place.
 */
#define ELAPSED_SEC(tv_start,tv_end) \
	(((double)(tv_end.tv_sec - tv_start.tv_sec)) + \
		((double)(tv_end.tv_usec - tv_start.tv_usec)) / ((double)1000000.0))

int cmd_go( argc, argv )
	int     argc;
	char   *argv[];
{
	static varbuf_t  *expand_buf = NULL;    /* Where variables are expanded */
	static varbuf_t  *filter_buf = NULL;    /* Where buffer is filtered */
	extern int        sqsh_optind;          /* Required by sqsh_getopt */
	extern char*      sqsh_optarg;          /* Required by sqsh_getopt */
	char             *headers;              /* Value of environment var */
	char             *footers;              /* Dito. */
	char             *expand;               /* Dito. */
	char             *echo;                 /* Dito. */
	char             *statistics;           /* Dito. */
	char             *clear_on_fail;        /* Dito. */
	char             *batch_failcount;      /* Dito. */
	char             *thresh_exit;          /* Dito. */
	char             *batch_pause;          /* Dito. */
	char             *repeat_batch;         /* Dito. */
	char             *filter;               /* Dito. */
	char             *filter_prog;          /* Dito. */
	char             *sql;
	int               sql_len;
	char              pause_buf[5];         /* Buffer for "hit enter" */
	int               ch;
	int               have_error             = False;
	int               no_expand              = False;
	int               exit_status;
	struct timeval    tv_start, tv_end;
	int               i;
	int               return_code;
	int               show_stats    = False;
	int               iterations    = 1;
	int               sleep_time    = 0;
	int               xact          = 0;
	double            total_runtime = 0.0;
	int               dsp_flags     = 0;
	int               dsp_old       = -1;
	char             *dsp_name      = NULL;

	CS_COMMAND       *cmd = NULL;

	/* 
	 * Assume something is going to go wrong.
	 */
	env_set( g_internal_env, "?", "-1" );

	/*
	 * Since we will be temporarily replacing some of our global
	 * settings, we want to set up a save-point to which we can
	 * restore when we are done.
	 */
	env_tran( g_env );

	/*
	 * sqsh-2.1.7 - Allow C style or ANSI SQL comments in \go command line.
	 * Logically or physically move the commentary arguments to the end of
	 * the argv list. Re-assign the effective number of arguments to argc.
	 */
	if ((argc = IgnoreCommentArgs ( argc, argv )) == -1)
	{
		fprintf( stderr, "\\go: Unbalanced comment tokens encountered\n" );
		have_error = True;
	}
	else while ((ch = sqsh_getopt( argc, argv, "nfhps:m:x;w:d:t;T:l" )) != EOF) 
	{
		switch (ch) 
		{
			case 't' :
				if (env_put( g_env, "filter", "1", ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\go: -t: %s\n", sqsh_get_errstr() );
					have_error = True;
				}

				if (sqsh_optarg != NULL)
				{
					if (env_put( g_env, "filter_prog", sqsh_optarg, 
					             ENV_F_TRAN ) == False)
					{
						fprintf( stderr, "\\go: -t: %s\n", sqsh_get_errstr() );
						have_error = True;
					}
				}
				break;

			case 'w' :
				if (env_put( g_env, "width", sqsh_optarg, ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\go: -h: %s\n", sqsh_get_errstr() );
					have_error = True;
				}
				break;

			case 'd' :
				if (env_put( g_env, "DISPLAY", sqsh_optarg, ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\go: -d: %s\n", sqsh_get_errstr() );
					have_error = True;
				}
				break;

			case 'x' :
				dsp_flags |= DSP_F_X;

				if (sqsh_optarg != NULL)
				{
					if (env_put( g_env, "xgeom", sqsh_optarg, ENV_F_TRAN ) == False)
					{
						fprintf( stderr, "\\go: -x: %s\n", sqsh_get_errstr() );
						have_error = True;
					}
				}
				break;
			case 'f' :
				dsp_flags |= DSP_F_NOFOOTERS;
				break;
			case 'l' :
				dsp_flags |= DSP_F_NOSEPLINE;
				break;
			case 'm' :
				dsp_name = sqsh_optarg;
				break;
			case 'n' :
				no_expand = True;
				break;
			case 'h' :
				dsp_flags |= DSP_F_NOHEADERS;
				break;
			case 'p' :
				show_stats = True;
				break;
			case 's' :
				sleep_time = atoi(sqsh_optarg);
				if( sleep_time < 0 ) 
				{
					fprintf( stderr, "\\go: -s: Invalid sleep time\n" );
					have_error = True;
				}
				break;
			case 'T' :
				/*
				 * sqsh-2.1.7 - Set a window title when using X result windows.
				 */
				if (env_put( g_env, "xwin_title", sqsh_optarg, ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\go: -T: %s\n", sqsh_get_errstr() );
					have_error = True;
				}
				break;

			default :
				fprintf( stderr, "\\go: %s\n", sqsh_get_errstr() );
				have_error = True;
		}
	}

	/*
	 * If there are any errors on the command line, or there are
	 * any options left over then we have an error.
	 */
	if( (argc - sqsh_optind) > 1 || have_error) 
	{
	    fprintf( stderr, 
		"Use: \\go [-d display] [-h] [-f] [-l] [-n] [-p] [-m mode] [-s sec]\n"
		"          [-t [filter]] [-w width] [-x [xgeom]] [-T title] [xacts]\n"
		"     -d display  When used with -x, send result to named display\n"
		"     -h          Suppress headers\n"
		"     -f          Suppress footers\n"
		"     -l          Suppress line separators with pretty style output mode\n"
		"     -n          Do not expand variables\n"
		"     -p          Report runtime statistics\n"
		"     -m mode     Switch display mode for result set\n"
		"     -s sec      Sleep sec seconds between transactions\n"
		"     -t [filter] Filter SQL through program\n"
		"                 Optional filter value overrides default variable $filter_prog\n"
		"     -w width    Override value of $width\n"
		"     -x [xgeom]  Send result set to a XWin output window\n"
		"                 Optional xgeom value overrides default variable $xgeom\n"
		"     -T title    Used in conjunction with -x to set window title\n"
		"     xacts       Repeat batch xacts times\n" );

		env_rollback( g_env );
		return CMD_FAIL;
	}

	/*
	 * If the user supplied a parameter then it is treated as the 
	 * number of times the command should be iterated.
	 */
	if( argc != sqsh_optind ) 
	{
		iterations = atoi(argv[argc-1]);

		if( iterations < 1 ) 
		{
			fprintf( stderr, "\\go: Invalid number of transactions\n" );
			env_rollback( g_env );
			return CMD_FAIL;
		}
	}

	/*
	 * If there is nothing to be executed in the work buffer, then
	 * we check to see if the user has requested that they be able
	 * to re-run the previous command.
	 */
	if (varbuf_getlen( g_sqlbuf ) == 0)
	{
		env_get( g_env, "repeat_batch", &repeat_batch );

		/*
		 * If they want to re-run the previous command, then we look
		 * up the buffer and copy it into the current work buffer
		 * and viola!
		 */
		if (repeat_batch != NULL && *repeat_batch == '1')
		{
			sql = buf_get( "!!" );

			if (sql == NULL)
			{
				env_rollback( g_env );
				return CMD_LEAVEBUF;
			}
			varbuf_strcpy( g_sqlbuf, sql );
		}
		else
		{
			env_rollback( g_env );
			return CMD_LEAVEBUF;
		}
	}

	/*
	 * Since we may be run from the background, which causes us to
	 * lose our database connection, we need to make sure we 
	 * re-establish a new connection.
	 */
 	if (g_connection == NULL) 
	{
		if (jobset_run( g_jobset, "\\connect", &exit_status ) == -1) 
		{
			fprintf( stderr, "\\go: Connect failed\n" );
			env_rollback( g_env );
			return CMD_FAIL;
		}
	}

	/*
	 * Retrieve any variables that may affect the way in which
	 * we process or display data.
	 */
	env_get( g_env, "headers", &headers );
	env_get( g_env, "footers", &footers );
	env_get( g_env, "echo", &echo );
	env_get( g_env, "expand",  &expand );
	env_get( g_env, "statistics",  &statistics );
	env_get( g_env, "clear_on_fail",  &clear_on_fail );
	env_get( g_env, "batch_pause",  &batch_pause );
	env_get( g_env, "filter",  &filter );

	/*
	 * If the user didn't request for statistics via the flag, but
	 * they did through the variable, then pretend they used the flag.
	 */
	if( show_stats == False && (statistics != NULL && *statistics == '1') )
		show_stats = True;

	if( !(dsp_flags & DSP_F_NOHEADERS) && headers != NULL && *headers == '0' )
		dsp_flags |= DSP_F_NOHEADERS;
	if( !(dsp_flags & DSP_F_NOFOOTERS) && footers != NULL && *footers == '0' )
		dsp_flags |= DSP_F_NOFOOTERS;


	/*
	 * If the user requests it we expand the current sql buffer of
	 * all variables that it may contain.  Since this requires
	 * a (possibly) large memory copy we allow this feature to
	 * be turned off.
	 */
	if( (expand == NULL || *expand == '1') && no_expand == False ) 
	{
		/*
		 * We need a buffer in which to expand any variables that may be
		 * lurking in the SQL buffer.  In order to save doing this 
		 * over and over again, we keep expand_buf in a static
		 * variable.
		 */
		if( expand_buf == NULL ) 
		{
			if( (expand_buf = varbuf_create(1024)) == NULL ) 
			{
				fprintf(stderr, "\\go: varbuf_create: %s\n", sqsh_get_errstr() );
				env_rollback( g_env );
				return CMD_FAIL;
			}
		}

		/*
		 * Expand the string contained in q_sqlbuf, but do not throw out
		 * any quotes contained within it, we do however, want to strip
		 * the escape character(s).
		 */
		if( sqsh_expand( varbuf_getstr(g_sqlbuf), expand_buf,
		                 EXP_STRIPESC|EXP_COMMENT|EXP_COLUMNS ) == False ) 
		{
	  		fprintf( stderr, "\\go: sqsh_expand: %s\n", sqsh_get_errstr() );
			env_rollback( g_env );
	  		return CMD_FAIL;
	  	}

		/*-- If requested, display the contents of the expanded buffer --*/
		if( echo != NULL && *echo == '1' )
			cmd_display( expand_buf );

	  	sql = varbuf_getstr(expand_buf);
		sql_len = varbuf_getlen(expand_buf);
	}
	else 
	{
		/*-- If requested, display the contents of the sql buffer --*/
		if( echo != NULL && *echo == '1' )
			cmd_display( g_sqlbuf );

		sql = varbuf_getstr(g_sqlbuf);
		sql_len = varbuf_getlen(g_sqlbuf);
	}

	/*
	 * If filtering is enabled, then we have yet another step of 
	 * copying to do.
	 */
	if (filter != NULL && *filter == '1')
	{
		/*
		 * First, we want to check to see if the user has defined a
		 * program to be used as the filter.  If they haven't then
		 * we have nothing to do.
		 */
		env_get( g_env, "filter_prog", &filter_prog );

		if (filter_prog != NULL)
		{
			/*
			 * If the filter buffer hasn't been established yet, then
			 * create it.
			 */
			if (filter_buf == NULL)
			{
				if ((filter_buf = varbuf_create( 1024 )) == NULL)
				{
					fprintf( stderr, 
						"\\go: varbuf_create: %s\n", sqsh_get_errstr() );
					env_rollback( g_env );
					return CMD_FAIL;
				}
			}

			/*
			 * Now, do the filtering.
			 */
			if (sqsh_filter( sql, sql_len, filter_prog, filter_buf ) == False)
			{
				fprintf( stderr, "\\go: %s\n", sqsh_get_errstr() );
				env_rollback( g_env );
				return CMD_FAIL;
			}

			sql     = varbuf_getstr( filter_buf );
			sql_len = varbuf_getlen( filter_buf );
		}
	}

	/*
	 * If the user requests that we use a different display style
	 * than the one currently set, then we need to set the requested
	 * one.  We don't need to check the return value from dsp_set_name()
	 * because we already verified the name with dsp_valid() above.
	 */
	if (dsp_name != NULL) 
	{
		if (dsp_prop( DSP_GET,
		              DSP_STYLE,
		              (void*)&dsp_old,
		              DSP_UNUSED ) != DSP_SUCCEED)
		{
			fprintf( stderr, "\\go: %s\n", sqsh_get_errstr() );
			env_rollback( g_env );
			return CMD_FAIL;
		}

		if (env_put( g_env, "style", dsp_name, ENV_F_TRAN ) != True)
		{
			fprintf( stderr, "\\go: %s\n", sqsh_get_errstr() );
			env_rollback( g_env );
			return CMD_FAIL;
		}

	}

	/*
	 * For each iteration of the execution the user requested
	 * we go through the rigamarole of executing the same
	 * transaction over-and-over.
	 */
	while (++xact <= iterations) 
	{

		/*
		 * Allocate a command structure.
		 */
		if (ct_cmd_alloc( g_connection, &cmd ) != CS_SUCCEED)
		{
			env_rollback( g_env );
			return CMD_FAIL;
		}
		
		/*-- Set up the command to be sent to the server --*/
		if (ct_command( cmd,                      /* Command */
		                CS_LANG_CMD,              /* Type */
		                (CS_VOID*)sql,            /* Buffer */
		                CS_NULLTERM,              /* Buffer Length */
							 CS_UNUSED ) != CS_SUCCEED)
		{
			ct_cmd_drop( cmd );
			env_rollback( g_env );
			return CMD_FAIL;
		}

		/*
		 * If we need to calculate the run-time, then do so.
		 */
		if (show_stats)
			gettimeofday( &tv_start, NULL );

		/*
		 * Have dsp_print() only display output on the final 
		 * iteration.
		 */
		if (xact != iterations)
			dsp_flags |= DSP_F_NOTHING;
		else
			dsp_flags &= ~(DSP_F_NOTHING);

		/*
		 * Since dsp_cmd is going to actually execute the query
		 * at this point, we want to set $? to 0.  The message
		 * callback handler will set it to @@errno if anything
		 * goes wrong.
		 */
		env_set( g_internal_env, "?", "0" );

		i = dsp_cmd( stdout, cmd, sql, dsp_flags );

		ct_cmd_drop( cmd );

		switch (i)
		{
			case DSP_INTERRUPTED:
				env_set( g_internal_env, "?", "-1" );
				goto cmd_go_interrupt;
			case DSP_FAIL:
				env_set( g_internal_env, "?", "-1" );
				goto cmd_go_error;
			case DSP_SUCCEED:
				break;
			default:
				env_set( g_internal_env, "?", "-1" );
				fprintf( stderr, "\\go: Invalid return code from dsp_print!\n" );
				goto cmd_go_error;
		}

		if( show_stats ) 
		{
			gettimeofday( &tv_end, NULL );
			total_runtime += ELAPSED_SEC(tv_start,tv_end);
		}

		if (batch_pause != NULL && *batch_pause == '1')
		{
			if (sqsh_getinput( "Paused. Hit enter to continue...", pause_buf, 0, 
				0 ) == -2)
			{
				goto cmd_go_interrupt;
			}
		}

		/*
		 * If we have more transactions to go and the user has asked
		 * us to sleep between iterations, then do so.
		 */
		if( xact != iterations && sleep_time > 0 )
			sleep( sleep_time );

	} /* while trasactions remain */

	if( show_stats ) 
	{
		if( iterations > 1 )
			printf( "%d xact%s:\n", iterations, (iterations > 1) ? "s" : "" );

		printf(
			"Clock Time (sec.): Total = %.3f  Avg = %.3f (%4.2f xacts per sec.)\n",
			total_runtime, 
			(total_runtime / (double)iterations),
			((double)1.0) / (total_runtime / (double)iterations));
	}

	goto cmd_go_succeed;

	/*
	 * The following jump points are used by the various parts of
	 * cmd_go, above, to deal with various styles of returning.
	 */
cmd_go_interrupt:
	if (!sqsh_stdin_isatty())
		return_code = CMD_INTERRUPTED;
	else
		return_code = CMD_RESETBUF;

	goto cmd_go_leave;

cmd_go_error:
	/*
	 * If something went wrong, and clear_on_fail is 1, then request
	 * that the work buffer be cleared.
	 */
	if( clear_on_fail == NULL || *clear_on_fail == '1' )
		return_code = CMD_RESETBUF;
	else
		return_code = CMD_FAIL;
	goto cmd_go_leave;

cmd_go_succeed:
	return_code = CMD_RESETBUF;

cmd_go_leave:
	if (dsp_old != -1)
	{
		dsp_prop( DSP_SET, DSP_STYLE, (void*)&dsp_old, DSP_UNUSED );
	}

	/*
	 * Restore the environment variables back to their
	 * original state.
	 */
	env_rollback( g_env );

	/*
	 * Retrieve the current values of thresh_exit and batch_failcount.
	 * If thresh_exit has been set to something other than 0, and
	 * thresh_exit == batch_failcount, then we want to abort (too
	 * many errors.  Note that there are only applicable while in
	 * non-interactive mode.
	 */
	if (!sqsh_stdin_isatty())
	{
		env_get( g_env, "thresh_exit", &thresh_exit );
		env_get( g_env, "batch_failcount", &batch_failcount );

		if (thresh_exit != NULL && batch_failcount != NULL) 
		{
			i = atoi(thresh_exit);

			if (i > 0 && atoi(batch_failcount) >= i)
			{
				DBG(sqsh_debug(DEBUG_ERROR,
					"cmd_go: batch_failcount = %s, thresh_exit = %d: Aborting.\n",
					batch_failcount, i);)

				return CMD_ABORT;
			}
		}
	}

	return return_code;
}

/*
 * sqsh-2.1.7 - Function: IgnoreCommentArgs
 *
 * If the '\go' command line contains comments, then move the arguments that
 * belong to a comment to the end of the argument list. The actual parameters
 * are at the beginning of the list.
 *
 * Return the number of real arguments, that is the adjusted argc value.
 * In case of error, return -1.
 */
int IgnoreCommentArgs (argc, argv)
    int   argc;
    char *argv[];
{
    int i = 1;  /* Skip command name argument argv[0] and start with argv[1] */

    while ( i < argc )
    {
        if ( strcmp( argv[i], "--" ) == 0 )
        {
            /*
             * Logically delete all remaining parameters.
            */
            return (i);
        }
        else if ( strcmp( argv[i], "/*" ) == 0 )
        {
            /*
             * Move all the parameters in the C-style commentary block
             * to the end of the argument list and decrement the
             * count of valuable arguments. We do not have to increment 'i',
             * because the argv_shift function shifts in the next argument to
             * the current location.
             * Note, because we decrement argc, the comment entries will
             * eventually be stored in reverse order, but that's OK here.
            */
            do {
                argv_shift ( argc--, argv, i );
            } while ( i < argc && strcmp( argv[i], "*/" ) != 0 );

            if ( i < argc && strcmp( argv[i], "*/" ) == 0 )
                argv_shift ( argc--, argv, i );
            else
                return (-1); /* Missing end of comment token */
        }
        else if ( strcmp( argv[i], "*/" ) == 0 )
            return (-1); /* Missing beginning of comment token */
        else
            i++;
    }

    return (argc);
}

/*
 * sqsh-2.1.7 - Function: argv_shift
 *
 * Helper function to move an argument in an argv array to
 * the end of the list and shift the rest of the arguments
 * up to the named location.
*/
static void argv_shift( argc, argv, idx )
    int   argc;
    char  *argv[];
    int   idx;
{
    char *cptr;
    int   i;

    /*-- Save pointer to argument --*/
    cptr = argv[idx] ;

    /*
     * First, we need to shift the rest of the arguments down,
     * removing this argument.
    */
    for( i = idx; i < argc - 1; i++ )
        argv[i] = argv[i+1] ;

    /*
     * Now, put this argument to the end of the argv array.
    */
    argv[argc - 1] = cptr ;
}

