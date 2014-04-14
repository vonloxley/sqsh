/*
** cmd_do.c - User command to send current work buffer to database and
**            process results.
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

static void    cmd_do_sigint_jmp     _ANSI_ARGS(( int, void* )) ;
static void    cmd_do_sigint_cancel  _ANSI_ARGS(( int, void* )) ;
static int     cmd_do_exec           _ANSI_ARGS(( CS_CONNECTION*, char*,
                                                  char* ));

/*
** sg_jmp_buf: The following buffer is used to contain the location
**             to which this module will return upon receipt of a
**             SIGINT. It is only used while waiting on input from the
**             user.
*/
static JMP_BUF sg_jmp_buf;
static int     sg_canceled = False;

int cmd_do( argc, argv )
	int     argc;
	char   *argv[];
{
	extern int        sqsh_optind;          /* Required by sqsh_getopt */
	extern char*      sqsh_optarg;          /* Required by sqsh_getopt */
	int               ch;
	char             *repeat_batch;
	char             *expand;
	varbuf_t         *expand_buf = NULL;
	char             *sql;

	varbuf_t         *do_buf;
	varbuf_t         *orig_sqlbuf;          /* SQL Buffer upon entry */
	CS_CONNECTION    *orig_conn;            /* Connection upon entry */
	CS_CONTEXT       *orig_ctxt;            /* Context upon entry */
	char              orig_password[SQSH_PASSLEN+1] = ""; /* Current session password */
	int               ret        = False;
	int               have_error = False;
	int               exit_status;
	int               do_connection = True;

	/*
	** Since we will be temporarily replacing some of our global
	** settings, we want to set up a save-point to which we can
	** restore when we are done.
	*/
	env_tran( g_env );

	/*
	 *                        sqsh-2.4 modification.
	 * This is totally utterly ugly, but the \do block may call \reconnect with a lot of
	 * parameter changes that will be committed in the global environment upon a successfull
	 * connection. If we put some of the original values on the logsave chain first, then the
	 * env_rollback call at the end of this function will eventually restore these parameter
	 * values back to the values of the current session.
	 */
	env_get( g_env, "DSQUERY",  &expand); env_put( g_env, "DSQUERY",  expand, ENV_F_TRAN );
	env_get( g_env, "database", &expand); env_put( g_env, "database", expand, ENV_F_TRAN );
	env_get( g_env, "username", &expand); env_put( g_env, "username", expand, ENV_F_TRAN );
	if (g_password != NULL)
		strcpy ( orig_password, g_password);

	while ((ch = sqsh_getopt( argc, argv, "S:U:P:D:n" )) != EOF)
	{
		switch (ch)
		{
			case 'n' :
				do_connection = False;
				ret = True;
				break;
			case 'S' :
				ret = env_put( g_env, "DSQUERY", sqsh_optarg, ENV_F_TRAN );
				break;
			case 'U' :
				ret = env_put( g_env, "username", sqsh_optarg, ENV_F_TRAN );
				break;
			case 'P' :
				ret = env_put( g_env, "password", sqsh_optarg, ENV_F_TRAN );
				break;
			case 'D' :
				ret = env_put( g_env, "database", sqsh_optarg, ENV_F_TRAN );
				break;
			default :
				ret = False;
		}

		if (ret != True)
		{
			fprintf( stderr, "\\do: %s\n", sqsh_get_errstr() );
			have_error = True;
		}
	}

	/*
	** If there are any errors on the command line, or there are
	** any options left over then we have an error.
	*/
	if( (argc - sqsh_optind) > 0 || have_error == True)
	{
		fprintf( stderr,
			"Use: \\do [-n] [-S server] [-U user] [-P pass] [-D db]\n"
			"        -n   Do not establish new connection (cannot issue SQL)\n"
			"        -S   Perform do-loop on specified server\n"
			"        -U   User name for do-loop connection\n"
			"        -P   Password-loop connection\n"
			"        -D   Database context for activity\n" );
		env_rollback( g_env );
		return(CMD_FAIL);
	}

	/*
	** If there is nothing to be executed in the work buffer, then
	** we check to see if the user has requested that they be able
	** to re-run the previous command.
	*/
	if (varbuf_getlen( g_sqlbuf ) == 0)
	{
		env_get( g_env, "repeat_batch", &repeat_batch );

		/*
		** If they want to re-run the previous command, then we look
		** up the buffer and copy it into the current work buffer
		** and viola!
		*/
		if (repeat_batch != NULL && *repeat_batch == '1')
		{
			sql = buf_get( "!!" );

			if (sql == NULL)
			{
				env_rollback( g_env );
				return(CMD_LEAVEBUF);
			}
			varbuf_strcpy( g_sqlbuf, sql );
		}
		else
		{
			varbuf_strcpy( g_sqlbuf, "" );
		}
	}

	/*
	** Before we go any further, read the remainder of the input
	** from the user (up to \done).
	*/
	do_buf = varbuf_create( 512 );

	if ((ret = cmd_body_input( do_buf )) != CMD_RESETBUF)
	{
		varbuf_destroy( do_buf );
		env_rollback( g_env );
		return(ret);
	}

	/*
	** Perform variable expansion if necessary.
	*/
	env_get( g_env, "expand", &expand );
	if (*expand == '1')
	{
		expand_buf = varbuf_create( 1024 );

		if (expand_buf == NULL)
		{
			fprintf( stderr, "\\do: Memory allocation failure\n" );
			varbuf_destroy( do_buf );
			env_rollback( g_env );
			return(CMD_FAIL);
		}

		if (sqsh_expand( varbuf_getstr( g_sqlbuf ), expand_buf,
			(EXP_STRIPESC|EXP_COMMENT|EXP_COLUMNS) ) == False)
		{
			fprintf( stderr, "\\do: Expansion failure: %s\n",
				sqsh_get_errstr() );

			varbuf_destroy( expand_buf );
			varbuf_destroy( do_buf );
			env_rollback( g_env );
			return(CMD_FAIL);
		}

		sql = varbuf_getstr( expand_buf );
	}
	else
	{
		sql = varbuf_getstr( g_sqlbuf );
	}

	/*
	** Now that we have our input we are ready to go.  Since we
	** are going to be temporarily replacing a couple of global
	** resources (namely the connection and the sql buffer), we
	** need to save away pointers to the original copies.
	*/
	orig_sqlbuf = g_sqlbuf;
	orig_conn   = g_connection;
	orig_ctxt   = g_context;

	/*
	** And replace then with new copies.  Note that setting
	** g_connection to NULL will cause a new connection to be
	** established for us by \connect.
	*/
	g_sqlbuf     = varbuf_create( 512 );

	/*
	** Default return code.
	*/
	ret = CMD_RESETBUF;

	/*
	** Create the new connection for the sub-batch.
	*/
	if (do_connection == True)
	{
		g_connection = NULL;
		g_context    = NULL;
		if (jobset_run( g_jobset, "\\connect", &exit_status ) == -1)
		{
			fprintf( stderr, "\\do: Connect failed\n" );
			ret = exit_status;
		}
	}

	if (ret != CMD_FAIL)
	{
		ret = cmd_do_exec( orig_conn, sql, varbuf_getstr(do_buf) );
	}

	if (do_connection == True &&
		g_connection != NULL)
	{
		if (ct_close( g_connection, CS_UNUSED ) != CS_SUCCEED)
		    ct_close( g_connection, CS_FORCE_CLOSE );
		ct_con_drop( g_connection );
		g_connection = NULL;
	}

	if (do_connection == True &&
		g_context != NULL)
	{
		if (ct_exit ( g_context, CS_UNUSED) != CS_SUCCEED)
		    ct_exit ( g_context, CS_FORCE_EXIT );
		cs_ctx_drop ( g_context );
		g_context = NULL;
	}

	varbuf_destroy( g_sqlbuf );
	varbuf_destroy( do_buf );

	g_context    = orig_ctxt;
	g_connection = orig_conn;
	g_sqlbuf     = orig_sqlbuf;

	if (expand_buf != NULL)
		varbuf_destroy( expand_buf );

	env_rollback( g_env );
	env_set( g_env, "password", orig_password );
	return(ret);
}

