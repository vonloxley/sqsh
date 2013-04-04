/*
** cmd_if.c - Conditional logic.
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
#include "sqsh_getopt.h"
#include "sqsh_buf.h"
#include "sqsh_stdin.h"
#include "sqsh_readline.h"
#include "cmd.h"
#include "dsp.h"
#include "cmd_misc.h"
#include "cmd_input.h"

static int     cmd_if_input       _ANSI_ARGS(( varbuf_t*, varbuf_t* ));
static void    cmd_if_sigint_jmp  _ANSI_ARGS(( int, void* )) ;
static JMP_BUF sg_jmp_buf;

/*
** cmd_if():
**
** Perform conditional logic.
*/
int cmd_if( argc, argv )
	int     argc;
	char   *argv[];
{
	varbuf_t  *if_buf;
	varbuf_t  *else_buf;
	varbuf_t  *buf;
	int        ret;
	int        exit_status;

	if (argc < 1)
	{
		fprintf( stderr, "Use: \\if <command>\n" );
		fprintf( stderr, "         <body>\n" );
		fprintf( stderr, "     \\elif <command>\n" );
		fprintf( stderr, "         <body>\n" );
		fprintf( stderr, "     \\else\n" );
		fprintf( stderr, "         <body>\n" );
		fprintf( stderr, "     \\fi\n" );
		return(CMD_FAIL);
	}

	/*
	** Allocate a buffer for the input.
	*/
	if ((if_buf = varbuf_create( 512 )) == NULL)
	{
		fprintf( stderr, "\\func: Memory allocation failure\n" );
		return(CMD_FAIL);
	}

	/*
	** Allocate a buffer for the input.
	*/
	if ((else_buf = varbuf_create( 512 )) == NULL)
	{
		fprintf( stderr, "\\func: Memory allocation failure\n" );
		varbuf_destroy( if_buf );
		return(CMD_FAIL);
	}

	env_tran(g_env);

	/*
	** Get the if/else portions of the input.  Note that elif is
	** still considered an 'else' for our purposes.
	*/
	if ((ret = cmd_if_input( if_buf, else_buf )
		!= CMD_LEAVEBUF))
	{
		varbuf_destroy( if_buf );
		varbuf_destroy( else_buf );

		env_rollback(g_env);
		return(ret);
	}

	env_rollback(g_env);

	/*
	** Now, execute the command.
	*/
	if (cmd_if_exec( argc - 1, &(argv[1]), &exit_status ) 
		== CMD_FAIL)
	{
		varbuf_destroy( if_buf );
		varbuf_destroy( else_buf );
		return(ret);
	}

	/*
	** Upon success, execute the if_buf.
	*/
	if (exit_status == 0)
	{
		varbuf_destroy( else_buf );
		buf = if_buf;
	}
	else
	{
		/*
		** Otherwise we'll do the else_buf.
		*/
		varbuf_destroy( if_buf );
		buf = else_buf;
	}

	/*
	** Redirect the stdin to the new buffer.
	*/
	sqsh_stdin_buffer( varbuf_getstr(buf), -1 );

	/*
	** And execute it.
	*/
	ret = cmd_input();

	/*
	** Restore stdin.
	*/
	sqsh_stdin_pop();

	/*
	** Destroy the buffer.
	*/
	varbuf_destroy( buf );

	/*
	** And return.
	*/
	return(ret);
}

