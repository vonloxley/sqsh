/*
 * cmd_input.c - Read and process input from data source
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
#include "sqsh_readline.h"
#include "sqsh_stdin.h"
#include "cmd.h"
#include "cmd_misc.h"
#include "cmd_input.h"

/*
 * Note about this module:
 *
 * This module is not a command in-and-of-itself, it is used by
 * any command that needs to read input from a data source and
 * act upon that source, in particular cmd_loop() relies heavily
 * on this to process its input.
 */

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_input.c,v 1.5 2010/01/28 15:30:37 mwesdorp Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes --*/
static void    input_sigint_jmp  _ANSI_ARGS(( int, void* ));
static char*   input_strchr      _ANSI_ARGS(( varbuf_t*, char*, int ));
static int     input_read        _ANSI_ARGS(( varbuf_t*, int ));
#if defined(USE_READLINE)
static int     DynKeywordLoad    _ANSI_ARGS(( void )); /* sqsh-2.1.8 - Feature dynamic keyword load */
#endif

/*
 * The following macro is used to determine if a line of input is
 * actually a comment string.
 */
#define IS_COMMENT(s) \
   ((*s) == '#' && \
     !(*((s)+1) == '_' || isdigit((int)*((s)+1)) || isalpha((int)*((s)+1))) )


/*
 * sg_jmp_buf: The following buffer is used to contain the location 
 *             to which this module will return upon receipt of a 
 *             SIGINT. It is only used while waiting on input from the
 *             user.
 */
static JMP_BUF sg_jmp_buf;

/*
 * sg_prompt_buf: This buffer is allocated once and kept around forever
 *             to hold the expanded version of the prompt. Fortunately,
 *             we are pretty much guarenteed that even through recursive
 *             calls to these functions this variable will be OK.
 */
static varbuf_t *sg_prompt_buf = NULL;

/*
 * sg_buf:     This is for those dopes that wanted ';' support and '!'
 *             history recall support:  For those commands that "derive"
 *             another command (i.e. ';' is converted to a "\go" and '!'
 *             is converted to a "\buf-append" command), this buffer is
 *             used to place the actual command string that is executed.
 */
static varbuf_t *sg_buf = NULL;

/*
 * cmd_input():
 *
 * Reads input from either input_file or input_buf, but not both.
 * By default, if input_file is not NULL it is used, otherwise
 * the contents of input_buf is used.
 */
