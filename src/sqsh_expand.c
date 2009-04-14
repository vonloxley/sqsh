/*
 * sqsh_expand.c - Routines for expanding environment variables
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
 * You may contact the author:
 *   e-mail:  gray@voicenet.com
 *            grays@xtend-tech.com
 *            gray@xenotropic.com
 */
#include <stdio.h>
#include <ctype.h>
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_varbuf.h"
#include "sqsh_env.h"
#include "sqsh_global.h"
#include "sqsh_fd.h"
#include "sqsh_expand.h"
#include "sqsh_strchr.h"
#include "sqsh_sig.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_expand.c,v 1.3 2004/11/04 19:47:25 mpeppler Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Different quotes --*/
#define QUOTE_NONE           1
#define QUOTE_DOUBLE         2
#define QUOTE_SINGLE         3

/*-- Local prototypes --*/
static int  expand_variable     _ANSI_ARGS(( char**, char*, varbuf_t*, int ));
static int  expand_column       _ANSI_ARGS(( char**, char*, varbuf_t*, int ));
static int  expand_escape       _ANSI_ARGS(( char**, char*, varbuf_t*, int ));
static int  expand_command      _ANSI_ARGS(( char**, char*, varbuf_t*, int ));
static int  expand_skip_eol     _ANSI_ARGS(( char**, char*, varbuf_t*, int ));
static int  expand_skip_comment _ANSI_ARGS(( char**, char*, varbuf_t*, int ));
static void expand_sighandler   _ANSI_ARGS(( int, void* ));

int sqsh_expand( str, buf, flags )
    char     *str;
    varbuf_t *buf;
    int       flags;
{
    return sqsh_nexpand( str, buf, flags, EXP_EOF );
}

/*
 * sqsh_expand():
 *
 * This function replaces any $variable names in the supplied str with
 * environment variables contained in the global environment, placing the
 * final results in buf.  It attempts to follow some of the basic rules
 * of shells.  If a variable is contained within double quotes it is still
 * expanded (although, unlike a shell it doesn't strip the quotes), if a
 * variable is contained in double quotes it is not expanded.  sqsh_expand()
 * returns True upon success, and False if there is some sort of error.
 */