/*
** cmd_if_exec():
**
** Execute command to be evaluated.
*/
int cmd_if_exec( argc, argv, exit_status )
	int    argc;
	char **argv;
	int   *exit_status;
{
	pid_t   pid;
	pid_t   ret_pid;
	int     status;
	char    nbr[16];
	func_t *f;
	char    *return_str;

#if defined(NOT_DEFINED)
	int     i;
	for (i = 0; i < argc; i++)
	{
		fprintf( stdout, "argv[%d] = '%s'\n", i, argv[i] );
	}
#endif /* NOT_DEFINED */

	/*
	** Just in case.
	*/
	*exit_status = -1;

	/*
	** Just for giggles, before we ship the command
	** off to the operating system, lets check to see
	** if this is a user-defined command.
	*/
	f = funcset_get( g_funcset, argv[0] );

	/*
	** If it *is* a user defined function, then lets
	** call it right now.
	*/
	if (f != NULL)
	{
		cmd_call( argc, argv );

		/*
		** Convert the $? to something usable by the
		** caller.
		*/
		env_get( g_internal_env, "?", &return_str );
		*exit_status = atoi(return_str);

		return(CMD_LEAVEBUF);
	}

	/*
	** Temporarily set the default SIGCHLD handler.
	*/
	sig_save();
	sig_install( SIGCHLD, SIG_H_DFL, (void*)NULL, 0 );

	switch (pid = fork())
	{
		case -1:
			fprintf( stderr, "\\if: fork() call failed: %s\n",
				strerror(errno) );
			env_set( g_internal_env, "?", "-1" );
			sig_restore();
			return(CMD_FAIL);
		
		case 0:  /* Child process */
			if (execvp( argv[0], argv ) == -1)
			{
				fprintf( stderr, "\\if: Failed to exec %s: %s\n",
					argv[0], strerror(errno) );
				exit(-1);
			}
			/*
			** NOT REACHED
			*/
			break;
		
		default: /* Parent process */
			/*
			** Wait for child to complete.
			*/
			do
			{
				ret_pid = waitpid(pid, &status, (int)0);
			}
			while (ret_pid == -1 && errno == EINTR);

			if (ret_pid == -1)
			{
				fprintf( stderr, "\\if: Error from waitpid(): %s\n",
					strerror(errno) );

				env_set( g_internal_env, "?", "-1" );
				sig_restore();
				return(CMD_FAIL);
			}

			if (WIFEXITED(status))
			{
				*exit_status = WEXITSTATUS(status);
			}
			else
			{
				*exit_status = -1;
			}

			sprintf( nbr, "%d", *exit_status );
			env_set( g_internal_env, "?", nbr );
			break;
	}

	sig_restore();
	return(CMD_LEAVEBUF);
}