static int cmd_do_exec( conn, sql, dobuf )
	CS_CONNECTION  *conn;
	char           *sql;
	char           *dobuf;
{
	CS_COMMAND *cmd;
	int         ret;
	CS_RETCODE  retcode;
	CS_INT      result_type;
	CS_INT      nrows;
	dsp_desc_t *desc;

	/*
	** Save away current signal context.
	**
	** sqsh-2.5: Make sure that the signal context is restored using sig_restore() prior
	** to every return from this function.
	*/
	sig_save();

	/*
	** Install signal handler
	*/
	sg_canceled = False;
	sig_install( SIGINT, cmd_do_sigint_cancel, (void*)conn, 0 );

	/*
	** Allocate new command structure.
	*/
	if (ct_cmd_alloc( conn, &cmd ) != CS_SUCCEED)
	{
		fprintf( stderr, "\\do: Error allocating CS_COMMAND structure\n" );

		sig_restore();
		return(CMD_FAIL);
	}

	if (ct_command( cmd, CS_LANG_CMD, (CS_VOID*)sql, CS_NULLTERM, CS_UNUSED )
		!= CS_SUCCEED)
	{
		fprintf( stderr, "\\do: Error initializing command\n" );

		ct_cmd_drop( cmd );
		sig_restore();
		return(CMD_FAIL);
	}

	/* sqsh-2.5 - Feature p2f, reset g_p2fc before a new batch is started */
	g_p2fc = 0;

	if (ct_send( cmd ) != CS_SUCCEED)
	{
		fprintf( stderr, "\\do: Error sending command\n" );

		ct_cmd_drop( cmd );
		sig_restore();
		return(CMD_FAIL);
	}

	/*
	** Suck in the results.
	*/
	while ((retcode = ct_results( cmd, &result_type ))
		== CS_SUCCEED)
	{
		/*
		** Check to see if we were canceled.
		*/
		if (sg_canceled == True)
		{
			ct_cancel( conn, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
			ct_cmd_drop( cmd );
			sig_restore();
			return(CMD_INTERRUPTED);
		}

		switch (result_type)
		{
			case CS_STATUS_RESULT:
			case CS_PARAM_RESULT:
				while((retcode = ct_fetch( cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED,
					&nrows )) == CS_SUCCEED);
				
				if (retcode != CS_END_DATA)
				{
					ct_cancel( conn, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
					ct_cmd_drop( cmd );
					sig_restore();

					if (retcode == CS_CANCELED)
					{
						return(CMD_INTERRUPTED);
					}
					return(CMD_FAIL);
				}
				break;
			
			case CS_ROW_RESULT:
			case CS_COMPUTE_RESULT:
				
				desc = dsp_desc_bind( cmd, result_type );

				if (desc == NULL)
				{
					ct_cancel( conn, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
					ct_cmd_drop( cmd );
					sig_restore();
					return(CMD_FAIL);
				}

				if (sg_canceled == True)
				{
					ct_cancel( conn, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
					ct_cmd_drop( cmd );
					dsp_desc_destroy( desc );
					sig_restore();
					return(CMD_INTERRUPTED);
				}

				/*
				** Save away the column description in the global table
				** of column descriptions (these will be referenced
				** during expansion of the sqlbuf.
				*/
				g_do_cols[g_do_ncols] = desc;
				++g_do_ncols;

				while((retcode = dsp_desc_fetch( cmd, desc )) == CS_SUCCEED)
				{

					sqsh_stdin_buffer( dobuf, -1 );

					/*
					** For each row we fetch back, we want to execute
					** the dobuf.
					*/
					if ((ret = cmd_input()) == CMD_FAIL ||
						ret == CMD_ABORT       ||
						ret == CMD_INTERRUPTED ||
						ret == CMD_BREAK       ||
						ret == CMD_RETURN)
					{
						sqsh_stdin_pop();

						/*
						** If the caller is breaking out of this loop, then
						** translate the return code to a LEAVEBUF. This
						** will prevent the parent loop from breaking as
						** well.
						*/
						if (ret == CMD_BREAK)
						{
							sig_restore();
							return(CMD_LEAVEBUF);
						}

						ct_cancel( conn, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
						ct_cmd_drop( cmd );
						dsp_desc_destroy( desc );
						--g_do_ncols;
						sig_restore();
						return(ret);
					}

					sqsh_stdin_pop();

					if (sg_canceled == True)
					{
						ct_cancel( conn, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
						ct_cmd_drop( cmd );
						dsp_desc_destroy( desc );
						sig_restore();
						return(CMD_INTERRUPTED);
					}
				}
				
				dsp_desc_destroy( desc );
				--g_do_ncols;

				if (retcode != CS_END_DATA)
				{
					ct_cancel( conn, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
					ct_cmd_drop( cmd );
					sig_restore();

					if (retcode == CS_CANCELED)
					{
						return(CMD_INTERRUPTED);
					}
					return(CMD_FAIL);
				}
				break;
			
			default:
				break;
		}
	}

	if (retcode != CS_END_RESULTS)
	{
		ct_cancel( conn, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
		ct_cmd_drop( cmd );
		sig_restore();
		return(CMD_FAIL);
	}

	ct_cmd_drop( cmd );
	sig_restore();
	return(CMD_RESETBUF);
}

/*
** cmd_body_input():
**
** Get input for the body of the "do" or "func" command.
*/
int cmd_body_input( buf )
	varbuf_t  *buf;
{
	char       *prompt;
	char       *str;
	char        prompt_indent[64];
	char        new_prompt[128];
	char        cmd[6];
	char       *cp;
	int         i;
	varbuf_t   *prompt_buf = NULL;
	int         nesting_level = 1;
	JMP_BUF     orig_jmpbuf;

	/*
	** Allocate a buffer to expand the prompt in.
	*/
	if (sqsh_stdin_isatty())
	{
		prompt_buf = varbuf_create( 512 );

		if (prompt_buf == NULL)
		{
			fprintf( stderr, "\\do: Error allocating prompt buffer: %s\n",
				sqsh_get_errstr() );
			return(CMD_FAIL);
		}

		env_get( g_env, "prompt", &prompt );
		sprintf( new_prompt, "${prompt_indent}%s", prompt );
		env_put( g_env, "prompt_indent", "  ", ENV_F_TRAN );
		env_put( g_env, "prompt", new_prompt, ENV_F_TRAN );
	}

	/*
	** Save the current signal handlers.
	*/
	sig_save();

	/*
	** Save copy of jump buffer.
	*/
	memcpy((void*)&(orig_jmpbuf), (void*)&(sg_jmp_buf), sizeof(JMP_BUF));

	/*
	** If we receive a SIGINT while waiting on input from the user
	** and we are not sure if the input routine will return upon
	** receiving the signal, we need to set up a jump point and
	** replace the existing signal handler.
	*/
	if (SETJMP( sg_jmp_buf ) != 0)
	{
		/*
		** Restore the signal state.
		*/
		sig_restore();

		if (prompt_buf != NULL)
			varbuf_destroy( prompt_buf );

		memcpy((void*)&(sg_jmp_buf), (void*)&(orig_jmpbuf), sizeof(JMP_BUF));
		return(CMD_INTERRUPTED);
	}

	/*
	** Install one that will jump us back to the statement
	** above when we get interrupted.
	*/
	sig_install( SIGINT, cmd_do_sigint_jmp, (void*)NULL, 0 );

	/*
	** Sit in a loop reading input until we hit EOF, or until we
	** get a \done as the only command on a line.
	*/
	for (;;)
	{
		/*
		** If this is an interactive session, then we want to
		** display a prompt for the user.
		*/
		if (sqsh_stdin_isatty())
		{
			/*
			** Increment line number by one.
			*/
			env_set( g_env, "lineno", "1" );

			env_get( g_env, "prompt", &prompt );
			sqsh_expand( prompt, prompt_buf, 0 );

			prompt = varbuf_getstr( prompt_buf );
		}
		else
		{
			prompt = NULL;
		}

		str = sqsh_readline( prompt );

		if (str == NULL)
		{
			if (sqsh_get_error() == SQSH_E_NONE)
			{
				break;
			}

			if (prompt_buf != NULL)
			{
				varbuf_destroy( prompt_buf );
			}

			sig_restore();
			memcpy((void*)&(sg_jmp_buf), (void*)&(orig_jmpbuf), sizeof(JMP_BUF));
			return(CMD_FAIL);
		}

		/*
		** Check to see if the string is one of \do or \done.
		** First, skip white-space.
		*/
		for(cp = str; *cp != '\0' && isspace((int)*cp); ++cp);

		/*
		** If the next character is not a '\', then we are ok.
		*/
		if (*cp == '\\')
		{
			++cp;
			for(i = 0; i < (sizeof(cmd)-1) && *cp != '\0' &&
				isalpha((int)*cp); ++i, ++cp)
			{
				cmd[i] = *cp;
			}
			cmd[i] = '\0';

			if (strcmp( cmd, "done" ) == 0)
			{
				--nesting_level;

				if (nesting_level == 0)
				{
					break;
				}

				/*
				** And shrink the intending level a tad.
				*/
				for (i = 0; i < (nesting_level)*2; i++)
				{
					prompt_indent[i] = ' ';
				}
				prompt_indent[i] = '\0';
				env_put( g_env, "prompt_indent", prompt_indent, ENV_F_TRAN );
			}
			else if ( strcmp( cmd, "do" )    == 0  ||
				  strcmp( cmd, "for" )   == 0  || /* sqsh-2.3 - Improvement suggested by Niki Hansche */
				  strcmp( cmd, "func" )  == 0  ||
				  strcmp( cmd, "while" ) == 0)
			{
				/*
				** If we hit another \do statement, then we want to
				** bump up the nesting level.
				*/
				++nesting_level;

				for (i = 0; i < nesting_level*2; i++)
				{
					prompt_indent[i] = ' ';
				}
				prompt_indent[i] = '\0';
				env_put( g_env, "prompt_indent", prompt_indent, ENV_F_TRAN );
			}
		}

		/*
		** If we haven't reached the 0'th nesting level,
		** then just tack this command on the line.
		*/
		varbuf_strcat( buf, str );
	}

	if (prompt_buf != NULL)
		varbuf_destroy( prompt_buf );

	sig_restore();
	memcpy((void*)&(sg_jmp_buf), (void*)&(orig_jmpbuf), sizeof(JMP_BUF));
	return(CMD_RESETBUF);
}

/*
** cmd_do_sigint_cancel():
**
** Cancels current query upon receipt of SIGINT.
*/
static void cmd_do_sigint_cancel( sig, user_data )
	int sig ;
	void *user_data;
{
	if (user_data != NULL)
	{
		ct_cancel( (CS_CONNECTION*)user_data, (CS_COMMAND*)NULL,
			CS_CANCEL_ATTN );
	}
	sg_canceled = True;
}

/*
** cmd_do_sigint_jmp():
**
** Used to catch ^C's from the user.  If there is currently a database
** action in progress it is canceled..
*/
static void cmd_do_sigint_jmp( sig, user_data )
	int sig ;
	void *user_data;
{
	LONGJMP( sg_jmp_buf, 1 ) ;
}