int sqsh_nexpand( str, buf, flags, n )
    char     *str;
    varbuf_t *buf;
    int       flags;
    int       n;
{
    int         quote_type = QUOTE_NONE ; /* Which quotes are we in? */
    int         r ;                       /* Results of varbuf_() functions */
    char       *str_end;
    int         leading_whitespace = True;
    DBG(char   *instr;)

    DBG(instr = str;)

    /*-- Clear out the expansion buffer --*/
    varbuf_clear( buf );

    if (n == EXP_EOF || n == EXP_WORD)
        str_end = NULL;
    else
        str_end = (str + n);

    while ((str_end == NULL || str < str_end) && *str != '\0')
    {
        /*
         * If the user requested the we only expand up to the first
         * word in the str...
         */
        if (n == EXP_WORD)
        {
            /*
             * If we are sitting in leading white space then just ignore
             * it, otherwise, if hit something other than whitespace
             * then mark ourselves to be in such a condition.
             */
            if (leading_whitespace)
            {
                if (isspace((int)*str))
                {
                    ++str;
                    continue;
                }
                leading_whitespace = False;
            }
            else
            {
                /*
                 * If we have already hit something other than whitespace
                 * and we hit whitespace again, then we have completed
                 * parsing the word.
                 */
                if (quote_type == QUOTE_NONE && isspace((int)*str))
                    return True;
            }
        }

        r = 0;
        switch (*str)
        {
            case '-':
                if (quote_type == QUOTE_NONE &&
                    (flags & EXP_COMMENT) != 0 && *(str+1) == '-')
                {
                    expand_skip_eol( &str, str_end, buf, flags );
                }
                else
                {
                    r = varbuf_charcat( buf, *str );
                    ++str;
                }
                break;

            case '/':
                if (quote_type == QUOTE_NONE &&
                    (flags & EXP_COMMENT) != 0 && *(str+1) == '*')
                {
                    expand_skip_comment( &str, str_end, buf, flags );
                }
                else
                {
                    r = varbuf_charcat( buf, *str );
                    ++str;
                }
                break;

            /*
             * If we are requested to strip newlines, then replace them
             * with a blank space.
             */
            case '\n':
            case '\r':
                if( quote_type == QUOTE_NONE && (flags & EXP_STRIPNL) )
                    r = varbuf_charcat( buf, ' ' );
                else
                    r = varbuf_charcat( buf, *str );
                ++str;
                break;

            /*
             * Determine if we are contained within single quotes.  If we
             * are then paramters no longer get expanded.
             */
            case '\'':
                /*
                 * If we are in double quotes, then we go ahead and copy the
                 * single quote in the dest buffer and leave it at that.
                 */
                if (quote_type == QUOTE_DOUBLE)
                {
                    r = varbuf_charcat( buf, *str++ );
                    break;
                }

                /*
                 * Unless requested to do otherwise, go ahead and stick
                 * the quote in the destination buffer.
                 */
                if (!(flags & EXP_STRIPQUOTE))
                    r = varbuf_charcat( buf, *str );
                
                /*
                 * If we are in already in single quotes, then mark outselves
                 * as no longer being in them. Otherwise, we are currently
                 * in single quotes.
                 */
                quote_type = (quote_type==QUOTE_SINGLE)?QUOTE_NONE:QUOTE_SINGLE;
                ++str;
                break;

            /*
             * Determine if we are contained within single quotes.  If we
             * are then paramters no longer get expanded.
             */
            case '\"':
                /*
                 * If we are in single quotes, then we go ahead and copy the
                 * double quote in the dest buffer and leave it at that.
                 */
                if (quote_type == QUOTE_SINGLE)
                {
                    r = varbuf_charcat( buf, *str++ );
                    break;
                }

                /*
                 * Unless requested to do otherwise, go ahead and stick
                 * the quote in the destination buffer.
                 */
                if (!(flags & EXP_STRIPQUOTE))
                    r = varbuf_charcat( buf, *str );
                
                /*
                 * If we are in already in double quotes, then mark outselves
                 * as no longer being in them. Otherwise, we are currently
                 * in double quotes.
                 */
                quote_type = (quote_type==QUOTE_DOUBLE)?QUOTE_NONE:QUOTE_DOUBLE;
                ++str;
                break;
            
            /*
             * Look for the command substitution character.
             */
            case '`':
                /*
                 * If we are inside single quotes, or the person calling
                 * us requested that we ignore command substitution then
                 * just copy the backquote into the buffer and continue
                 * on our merry way.
                 */
                if (quote_type == QUOTE_SINGLE || flags & EXP_IGNORECMD)
                {
                    r = varbuf_charcat( buf, *str++ );
                    break;
                }

                if (expand_command( &str, str_end, buf, flags ) == False)
                    return False;
                break;

            /*
             * Look for an escape character.  This allows the user to
             * insert a $ into a buffer without having it interpreted.
             */
            case '\\':
                /*
                 * If we are inside a single quote, or the user has requested
                 * that we always keep the escape character, then copy the
                 * escape character into the buffer, otherwise skip it.
                 */
                if (quote_type == QUOTE_SINGLE)
                {
                    r = varbuf_charcat( buf, *str++ );
                    break;
                }

                if (expand_escape( &str, str_end, buf, flags ) == False)
                    return False;
                break;

            /*
             * The biggie.  We seem to have found the name of a variable
             * that needs to be expanded.  If this is the case, then
             * we need to read the variable name into a buffer first.
             */
            case '$':
                /*
                 * If we are inside of single quotes then keep the $ and treat
                 * it just as we would any other character.
                 */
                if (quote_type == QUOTE_SINGLE)
                {
                    r = varbuf_charcat( buf, *str++ );
                    break;
                }

                if (expand_variable( &str, str_end, buf, flags ) == False)
                    return False;
                break;

            case '#':
                /*
                 * If we are inside of single quotes then keep the : and treat
                 * it just as we would any other character.
                 */
                if (quote_type == QUOTE_SINGLE)
                {
                    r = varbuf_charcat( buf, *str++ );
                    break;
                }

                /*
                ** Only expand columns when there are actually
                ** columns available to be expanded (this may
                ** protect us against expanding weird temp-table
                ** names.
                */
                if ((flags & EXP_COLUMNS) != 0 && g_do_ncols > 0)
                {
                    r = 0;
                    if (expand_column( &str, str_end, buf, flags ) == False)
                        return False;
                }
                else
                {
                    r = varbuf_charcat( buf, *str++ );
                }
                break;


            default:
                r = varbuf_charcat( buf, *str++ );
        }

        /*
         * As we blast through this loop we perform some sort of operation
         * on the varbuf structure, storing the results of the operation
         * in r.  Here we check to see if something blew up.
         */
        if (r == -1)
        {
            sqsh_set_error( sqsh_get_error(), "varbuf: %s",
                            sqsh_get_errstr() );
            return False;
        }
    }

    DBG(sqsh_debug( DEBUG_EXPAND, "sqsh_nexpand: '%s' -> '%s'\n",
                    (char*)instr, varbuf_getstr(buf) );)

    sqsh_set_error( SQSH_E_NONE, NULL );
    return True;
}