int cmd_input()
{
    /*-- Variable Values --*/
    char        *semicolon_hack ;     /* Value of $semicolon_hack */
    char        *semicolon_cmd;       /* Value of $semicolon_cmd */
    char        *newline_go ;         /* Value of $newline_go */
    char        *history_shorthand ;  /* Value of $history_shorthand */
    char        *lineno ;             /* Value of $lineno */
#if defined(USE_READLINE)
    char        *keyword_dynamic ;    /* Value of $keyword_dynamic */
    char        *keyword_refresh ;    /* Value of $keyword_refresh */
#endif

    /*-- Misc --*/
    int          exit_status ;        /* Exit status of sub-command */
    JMP_BUF      old_jmp_buf ;        /* Store the previous jmp_buf */
    int          is_cmd ;             /* True if the current line is cmd */
    int          no_hist ;            /* True if hist should not be updt. */
    job_id_t     job_id ;             /* Id of job launched or completed */
    char        *defer_file ;         /* Name of file holding user output */
    struct stat  stat_buf ;           /* Check for defer file's existence */
    pid_t        job_pid ;            /* Pid of completed job */
    int          ret ;                /* Various return codes */
    char        *str ;                /* Generic pointer */
    char        *ch ;                 /* Generic pointer */
    char        number[50] ;          /* Generic buffer */
    int         cur_lineno;
    int         interactive;


    /*
     * Variables that need to be restored before turning to the
     * caller.
     */
    int       orig_lineno     = -1;
    varbuf_t *orig_sqlbuf     = NULL;
    varbuf_t *read_buf        = NULL;

    /*
     * Record whether or not we are interactive.
     */
    interactive = sqsh_stdin_isatty();

    /*
     * General note: from here on out, nothing should return
     * from this function, rather it should issue a 'goto' to
     * jump to the appropriate section of code to clean up
     * memory allocations and return to the caller.
     */

    /*
     * Ok, here we make a backup of the previous jump buffer so
     * that we can nest calls to cmd_loop.
     */
    memcpy((void*)&(old_jmp_buf),(void*)&(sg_jmp_buf),sizeof(JMP_BUF));

    /*
     * The following buffer is kept around to hold any commands
     * that may be derived by short-hand.  That is, ';' turns
     * into a '\go'.  Unfortunately, we need someplace to stick
     * the derived command, and this is it.
     */
    if (sg_buf == NULL)
    {
        if( (sg_buf = varbuf_create( 32 )) == NULL )
            goto loop_fail;
    }

    /*
     * Allocate the buffer in which we are reading input from the user.
     * This buffer should be destroyed when cmd_loop returns.
     */
    if ((read_buf = varbuf_create( 1024 )) == NULL)
    {
        goto loop_fail;
    }

    /*
     * Remember the previous line number that was being worked on
     * by the calling loop.
     */
    env_get( g_env, "lineno", &lineno );
    if( lineno != NULL )
        orig_lineno = atoi(lineno);
    else
        orig_lineno = 0;

    /*
     * Replace the global sql buffer with a new (empty) copy, which
     * will act as the new context for this loop.
     */
    orig_sqlbuf = g_sqlbuf;
    if ((g_sqlbuf = varbuf_create( 1024 )) == NULL)
    {
        fprintf( stderr, "varbuf_create: %s\n", sqsh_get_errstr() );
        goto loop_fail;
    }

    /*
     * Save current signal context.
     */
    sig_save();

    /*
     * Install a new location to jump to in case we are
     * interrupted.  This will return if we are not
     * in interactive mode.
     */
    if (SETJMP( sg_jmp_buf ) != 0)
    {
        if (!interactive)
        {
            goto loop_interrupt;
        }
    }
    sig_install( SIGINT, input_sigint_jmp, (void*)NULL, 0 );

    /*
     * Intialize whatever we need to.  It may seem strange that I am
     * using environment variables for many operations, this is so
     * these variables may be referenced in the prompt or some-such.
     */
    env_set( g_env, "lineno", "=1" );  /* Set lineno to 1 */
    varbuf_clear( g_sqlbuf );          /* Clear out the buffer */

    /*
     * Ok, now for the main loop.  This will essentially keep running
     * until we get an exit condition.
     */
    for (;;)
    {
#if defined(USE_READLINE)
        /*
         * sqsh-2.1.8 - Feature dynamic keyword load
         * If we are in interactive mode and we have keyword_dynamic enabled
         * then we want to do a refresh of the keyword list when the database
         * context is changed, i.e. a "use <database>" command was executed.
         */
        env_get( g_env,          "keyword_dynamic", &keyword_dynamic );
        env_get( g_internal_env, "keyword_refresh", &keyword_refresh );
        if (interactive &&  
            keyword_refresh != NULL && *keyword_refresh != '0' &&
            keyword_dynamic != NULL && *keyword_dynamic != '0')
        {
            (void) DynKeywordLoad();
            env_set( g_internal_env, "keyword_refresh", "0" );
        }
#endif

        no_hist = False;        /* Save history for this buffer */

        /*
         * Clear out the buffer that will be used to place input read
         * from the user.
         */
        varbuf_clear( read_buf );

        /*
         * If an input_file was supplied, or no input string was supplied
         * then call input_read.  By default, if input_read gets a NULL
         * input_file, stdin is used.
         */
        ret = input_read( read_buf, interactive );

        if (ret <= 0)
        {
            if (ret == 0)
            {
                goto loop_leave;
            }
            fprintf( stderr, "input: %s\n", sqsh_get_errstr() );
            goto loop_abort;
        }

        /*
         * Pull contents of read_buf out.
         */
        str = varbuf_getstr( read_buf );
        
        /*
         * The first thing we need to determine is if the current
         * line contains a sqsh command.  This information will be
         * used in several places.
         */
        if ((is_cmd = jobset_is_cmd( g_jobset, str )) == -1)
        {
            fprintf( stderr, "input: %s\n", sqsh_get_errstr() );
            goto loop_fail;
        }

        /*
         * Yet another hack...if we are in interactive mode, and
         * the current line we are dealing with starts with a '!'
         * then we pretend it is a buffer recall.  So, we turn
         * it into a logical call to \buf-append.
         */
        if (is_cmd == False && interactive && *str == '!' && 
            !isspace((int)*(str+1)))
         {

            /*
             * Check to see if the user has the history_shorthand feature
             * turned on.  If they don't then don't bother to continue
             * with this horrid logic.
             */
            env_get( g_env, "history_shorthand", &history_shorthand );

            if (history_shorthand != NULL && *history_shorthand == '1'  &&
                input_strchr( g_sqlbuf, str, '!' ) == str)
            {
                /*
                 * Create the buf-append call, specifying the destination
                 * buffer be the current work buffer (!.).
                 */
                varbuf_strcpy( sg_buf, "\\buf-append !. " );
                varbuf_strcat( sg_buf, str );

                /*
                 * And replace the current line input from the user
                 * with the new buf-append command.
                 */
                str = varbuf_getstr(sg_buf);
                is_cmd = True;
            }
        }

        /*
         * This next line is totally disgusting.  If semicolon_hack is
         * set to '1', and the current string is *not* a command, and
         * it contains a semicolon that is not in a string, then we have
         * to build us a "go" statement.  Note that I actually perform
         * two tests for the ';'.  The first strchr() determines if the
         * current line even contains the semicolon, and the second,
         * input_strchr(), returns the first ';' in str that will not
         * be contained in double quotes when str is appended to
         * g_sqlbuf.
         */
        env_get( g_env, "semicolon_hack", &semicolon_hack );
        if (semicolon_hack != NULL && *semicolon_hack == '1' && !is_cmd &&
            strchr( str, ';') != NULL && 
            (ch = input_strchr( g_sqlbuf, str, ';' )) != NULL)
        {
            /*
             * Copy everything up to the ';' into the current work buffer.
             */
            if (ch - str != 0)
            {
                varbuf_strncat( g_sqlbuf, str, ch - str );
                varbuf_charcat( g_sqlbuf, '\n' );

                /*
                 * We now have an extra line.
                 */
                env_set( g_env, "lineno", "+1" );
            }

            /*
             * Look up the name of the command that the user wishes
             * to use when a semicolon is encountered.
             */
            env_get( g_env, "semicolon_cmd", &semicolon_cmd );

            if (semicolon_cmd == NULL || *semicolon_cmd == '\0')
            {
                varbuf_strcpy( sg_buf, "\\go " );
            }
            else
            {
                varbuf_strcpy( sg_buf, semicolon_cmd );
                varbuf_charcat( sg_buf, ' ' );
            }

            /*
             * Now, stick the semicolon command in the front of everything
             * following the semicolon. and turn that into the command
             * line.
             */
            varbuf_strcat( sg_buf, ch + 1 );

            str = varbuf_getstr( sg_buf );
            is_cmd = True;
        }

        /*
         * And, to add to the long list of never-ending hacks.  The
         * following provides support for the particularly irritating
         * newline-go.  If $newline_go is set to 1, and the current
         * command line is empty, then we pretend a go was issued.
         */
        env_get( g_env, "newline_go", &newline_go );
        if (newline_go != NULL && *newline_go == '1' &&
            (*str == '\n' || *str == '\0') )
        {
            str = "\\go\n" ;   /* Cheesy!! */
            is_cmd = True;
        }

        /*
         * If this is a command, then we need to attempt to run it
         * otherwise we can just append the current line onto our
         * command buffer.
         */
        if (!is_cmd)
        {
            /*
             * Append the current line onto the end of the sql buffer
             * and increment the line number.
             */
            varbuf_strcat( g_sqlbuf, str );
            env_set( g_env, "lineno", "+1" );
        }
        else
        {
            /*
             * Before running command, save away current line number.
             */
            env_get( g_env, "lineno", &lineno );
            cur_lineno = atoi(lineno);

            /*
             * We start by assuming that the string the user typed in was
             * a command.  jobset_run() will differentiate between commands
             * and any other type of string for us.
             */
            switch ((job_id = jobset_run( g_jobset, str, &exit_status )))
            {
                /*
                 * Something went wrong with the whole process of
                 * attempting to launch the job.
                 */
                case -1 :
                    fprintf( stderr, "sqsh: %s\n", sqsh_get_errstr() );
                    break;

                /*
                 * If jobset_run() returns a 0 then str contained a command,
                 * and it was run in the foreground.  So, we need to pay
                 * attention to the exit status of the command.
                 */
                case 0 :

                    switch (exit_status)
                    {
                        case CMD_RETURN:
                            goto loop_return;

                        case CMD_BREAK:
                            goto loop_break;

                        case CMD_INTERRUPTED:
                            if (!interactive)
                            {
                                goto loop_interrupt;
                            }
                            /*
                             * FALLTHRU
                             */

                        /*
                         * CMD_RESETBUF: The command has requested that we clear
                         * our global buffer.  For this we simply clear g_sqlbuf
                         * and set lineno back to 1.
                         */
                        case CMD_RESETBUF:
                            /*
                             * Save the workbuffer away into the history. Note,
                             * if $interactive is set to 0, then this entry will
                             * automatically be thrown away.
                             */
                            if (!no_hist && interactive)
                            {
                                history_append( g_history, varbuf_getstr(g_sqlbuf) );

                                /*-- Set histnum to be current history number --*/
                                sprintf( number, "%d", history_get_nbr(g_history) );
                                env_set( g_env, "histnum", number );
                            }
                            varbuf_clear( g_sqlbuf );
                            env_set( g_env, "lineno", "=1" ) ;  /* Set to 1 */
                            break;

                        case CMD_CLEARBUF:
                            /*
			     * sqsh-2.1.7 - The same as CMD_RESETBUF but without
			     * saving the buffer to the history.
                             */
                            varbuf_clear( g_sqlbuf );
                            env_set( g_env, "lineno", "=1" ) ;  /* Set to 1 */
                            break;

                        /*
                         * CMD_FAIL & CMD_LEAVEBUF: don't do anything to the buffer.
                         * Commands such as '\set' should be returning these.
                         */
                        case CMD_FAIL :
                        case CMD_LEAVEBUF :
                            /*
                             * Restore line number.
                             */
                            sprintf( number, "=%d", cur_lineno );
                            env_set( g_env, "lineno", number );
                            break;

                        /*
                         * CMD_ALTERBUF : The command has altered g_sqlbuf in
                         * some fashion so, if we are in interactive mode, we
                         * need to print the buffer back out for the user.
                         * Commands such as '\edit' will return this.
                         */
                        case CMD_ALTERBUF :
                            if (interactive)
                                cmd_display( g_sqlbuf );
                            break;

                        /*
                         * CMD_EXIT: The command requested that we leave the
                         * read-eval-print loop.
                         */
                        case CMD_EXIT :
                            goto loop_exit;
                            break;

                        case CMD_ABORT :
                            goto loop_abort;
                            break;
                        

                        default :
                            sprintf( number, "=%d", cur_lineno );
                            env_set( g_env, "lineno", number );

                            fprintf( stderr, "Invalid exit status from command: %d\n",
                                        exit_status );
                    }

                    break;

                /*
                 * jobset_run() returned a non-negative value, so it launched
                 * a background process.  The only thing we need to do it
                 * let the user know it was launched.
		 * sqsh-2.1.7 - Also save and clear the command buffer.
                 */
                default :
                    if (interactive)
                    {
                        job_pid = jobset_get_pid( g_jobset, job_id );
                        fprintf( stdout, "Job #%d running [%d]\n", (int)job_id,
                                    (int)job_pid );
                        if (!no_hist)
                        {
                            history_append( g_history, varbuf_getstr(g_sqlbuf) );

                            /*-- Set histnum to be current history number --*/
                            sprintf( number, "%d", history_get_nbr(g_history) );
                            env_set( g_env, "histnum", number );
                        }
                        varbuf_clear( g_sqlbuf );
                        env_set( g_env, "lineno", "=1" ) ;  /* Set to 1 */
                    }

            } /* switch (jobset_run()) */

        } /* if (!is_cmd) */

        /*
         * Loop while there are jobs that have completed.  This way
         * we print out "Job complete" messages all at once rather
         * than each time the user hits return.
         */
        job_id = 0;
        while(interactive && 
            (job_id = jobset_wait(g_jobset, -1, &exit_status, JOB_NONBLOCK)) > 0)
        {
            /*
             * If something completed, we need to notify the user
             * of this.  Also, we need to check if the command has any output
             * to speak of.  If it didn't then we can go ahead and terminate
             * the job.
             */
            /*-- Get the name of the defer file --*/
            defer_file = jobset_get_defer( g_jobset, job_id );

            /*
             * If there is a defer_file, then we will stat the file
             * to get information on it.  If we can't even stat the
             * file, or if the file is of zero length, then we
             * pretend that there is no defer file.
             */
            if (defer_file != NULL)
            {
                if (stat( defer_file, &stat_buf ) == -1)
                    defer_file = NULL;
                else
                {
                    if( stat_buf.st_size == 0 )
                        defer_file = NULL;
                }
            }

            /*
             * If there is no defer file, then we can go ahead and end
             * the job and let the user know that it completed without
             * any output.
             */
            if (defer_file == NULL)
            {
                if (interactive)
                    fprintf(stdout, "Job #%d complete (no output)\n", job_id);

                if (jobset_end( g_jobset, job_id ) == False)
                {
                    fprintf( stderr, "\\loop: Unable to end job %d: %s\n",
                                (int)job_id, sqsh_get_errstr() );
                }
            }
            else
            {
                /*
                 * If there is output pending then we need to inform the
                 * user.  In this case we don't kill the job.  It is the
                 * responsibility of the user to run the appropriate
                 * command to terminate the job.
                 */
                if (interactive)
                {
                    fprintf( stdout, "Job #%d complete (output pending)\n",
                                job_id );
                }
            }

        } /* while(...) */

        if (job_id == -1)
            fprintf( stderr, "jobset_wait: %s\n", sqsh_get_errstr() );

    } /* for (;;) */

loop_return :
    ret = CMD_RETURN;
    goto loop_done;

loop_break :
    ret = CMD_BREAK;
    goto loop_done;

loop_interrupt :
    ret = CMD_INTERRUPTED;
    goto loop_done;

loop_abort :
    ret = CMD_ABORT;
    goto loop_done;

loop_fail :
    ret = CMD_FAIL;
    goto loop_done;

loop_exit :
    ret = CMD_EXIT;
    goto loop_done;

loop_leave :
    ret = CMD_LEAVEBUF;

loop_done :

    if (read_buf != NULL)
        varbuf_destroy( read_buf );
    
    /*
     * Restore the line number to its previous value.
     */
    if (orig_lineno != -1)
    {
        sprintf( number, "=%d", orig_lineno );
        env_set( g_env, "lineno", number );
    }

    /*
     * Restore our sql buffer back to its original status.
     */
    if (orig_sqlbuf != NULL)
    {
        varbuf_destroy( g_sqlbuf );
        g_sqlbuf = (varbuf_t*)orig_sqlbuf;
    }
    
    /*
     * Restore the original signal context, and, just in case
     * cmd_loop() has been recursively called, restore the original
     * jump buffer.
     */
    sig_restore();
    memcpy((void*)&(sg_jmp_buf),(void*)&(old_jmp_buf),sizeof(JMP_BUF));

    return ret;

    /* NOTREACHED */
}