/*
** cmd_if_input():
**
** Get input for the body of the "\if" function.
*/
int cmd_if_input( if_buf, else_buf )
	varbuf_t  *if_buf;
	varbuf_t  *else_buf;
{
	char       *prompt;
	char       *str;
	char        prompt_indent[64];
	char        new_prompt[128];
	char        cmd[6];
	char       *cmd_start;
	char       *cp;
	int         i;
	varbuf_t   *prompt_buf = NULL;
	int         nesting_level = 1;
	JMP_BUF     orig_jmpbuf;
	int         ret = CMD_LEAVEBUF;
	int         have_elif = False;

#define STATE_IF    1
#define STATE_ELIF  2
#define STATE_ELSE  3
	int         parse_state = STATE_IF;

	/*
	** Allocate a buffer to expand the prompt in.
	*/
	if (sqsh_stdin_isatty())
	{
		prompt_buf = varbuf_create( 512 );

		if (prompt_buf == NULL)
		{
			fprintf( stderr, "\\if: Error allocating prompt buffer: %s\n",
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
		{
			varbuf_destroy( prompt_buf );
		}

		memcpy((void*)&(sg_jmp_buf), (void*)&(orig_jmpbuf), sizeof(JMP_BUF));
		return(CMD_INTERRUPTED);
	}

	/*
	** Install one that will jump us back to the statement
	** above when we get interrupted.
	*/
	sig_install( SIGINT, cmd_if_sigint_jmp, (void*)NULL, 0 );

	/*
	** Sit in a loop reading input until we hit EOF, or until we
	** get a \fi as the only command on a line.
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
			if (prompt_buf != NULL)
			{
				varbuf_destroy( prompt_buf );
			}

			if (sqsh_get_error() == SQSH_E_NONE)
			{
				fprintf( stderr, "\\if: \\fi expected.\n" );
				/*
				 * sqsh-2.1.7 - Logical fix: Do not return CMD_FAIL before the
				 * signalling context and original jump buffer are restored.
				 *
				 * return(CMD_FAIL);
				 */
			}

			sig_restore();
			memcpy((void*)&(sg_jmp_buf), (void*)&(orig_jmpbuf), sizeof(JMP_BUF));
			return(CMD_FAIL);
		}

		/*
		** Check to see if the string is one of \if, \elif, \else, or \fi.
		** First, skip white-space.
		*/
		for(cp = str; *cp != '\0' && isspace((int)*cp); ++cp);

		/*
		** If the next character is not a '\', then we are ok.
		*/
		if (*cp == '\\')
		{
			/*
			** Save away where the command starts.
			*/
			cmd_start = cp;

			++cp;
			for(i = 0; i < (sizeof(cmd)-1) && *cp != '\0' && 
				isalpha((int)*cp); ++i, ++cp)
			{
				cmd[i] = *cp;
			}
			cmd[i] = '\0';

			/*
			** If we hit another \if, then we just need to record that
			** we are nested a level.
			*/
			if (strcmp( cmd, "if" ) == 0)
			{
				/*
				** If we hit another \if statement, then we want to
				** bump up the nesting level.
				*/
				++nesting_level;

				for (i = 0; i < nesting_level*2; i++)
				{
					prompt_indent[i] = ' ';
				}
				prompt_indent[i] = '\0';
				env_put( g_env, "prompt_indent", prompt_indent, ENV_F_TRAN );

				/*
				** Bugfix sqsh-2.1.8
				** Copy the nested new 'if' into the current buffer.
				*/
				if (parse_state == STATE_IF)
					varbuf_strcat( if_buf, str );
				else
					varbuf_strcat( else_buf, str );
			}
			else if (strcmp( cmd, "elif" ) == 0 && 
				nesting_level == 1)
			{
				/*
				** If we hit an elif that applies to the outermost if, 
				** then need to check to see if we were expecting one.
				*/
				if (parse_state == STATE_ELSE)
				{
					fprintf( stderr, "\\if: \\elif not expected\n" );
					ret = CMD_FAIL;
				}
				else if (parse_state == STATE_IF)
				{
					/*
					** If we were previously parsing the \if block and we
					** hit the \elif, then we are going to do some trickery.
					** We are going to copy the current line into the 
					** else_buf, BUT we are going to replace the \elif
					** with a \if.  Thus, turning:
					**
					**    if ...
					**        body
					**    elif ...
					**        body
					**    fi
					**
					** into
					**
					**    if ...
					**        body
					**    else
					**        if ...
					**           body
					**        fi
					**    fi
					**
					** Cheesy, ain't it?
					*/
					cmd_start[1] = 'i';
					cmd_start[2] = 'f';
					cmd_start[3] = ' ';
					cmd_start[4] = ' ';

					varbuf_strcat( else_buf, str );
					parse_state = STATE_ELIF;

					/*
					** Record that we have an \elif so that we can
					** re-append the \fi on the end.
					*/
					have_elif = True;
				}
				else
				{
					/*
					** IF we were already in an elif block, then just
					** copy the current input into the else buffer.
					*/
					varbuf_strcat( else_buf, str );
				}
			}
			else if (strcmp( cmd, "else" ) == 0 && 
				nesting_level == 1)
			{
				/*
				** If we hit an ELSE after we have already seen an
				** ELSE, then we want to puke.
				*/
				if (parse_state == STATE_ELSE)
				{
					fprintf( stderr, "\\if: \\else not expected\n" );
					ret = CMD_FAIL;
				}

				/*
				** If we had been parsing an ELIF, then we want
				** to keep this \else since it is now part of the
				** \elif expression.
				*/
				if (parse_state == STATE_ELIF)
				{
					varbuf_strcat( else_buf, str );
				}

				parse_state = STATE_ELSE;

			}
			else if (strcmp( cmd, "fi" ) == 0)
			{
				/*
				** We have hit the end of an \if block.
				*/
				--nesting_level;

				/*
				** If it is the outer-most block, then we are done 
				** here.
				*/
				if (nesting_level == 0)
				{
					/*
					** If we did the elif hack mentioned above, then we
					** need to keep the \fi to complete the newly formed
					** \fi expression.
					*/
					if (have_elif == True)
					{
						varbuf_strcat( else_buf, str );
					}
					break;
				}

				for (i = 0; i < nesting_level*2; i++)
				{
					prompt_indent[i] = ' ';
				}
				prompt_indent[i] = '\0';
				env_put( g_env, "prompt_indent", prompt_indent, ENV_F_TRAN );

				/*
				** Copy the 'fi' into the current buffer.
				*/
				if (parse_state == STATE_IF)
					varbuf_strcat( if_buf, str );
				else
					varbuf_strcat( else_buf, str );
			}
			else
			{
				/*
				** If the input is an unrecognized command, then
				** just blindly copy the line.
				*/
				if (parse_state == STATE_IF)
					varbuf_strcat( if_buf, str );
				else
					varbuf_strcat( else_buf, str );
			}
		}
		else
		{
			/*
			** Line doesn't have anything we are interested in...
			*/
			if (parse_state == STATE_IF)
				varbuf_strcat( if_buf, str );
			else
				varbuf_strcat( else_buf, str );
		}
	}

	if (prompt_buf != NULL)
	{
		varbuf_destroy( prompt_buf );
	}

	sig_restore();
	memcpy((void*)&(sg_jmp_buf), (void*)&(orig_jmpbuf), sizeof(JMP_BUF));
	return(ret);
}

/*
** cmd_if_sigint_jmp():
**
** Used to catch ^C's from the user.
*/
static void cmd_if_sigint_jmp( sig, user_data )
	int sig ;
	void *user_data;
{
	LONGJMP( sg_jmp_buf, 1 ) ;
}