/*
 * expand_skip_eol():
 *
 * Copy everything up to the current end of line into the 
 * expand buffer.
 */
static int expand_skip_eol( cpp, str_end, buf, flags )
    char    **cpp;
    char     *str_end;
    varbuf_t *buf;
    int       flags;
{
    char     *str;

    str       = *cpp;

    /*
     * Track down the end of line.
     */
    while (str != str_end && *str != '\n')
    {
        ++str;
    }

    if (*str == '\n')
    {
        ++str;
    }

    varbuf_strncat( buf, *cpp, str - (*cpp) );

    *cpp = str;
    return True;
}

/*
 * expand_skip_comment():
 *
 * Copy the contents of a comment into the buffer.
 */
static int expand_skip_comment( cpp, str_end, buf, flags )
    char    **cpp;
    char     *str_end;
    varbuf_t *buf;
    int       flags;
{
    char     *str;

    str       = *cpp;

    /*
     * Track down the end of comment.
     */
    while (*str && (str_end == NULL || str != (str_end-1)) && 
        (*str != '*' || *(str+1) != '/'))
    {
        ++str;
    }

    /* if we're not at the end of the string (CR 1046570)*/
    if(*str) {
	/*
	 * If we failed to find a closing comment, then
	 * we stopped at the end of line.
	 */
	if (*str != '*' || *(str+1) != '/')
	{
	    ++str;
	}
	else
	{
	    str += 2;
	}
    }

    varbuf_strncat( buf, *cpp, str - (*cpp) );
    *cpp = str;

    return True;
}

/*
 * expand_command():
 *
 * This function expectes to receieve a pointer to the current parsing
 * location into command buffer (cpp), a pointer to the end of the
 * buffer (str_end), which may be NULL if the string is NULL terminated,
 * a desination buffer (buf), and the current set of parsing flags. cpp
 * should be pointing a ` character.
 * 
 */