/*
 * input_read():
 *
 * This function is responsible for reading a line of input from
 * the user. It automagically throws out comment lines, and deals
 * with line continuations.
 */
static int input_read( output_buf, interactive )
    varbuf_t  *output_buf;
    int        interactive;
{
    char   *prompt;
    char   *str;
    int     len;

    volatile int     is_continued = False;
    char            *exp_prompt   = NULL;

    /*
     * If we are in interactive mode then we need to display a 
     * prompt to the user.
     */
    if (interactive)
    {
        /*
         * If we haven't already allocated a buffer in which to
         * expand the prompt then we should do so.
	 * sqsh-2.1.6 - expand buffer from 32 to 64 bytes.
         */
        if (sg_prompt_buf == NULL)
        {
            if ((sg_prompt_buf = varbuf_create( 64 )) == NULL)
            {
                return -1;
            }
        }
    }

    /*
     * The following loop will continue until either EOF is hit,
     * or a line that does not contain a continuation is hit.
     */
    for (;;)
    {
        /*
         * If we are in interactive mode, then we need to display
         * a prompt back to the user.  The first time through this
         * loop we display the normal prompt, subsequent passes
         * cause the prompt2 to be displayed.
         */
        if (interactive)
        {
            if (!is_continued)
            {
                env_get( g_env, "prompt", &prompt );
                if( prompt == NULL || *prompt == '\0' ) 
                    prompt = "${lineno}> ";
            }
            else
            {
                env_get( g_env, "prompt2", &prompt );
                if( prompt == NULL || *prompt == '\0' ) 
                    prompt = "--> ";
            }

            /*
             * Now, expand the prompt of any environment variables
             * that it may contain.
             */
            if ((sqsh_expand( prompt, sg_prompt_buf, 0 )) == False)
            {
                fprintf( stderr, "prompt: %s\n", sqsh_get_errstr() );
                varbuf_strcpy( sg_prompt_buf, "?> " );
            }
            
            exp_prompt = varbuf_getstr(sg_prompt_buf);
        }
        else
            exp_prompt = NULL;

        /*
         * If the user supplied a file to read from then we request
         * the line of input from that file (which very well could
         * be stdin.
         */
        str = sqsh_readline( (char*)exp_prompt );

        /*
         * If str is NULL, then something went wrong.  If sqsh_get_error()
         * returns something then we generate an error message.  If
         * there is no error we reached EOF, so we can exit.
         */
        if (str == NULL)
        {
            if (sqsh_get_error() == SQSH_E_NONE)
            {
                return 0;
            }
            return -1;

        }
        else
        {
            /*
             * A comment is a string that starts with '#' and the next
             * character is *not* a valid character to start a temporary
             * table name, [_0-9A-Za-z].  If we hit a comment, then we
             * simply ignore this line, without even incrementing the
             * line number.
             */
            if (IS_COMMENT(str))
            {
                if (is_continued)
                {
                    break;
                }
                continue;
            }

            /*
             * Ack! I hate doing strlen if I can help it. I know that
             * it is a relatively fast operation, but a linear search
             * is terribly unappealing.
             */
            len = strlen( str );

            /*
             * Check to see if the last two characters on the line is
             * the escape sequence (\\), if it is then we append this
             * to the read buffer and read another line.
             */
            if (len >= 2 && str[len-2] == '\\' && str[len-3] == '\\')
            {
                if (varbuf_strncat( output_buf, str, len - 3 ) == -1)
                {
                    return -1;
                }
                is_continued = True;
                continue;
            }

            /*
             * This line doesn't look like there is anything special about
             * it so we can simply return it to the caller.
             */
            if (varbuf_strcat( output_buf, str ) == -1)
            {
                return -1;
            }
            break;
        }
    }

    return varbuf_getlen(output_buf);
}