static int expand_command( cpp, str_end, buf, flags )
    char    **cpp;
    char     *str_end;
    varbuf_t *buf;
    int       flags;
{
    char     *str ;        /* Pointer to current location in cpp */
    char     *cmd_start ;  /* Pointer into str for start of command */
    int       cmd_fd ;     /* File descriptor to pipe talking to command */
    FILE     *cmd_file ;   /* A FILE version of the cmd_fd */
    varbuf_t *cmd ;        /* Expanded version of the command */
    int      first_word = True;
    char     *ifs;
    int       ch;
    int       loc_errno;
    int       got_signal;

    /*
     * Skip the backquote.
     */
    str       = (*cpp) + 1;
    cmd_start = str;

    /*
     * Look for the closing quote.
     */
    if (str_end == NULL)
        str = sqsh_strchr( str, '`' );
    else
        str = sqsh_strnchr( str, '`', str_end - str );

    /*
     * If str is NULL it is because (1) the character didn't exist
     * or (2) there was some other error (probably an unbounded
     * quote).
     */
    if (str == NULL)
    {
        if (sqsh_get_error() == SQSH_E_NONE)
            sqsh_set_error( SQSH_E_BADQUOTE, "Unbounded ` character" );
        return False;
    }
    
    /*
     * Create a buffer to hold the soon-to-be-expanded
     * command string.
     */
    if ((cmd = varbuf_create(64)) == NULL)
    {
        sqsh_set_error( sqsh_get_error(), "varbuf: %s",
                             sqsh_get_errstr() );
        return False;
    }

    /*
     * Expand the contents of the command to be run into our work
     * buffer.  While we go we want to strip out any newlines we
     * may encounter along the way as well as any escape sequences
     * (so they may be interpreted by the underlying shell).
     */
    if (sqsh_nexpand( cmd_start, cmd, flags|EXP_STRIPNL|EXP_STRIPESC, 
        (str - cmd_start) ) == False)
    {
        varbuf_destroy( cmd );
        return False;
    }

    DBG(sqsh_debug( DEBUG_EXPAND, "expand_command: Executing '%s'\n",
                    varbuf_getstr(cmd) );)

    /*
     * Ok, we have expanded the command to be run, so now all
     * that is left is to run it and process the results.
     */
    if ((cmd_fd = sqsh_popen( varbuf_getstr(cmd), "r", NULL, NULL )) == -1)
    {
        varbuf_destroy( cmd );
        return False;
    }

    /*
     * Now that we have launched the command we don't need to keep
     * around the string that started it.
     */
    varbuf_destroy(cmd);

    /*
     * Turn our pipe file descriptor into a FILE buffer so we can
     * get a little performance while we are reading from input.
     */
    if( (cmd_file = fdopen( cmd_fd, "r" )) == NULL ) {
        loc_errno = errno;

        sqsh_close( cmd_fd );
        sqsh_set_error( loc_errno, NULL );
        return False;
    }

    /*
     * Check to see if the user has declared a replacement internal
     * file separator for us to use.
     */
    env_get( g_env, "ifs", &ifs );
    if( ifs == NULL || *ifs == '\0' )
        ifs = "\f\n\r\t\v";
    
    /*
     * Because this expansion could theoretically take a while
     * we want to protect ourselves agains recieving an interrupt
     * while we are reading from the input. We need to make
     * sure that we can clean up if we get one.
     */
    ch          = EOF;
    got_signal  = 0;

    sig_save();
    sig_install( SIGINT, expand_sighandler, (void*)&got_signal, 0 );

    /*
     * Now we are ready to begin reading from the input file.  For
     * now I am going to assume that IFS is a local variable, later
     * it may be worthwhile moving it out to the environment.
     */
    do {

        /*
         * Consume any character from input that are considered white-space
         * (as determined by our ifs variable).
         */
        while (got_signal == 0 && (ch = sqsh_getc( cmd_file )) != EOF ) {
            if (strchr( ifs, ch ) == NULL)
                break;
        }

        /*
         * If we are parsing the very first word being returned and
         * we haven't gotten a signal or hit EOF, then set first_word
         * to False.  If we have already read at least one word then
         * place a space following the previous word read.
         */
        if( first_word )
            first_word = False;
        else {
            if( got_signal == 0 && ch != EOF )
                varbuf_charcat( buf, ' ' );
        }
        
        /*
         * While we haven't reached EOF, and we haven't hit an IFS
         * character start copying character into the buffer.
         */
        while( got_signal == 0 && ch != EOF && strchr(ifs, ch) == NULL ) {
            varbuf_charcat( buf, ch );
            ch = sqsh_getc( cmd_file );
        }

    } while( got_signal == 0 && ch != EOF );

    /*
     * Restore the original signal handler.
     */
    sig_restore();

    /*
     * First, we need to close our file descriptor to make sure
     * that the process dies, then we can go ahead and close our
     * file structure.
     */
    sqsh_close( cmd_fd );
    fclose( cmd_file );

    *cpp = str + 1;
    return True;
}

static int expand_variable( cpp, str_end, buf, flags )
    char    **cpp;
    char     *str_end;
    varbuf_t *buf;
    int       flags;
{
    char       *str;
    char       *var_start;           /* Pointer to $ in variable name */
    char       *var_name_start;      /* Pointer to char after the $ */
    char       *var_name_end;        /* Pointer to end of variable name */
    char       *var_value;           /* Results of env_get() */
    int         in_brace;
    char        nbr[32];
    char       *cp;
    int         arg_nbr;
    int         r;
    int         i;
    int         all_digits = True;

    str = *cpp;

    /*
     * This is a special case to deal with money date-types
     * in a select statement.  If we see a '$' followed
     * by zero or more white-space, followed by a digit, then
     * it is probably a money datatype, so don't touch the
     * '$'
     */
    for (var_start = str + 1 ; var_start != str_end &&
          *var_start != '\0' && isspace((int)*var_start); ++var_start);

    if (isdigit((int)*var_start))
    {
        if (varbuf_charcat( buf, *str++ ) == -1)
            return False;
        *cpp = str;
        return True;
    }

    /*-- Keep around index of the '$' --*/
    var_start = str++;

    /*-- Look for a curley brace --*/
    if (str != str_end && *str == '{')
    {
        in_brace = True;
        ++str;
    }
    else
    {
        in_brace = False;
    }

    /*
     * Keep around index of the beginning of the name of
     * the variable.
     */
    var_name_start = str;

    /*
     * Move str forward until we reach the end of the name of
     * the variable.  We are still in part of the name if we
     * see the characters [?a-zA-Z0-9_].
     */
    while (!(str_end == NULL && str == str_end) && *str != '\0' && 
        (isalnum((int)*str) || *str == '_' || *str == '?' || *str == '#'))
    {
        if (!isdigit((int)*str))
        {
            all_digits = False;
        }
        ++str;
    }

    /*
     * Keep track of where the variable ends, this may not be
     * equal to the value of str if the next character is a 
     * close brace.
     */
    var_name_end = str;

#if 0
    /* FIXME - this needs testing! */
    /*
     * patch from Michael Chepelev:
     * To avoid coredumps in we enter $$, $., $, and some other sequences
     * we check if the name string is empty. If it is -
     * then keep the $ and treat it just as we would any other character.
     */
    if( var_name_start == var_name_end )
    {
	if (varbuf_charcat( buf, *str++ ) == -1)
	    return False;
	*cpp = str;
	return True;
    }
#endif

    /*
     * If we are in a braced variable name then we better have
     * hit a closing brace.
     */
    if (in_brace)
    {
        if ((str_end != NULL && str == str_end) || *str != '}')
        {
            sqsh_set_error( SQSH_E_BADVAR, NULL );
            return False;
        }
        ++str;
    }

    /*
     * Check for special case. First, $# is the number of arguments.
     */
    if (*var_name_start == '#' && 
        (var_name_end - var_name_start) == 1)
    {
        /*
         * If there are no available arguments, then return
         * a value of 0.
         */
        if (g_func_nargs == 0)
        {
            varbuf_charcat( buf, '0' );
        }
        else
        {
            sprintf(nbr, "%d", g_func_args[g_func_nargs-1].argc - 1 );
            varbuf_strcat( buf, nbr );
        }
        *cpp = str;
        return(True);
    }

    /*
     * Next, $* is the complete list of arguments.
     */
    if (*var_name_start == '*' && 
        (var_name_end - var_name_start) == 1)
    {
        if (g_func_nargs > 0)
        {
            for (i = 1; i < g_func_args[g_func_nargs-1].argc; i++)
            {
                if (i != 1)
                {
                    varbuf_charcat( buf, ' ' );
                }
                varbuf_strcat( buf, g_func_args[g_func_nargs-1].argv[i] );
            }
        }
    }

    /*
     * Next special case. All digits point us to a function 
     * argument.
     */
    if (all_digits == True)
    {
         for (i = 0, cp = var_name_start; cp < var_name_end; ++i, ++cp)
         {
             nbr[i] = *cp;
         }
         nbr[i] = '\0';

         arg_nbr = atoi(nbr);

        /*
         * If invalid argument number, then just leave blank.
         */
        if (arg_nbr < 0 || 
            arg_nbr > g_func_args[g_func_nargs-1].argc)
        {
            *cpp = str;
            return(True);
        }

        varbuf_strcat( buf, g_func_args[g_func_nargs-1].argv[arg_nbr] );
        *cpp = str;
        return(True);
    }

    /*
     * First, check to see if the variable is available in our
     * "extneral" environment.
     */
    r = env_nget( g_env, var_name_start, &var_value, 
        var_name_end - var_name_start);

    if (r == -1)
    {
        sqsh_set_error( sqsh_get_error(), "env_nget: %s",
                             sqsh_get_errstr() );
        return False;
    }

    if (r == 0)
    {
        /*
         * Failing this, we look to the internal environment for
         * wisdom.
         */
        if( (r = env_nget( g_internal_env, var_name_start, &var_value,
                                 var_name_end - var_name_start)) == -1 ) {
            sqsh_set_error( sqsh_get_error(), "env_nget: %s",
                                 sqsh_get_errstr() );
            return False;
        }
    }

    /*
     * If var_value is not NULL, then we have expanded.
     */
    if( r != 0 && var_value != NULL ) {
        if( varbuf_strcat( buf, var_value ) == -1 )
            return False;
    }
    
    *cpp = str;
    return True;
}