/*
 * input_strchr():
 *
 * The following, very special purpose function, is used to return the
 * first occurance of c, contained in the str that is about to be appended
 * to varbuf, where c will not be contained in quotes (either single or
 * double) when str is appended to varbuf.
 */
static char* input_strchr( varbuf, str, c )
    varbuf_t   *varbuf;
    char       *str;
    int         c;
{
#define   QUOTE_NONE            0
#define   QUOTE_SINGLE          1
#define   QUOTE_DOUBLE          2
#define   QUOTE_COMMENT         3

    char   *cptr;
    int     quote_type      = QUOTE_NONE;

    cptr = varbuf_getstr(varbuf);

    if (str == NULL || cptr == NULL)
    {
        return NULL;
    }

    /*-- Blast through varbuf --*/
    for (; *cptr != '\0'; ++cptr)
    {
        switch (quote_type)
        {
            case QUOTE_NONE:
                switch (*cptr)
                {
                    case '\'': 
                        quote_type = QUOTE_SINGLE;
                        break;
                    case '\"': 
                        quote_type = QUOTE_DOUBLE;
                        break;
                    case '/' : 
                        if (*(cptr + 1) == '*')
                            quote_type = QUOTE_COMMENT;
                        break;
                    case '-':
                        if (*(cptr + 1) == '-')
                        {
                            while (*cptr != '\0' && *cptr != '\n')
                                ++cptr;

                            if (*cptr == '\0')
                                --cptr;
                        }
                        break;
                    default:
                        break;
                }
                break;
            
            case QUOTE_COMMENT:
                if (*cptr == '*' && *(cptr + 1) == '/')
                {
                    quote_type = QUOTE_NONE;
                }
                break;
            
            case QUOTE_SINGLE:
                if (*cptr == '\'')
                {
                    quote_type = QUOTE_NONE;
                }
                break;
            
            case QUOTE_DOUBLE:
                if (*cptr == '\"')
                {
                    quote_type = QUOTE_NONE;
                }
                break;
            
            default:
                break;
        }
    }

    /*-- Blast through str --*/
    for (cptr = str; *cptr != '\0'; ++cptr)
    {
        if ((*cptr == c) && (quote_type == QUOTE_NONE))
            break;

        switch (quote_type)
        {
            case QUOTE_NONE:
                switch (*cptr)
                {
                    case '\'': 
                        quote_type = QUOTE_SINGLE;
                        break;
                    case '\"': 
                        quote_type = QUOTE_DOUBLE;
                        break;
                    case '/' : 
                        if (*(cptr + 1) == '*')
                            quote_type = QUOTE_COMMENT;
                        break;
                    case '-':
                        if (*(cptr + 1) == '-')
                        {
                            while (*cptr != '\0' && *cptr != '\n')
                                ++cptr;

                            if (*cptr == '\0')
                                --cptr;
                        }
                        break;
                    default:
                        break;
                }
                break;
            
            case QUOTE_COMMENT:
                if (*cptr == '*' && *(cptr + 1) == '/')
                {
                    quote_type = QUOTE_NONE;
                }
                break;
            
            case QUOTE_SINGLE:
                if (*cptr == '\'')
                {
                    quote_type = QUOTE_NONE;
                }
                break;
            
            case QUOTE_DOUBLE:
                if (*cptr == '\"')
                {
                    quote_type = QUOTE_NONE;
                }
                break;
            
            default:
                break;
        }
    }

    if( *cptr == c )
        return cptr;
    return NULL;
}

/*
 * input_sigint_jmp():
 *
 * Used to catch ^C's from the user.  If there is currently a database
 * action in progress it is canceled..
 */
static void input_sigint_jmp( sig, user_data )
    int sig;
    void *user_data;
{
    LONGJMP( sg_jmp_buf, 1 );
}

#if defined(USE_READLINE)
/*
 * Function: DynKeywordLoad()
 *
 * sqsh-2.1.8 - Dynamically execute a query provided by the variable keyword_query
 * and load the result set into the readline autocompletion list.
 * By default the query is "select name from sysobjects order by name"
 * But of course you can change this to anything you like as long as the result set
 * contains a first column with character data. The variable keyword_query can be
 * defined in your .sqshrc file for example or in a sqsh_session file.
 *
 */
static int DynKeywordLoad ()
{
    CS_COMMAND *cmd;
    CS_CHAR    *keyword_query;
    CS_DATAFMT  columns[1];
    CS_RETCODE  ret;
    CS_RETCODE  results_ret;
    CS_INT      result_type;
    CS_INT      count;
    CS_INT      idx;
    CS_INT      datalength[1];
    CS_SMALLINT indicator [1];
    CS_CHAR     name   [256];


    env_get( g_env, "keyword_query", &keyword_query );
    if ( keyword_query == NULL || *keyword_query == '\0' ) {
        DBG(sqsh_debug(DEBUG_ERROR, "DynKeywordLoad: Variable keyword_query is empty.\n"));
        return (CS_FAIL);
    }
    if ( g_connection == NULL )
    {
        DBG(sqsh_debug(DEBUG_ERROR, "DynKeywordLoad: g_connection is not initialized.\n"));
        return (CS_FAIL);
    }
    if (ct_cmd_alloc( g_connection, &cmd ) != CS_SUCCEED)
    {
        DBG(sqsh_debug(DEBUG_ERROR, "DynKeywordLoad: Call to ct_cmd_alloc failed.\n"));
        return (CS_FAIL);
    }
    if (ct_command( cmd,                /* Command Structure */
                    CS_LANG_CMD,        /* Command Type      */
                    keyword_query,      /* Buffer            */
                    CS_NULLTERM,        /* Buffer Length     */
                    CS_UNUSED           /* Options           */
                  ) != CS_SUCCEED) 
    {
        ct_cmd_drop( cmd );
        DBG(sqsh_debug(DEBUG_ERROR, "DynKeywordLoad: Call to ct_command failed.\n"));
        return (CS_FAIL);
    }
    if (ct_send( cmd ) != CS_SUCCEED) 
    {
        ct_cmd_drop( cmd );
        DBG(sqsh_debug(DEBUG_ERROR, "DynKeywordLoad: Call to ct_send failed.\n"));
        return (CS_FAIL);
    }

    while ((results_ret = ct_results(cmd, &result_type)) == CS_SUCCEED)
    {
        switch ((int) result_type)
        {
            case CS_ROW_RESULT:
                columns[0].datatype  = CS_CHAR_TYPE;
                columns[0].format    = CS_FMT_NULLTERM;
                columns[0].maxlength = 255;
                columns[0].count     = 1;
                columns[0].locale    = NULL;
                ct_bind(cmd, 1, &columns[0], name, &datalength[0], &indicator[0]);

                sqsh_readline_clear();

                while ( ct_fetch (cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count) == CS_SUCCEED )
                {
                    /* Remove trailing blanks, tabs and newlines, just in case */
                    for ( idx = strlen(name) - 1;
                          idx >= 0 && (name[idx] == ' ' || name[idx] == '\t' || name[idx] == '\n');
                          name[idx--] = '\0');
                    sqsh_readline_add (name);    /* Add name to readline linked list of keywords */
                }
                break;

            case CS_CMD_SUCCEED:
                DBG(sqsh_debug(DEBUG_ERROR, "DynKeywordLoad: No rows returned from query.\n"));
                ret = CS_FAIL;
                break;

            case CS_CMD_FAIL:
                DBG(sqsh_debug(DEBUG_ERROR, "DynKeywordLoad: Error encountered during query processing.\n"));
                ret = CS_FAIL;
                break;

            case CS_CMD_DONE:
                break;

            default:
                DBG(sqsh_debug(DEBUG_ERROR, "DynKeywordLoad: Unexpected error encountered. (1)\n"));
                ret = CS_FAIL;
                break;
        }
    }

    switch ((int) results_ret)
    {
        case CS_END_RESULTS:
            ret = CS_SUCCEED;
            break;

        case CS_FAIL:
            DBG(sqsh_debug(DEBUG_ERROR, "DynKeywordLoad: Unexpected error encountered. (2)\n"));
            ret = CS_FAIL;
            break;

        default:
            DBG(sqsh_debug(DEBUG_ERROR, "DynKeywordLoad: Unexpected error encountered. (3)\n"));
            ret = CS_FAIL;
            break;
    }
    ct_cmd_drop( cmd );
    return ( ret );
}
#endif