static int expand_column( cpp, str_end, buf, flags )
    char    **cpp;
    char     *str_end;
    varbuf_t *buf;
    int       flags;
{
    char       *str;
    char        number[6];
    int         i;
    int         depth = 0;
    int         col;
    dsp_desc_t *desc;

    str = *cpp;

    /*
    ** For each occurrance of a '#', we are referring one more
    ** level back in the \do hierarchy.
    */
    while ((str_end == NULL || str < str_end) &&
        *str == '#')
    {
        ++str;
        ++depth;
    }

    for (i = 0; (str_end == NULL || str < str_end) &&
        *str != '\0' && i < (sizeof(number)-1) && isdigit((int)*str); ++i, ++str)
    {
        number[i] = *str;
    }
    number[i] = '\0';

    /*
    ** If the number is empty (that is, we didn't have any digits)
    ** or the current position on the input is not either EOL or
    ** a white-space, then this is a malformed expression, so we
    ** leave it alone.
    */
    if (number[0] == '\0' ||
        (str_end == NULL && *str != '\0' && isalpha((int)*str)) ||
        (str_end != NULL && str < str_end && !isalpha((int)*str)))
    {
        varbuf_strncat( buf, *cpp, (str - (*cpp)) );
        *cpp = str;
        return True;
    }

    /*
    ** Now we know which column context we are talking about
    ** and which column in that context.
    */
    col    = atoi(number);

    /*
    ** Let the caller know where to pick up.
    */
    *cpp = str;

    /*
    ** If we are referring to an illegal depth or column then
    ** we are done (no expansion).
    */
    if (g_do_ncols == 0 || depth > g_do_ncols || col <= 0 ||
        col > g_do_cols[g_do_ncols-depth]->d_ncols)
    {
        return(True);
    }

    desc = g_do_cols[g_do_ncols-depth];

    /*
    ** Copy the value of the column in.
    */
    if (varbuf_strcat( buf, desc->d_cols[col-1].c_data ) == -1)
    {
        return False;
    }

    return True;
}

static int expand_escape( cpp, str_end, buf, flags )
    char    **cpp;
    char     *str_end;
    varbuf_t *buf;
    int       flags;
{
    char *str;

    str = *cpp;

    /*
     * If the next character isn't a \, or the next character is
     * then end of the string, then simply copy the \ into the
     * destination buffer, and return.
     */
    if( (*(str+1) != '\\') || (str_end != NULL && (str+1) == str_end) ) {
        if( (varbuf_charcat( buf, *str ) == -1) )
            return False;
        *cpp = str + 1;
        return True;
    }

    /*
     * If we have been requested not to strip the escape sequence
     * then copy the \\x into the buffer and continue.
     */
    if( !(flags & EXP_STRIPESC) ) {
        if( varbuf_strncat( buf, str, 2 ) == -1 )
            return False;
    }
    
    /*
     * Skip straight to the escaped character.
     */
    str += 2;

    /*
     * If we are escaping a new-line or end-of-string, then
     * just throw it out, otherwise place the escaped character
     * literally in the destination buffer and continue.
     */
    if( (str_end == NULL || str != str_end) &&
          *str != '\0' && *str != '\n' ) {
        if( varbuf_charcat( buf, *str ) == -1 )
            return False;
    } 
    ++str;

    *cpp = str;
    return True;
}

/*
 * expand_sighandler():
 *
 * A rather generic signal handler used to trap the most recent
 * signal that has occured.
 */
static void expand_sighandler( sig, user_data )
    int   sig;
    void *user_data;
{
    *((int*)user_data) = sig;
}


/*
 * sqsh-2.1.6 feature - expand_color_prompt()
 *
 * Expand the prompt with color definitions to a static buffer and return the buffer
 * address to the caller.
 *
 * Parameters: prompt   - char pointer to a string containing the unexpanded prompt
 *             readline - Boolean switch: will the prompt string be passed on to
 *                        the readline library yes or no. If yes, then the colorcodes
 *                        also have to be encapsulated in between \001 and \002.
 *
 * The prompt string may contain colorcodes in the format {a;b;c}. This must be expanded
 * to a string "\033[a;b;cm".
 *   Color definitions byte a,b,c
 *   The first code defines the Color Attribute Code with possible values:
 *     00=none 01=bold
 *   The second value defines the Text Color Code:
 *     30=black 31=red 32=green 33=yellow 34=blue 35=magenta 36=cyan 37=white
 *   The third value defines the Background Color Code:
 *     40=black 41=red 42=green 43=yellow 44=blue 45=magenta 46=cyan 47=white
 *
 * If a curly bracket is part of the prompt string it must be escaped by a double brace,
 * for example {{....}}. No other checks are performed if an opening brace is closed and
 * if the specified color values are valid.
 */
#define PROMPTSZ 512
#define OVFLSZ     4

char *expand_color_prompt( prompt, readline )
    char    *prompt;
    int      readline;
{
    static   char buf [PROMPTSZ+OVFLSZ];
    register int  i, j;


    if ( prompt == NULL )
        return NULL;

    i = 0;
    j = 0;
    while ( prompt[i] != '\0' && j < PROMPTSZ )
    {
        if ( prompt[i] == '{' )
        {
            if ( prompt[i+1] == '{' )
                buf[j++] = prompt[i++];
            else
            {
                if ( readline == True )
                  buf[j++]  = '\001';
                buf[j++] = '\033';
                buf[j++] = '[';
            }
            i++;
        }
        else if ( prompt[i] == '}' )
        {
            if ( prompt[i+1] == '}' )
                buf[j++] = prompt[i++];
            else
            {
                buf[j++] = 'm';
                if ( readline == True )
                  buf[j++] = '\002';
            }
            i++;
        }
        else
            buf[j++] = prompt[i++];
    }
    buf[j] = '\0';

    if ( prompt[i] != '\0' && j >= PROMPTSZ )
        strcpy (buf, "sqsh> ");

    return buf;
}

