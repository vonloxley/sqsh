/*
 * sqsh_readline.c - Retrieve a line of input from user
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
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_global.h"
#include "sqsh_env.h"
#include "sqsh_stdin.h"
#include "sqsh_readline.h"
#include "sqsh_expand.h"   /* sqsh-2.1.6 */

#if defined(USE_READLINE)
#include <readline/readline.h>

/*
 * Readline history functions - for some reason not all
 * readline installs have history.h available, so we do
 * this.
 */
extern void stifle_history();
extern int read_history();
extern int write_history();
extern void add_history();

#endif /* USE_READLINE */

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_readline.c,v 1.2 2004/11/05 15:53:45 mpeppler Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes needed by readline --*/
#if defined(USE_READLINE)
static char**  sqsh_completion     _ANSI_ARGS(( char*, int, int )) ;
static char*   sqsh_generator      _ANSI_ARGS(( char*, int )) ;

#if defined(OLD_READLINE)
#define   rl_compentry_func_t   Function
#define   rl_completion_matches completion_matches
#endif

/*
 * If GNU Readline support is compiled in, this data structure is
 * used to contain the active list of keywords.
 */
typedef struct keyword_st {
    char              *k_word ;          /* The actual word */
    struct keyword_st *k_nxt ;           /* Keep 'em in a list */
} keyword_t ;

/*-- Linked list of keywords --*/
static keyword_t     *sg_keyword_start = NULL ;
static keyword_t     *sg_keyword_end   = NULL ;

#endif /* USE_READLINE */


/*
 * sqsh_readline_init():
 *
 * This internal function is used to initialize any variables that
 * may be required by the readline library.  It doesn't really do
 * much if readline support isn't compiled in.
 */
int sqsh_readline_init()
{
#if defined(USE_READLINE)
    int   stifle_value;
    char *readline_histsize;
    char *readline_history;
    /* sqsh-2.1.6 - New variables */
    varbuf_t *exp_buf;

    /*
     * In non-interactive mode we don't need to bother with
     * any of the readline crap.
     */
    if (!sqsh_stdin_isatty())
    {
        return True;
    }

    /*
     * Set up the limit on the size of the readline history
     * buffer according to the value of $readline_history.
     */
    env_get( g_env, "readline_histsize", &readline_histsize );
    if (readline_histsize != NULL)
    {
        stifle_value = atoi(readline_histsize);

        if (stifle_value > 0)
        {
            stifle_history( stifle_value );
        }
    }

    /*
     * Read in the readline history file if necessary.
     * sqsh-2.1.6 feature - Expand readline_history variable
     */
    env_get( g_env, "readline_history", &readline_history );
    if (readline_history != NULL && *readline_history != '\0')
    {
        exp_buf = varbuf_create (512);
        if (exp_buf == NULL)
        {
          fprintf (stderr, "sqsh: %s\n", sqsh_get_errstr());
          sqsh_exit (255);
        }
        if (sqsh_expand (readline_history, exp_buf, 0) != False)
          read_history( varbuf_getstr(exp_buf) );
        else
            fprintf( stderr, "sqsh: Error expanding $readline_history: %s\n",
                sqsh_get_errstr() );
        varbuf_destroy (exp_buf);
    }

    DBG(sqsh_debug( DEBUG_READLINE, "sqsh_readline_init: Initializing\n" );)

    rl_readline_name                 = "sqsh" ;
    rl_completion_entry_function     = (rl_compentry_func_t*)sqsh_completion ;
    rl_attempted_completion_function = (CPPFunction*)sqsh_completion ;
#endif /* USE_READLINE */

    return True ;
}

int sqsh_readline_exit()
{
#if defined(USE_READLINE)
    char *readline_history;
    /* sqsh-2.1.6 - New variables */
    varbuf_t *exp_buf;

    /*
     * In non-interactive mode we don't need to bother with
     * any of the readline crap.
     */
    if (!sqsh_stdin_isatty())
    {
        return True;
    }

    /*
     * sqsh-2.1.6 feature - Expand readline_history variable
    */
    env_get( g_env, "readline_history", &readline_history );
    if (readline_history != NULL && *readline_history != '\0')
    {
        exp_buf = varbuf_create (512);
        if (exp_buf == NULL)
        {
          fprintf (stderr, "sqsh: %s\n", sqsh_get_errstr());
          sqsh_exit (255);
        }
        if (sqsh_expand (readline_history, exp_buf, 0) != False)
          write_history( varbuf_getstr(exp_buf) );
        else
            fprintf( stderr, "sqsh: Error expanding $readline_history: %s\n",
                sqsh_get_errstr() );
        varbuf_destroy (exp_buf);
    }
#endif /* USE_READLINE */

    return True;
}

/*
 * sqsh_readline():
 *
 * Reads a line of input from the user, providing prompt (if it is
 * non-null.
 */
char* sqsh_readline( prompt )
    char *prompt;
{
    static char str[4096];
    char *line;
#if defined(USE_READLINE)
    char *cp;
    /* sqsh-2.1.6 - New variables */
    char *ignoreeof = NULL;


    /* sqsh-2.1.6 feature - Expand color prompt */
    prompt = expand_color_prompt (prompt, True);
    if (prompt != NULL)
    {
        /*
         * sqsh-2.1.6 feature - Obtain environment variable ignoreeof. This will
         * indicate if we have to ignore ^D yes or no.
        */
        env_get( g_env, "ignoreeof", &ignoreeof );
        if (ignoreeof == NULL || *ignoreeof == '0')
        {
            /*
             *               Standard behaviour:
             * Since we have no way of capturing any real error conditions
             * from readline, if it returns NULL we just have to assume
             * that we have hit EOF, and not some error condition.  From what
             * we can tell from the readline library, there is no way to
             * differentiate the two.
            */
            if ((line = readline( prompt )) == NULL)
            {
                sqsh_set_error( SQSH_E_NONE, NULL );
                return NULL;
            }
        }
        else
        {
            /*
            ** If ignoreeof is defined True, continue with readline
            ** as long as NULL is returned (accidentally C-d pressed).
            */
            while ((line = readline( prompt )) == NULL) {
                fprintf (stdout, "\nUse \"exit\" or \"quit\" to leave the sqsh shell.\n");
                fflush  (stdout);
            }
        }


        /*
         * Attempt to find out if there is anything in this line except
         * for white-space.  If there isn't then don't save it to the
         * history.
         */
        for (cp = line; *cp != '\0' && isspace((int)*cp); ++cp);

        if (*cp != '\0')
        {
            add_history( line );
        }

        /*
         * Since readline mallocs line every time and doesn't append a 
         * newline, we make a copy of the current line, adding the newline
         * and free the readline copy.
         */
        sqsh_set_error( SQSH_E_NONE, NULL );
        sprintf( str, "%s\n", line );
        free( line );

        return str;
    }

#endif

    /*
     * If the user supplied a prompt, then print it out. Otherwise
     * they get no stinking prompt.
     */
    if (prompt != NULL)
    {
        fputs( prompt, stdout );
        fflush( stdout );
    }

    /*
     * Keep trying to read a line until we hit feof(stdin), or while
     * we are getting interrupted by signal handlers.
     */
    line = sqsh_stdin_fgets(str, sizeof( str ));

    if (line == NULL)
    {
        return(NULL);
    }

    sqsh_set_error( SQSH_E_NONE, NULL );
    return line;
}

/*
 * sqsh_readline_read():
 *
 * Reads the contents of filename into the readline keyword completion
 * list.
 */
int sqsh_readline_read( filename )
    char *filename ;
{
#if !defined(USE_READLINE)
    sqsh_set_error(SQSH_E_EXIST, "sqsh compiled without readline support" ) ;
    return False ;
#else
    FILE      *infile ;
    char       keyword[512] ;
    int        idx ;
    int        ch ;

    DBG(sqsh_debug(DEBUG_READLINE,"sqsh_readline_read: Reading %s\n",filename);)

    if( (infile = fopen( filename, "r")) == NULL ) {

        DBG(sqsh_debug( DEBUG_READLINE, "sqsh_readline_read: %s: %s\n", filename,
                       strerror( errno ) ) ;)

        sqsh_set_error( errno, "%s: %s", filename, strerror( errno ) ) ;
        return False ;
    }

    while( (ch = sqsh_getc(infile)) != EOF ) {

        /*-- Skip whitespace --*/
        while( ch != EOF && isspace((int)ch) )
            ch = sqsh_getc(infile) ;
        
        idx = 0 ;
        while( ch != EOF && !(isspace((int)ch)) ) {
            keyword[idx++] = ch ;
            ch = sqsh_getc(infile) ;
        }

        if( idx != 0 ) {
            keyword[idx] = '\0' ;

            if( sqsh_readline_add( keyword ) == False ) {
                fclose( infile ) ;
                return False ;
            }
        }
    }
    fclose(infile) ;
    return True ;
#endif
}

/*
 * sqsh_readline_add()
 *
 * Adds a new keyword to the readline keyword completion list.
 */
int sqsh_readline_add( keyword )
    char *keyword ;
{
#if !defined(USE_READLINE)
    sqsh_set_error(SQSH_E_EXIST, "sqsh compiled without readline support" ) ;
    return False ;
#else
    keyword_t *k ;

    /*-- Allocate a new structure --*/
    if( (k = (keyword_t*)malloc(sizeof(keyword_t))) == NULL ) {
        sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
        return False ;
    }

    /*-- Create the keyword --*/
    if( (k->k_word = sqsh_strdup( keyword )) == NULL ) {
        free( k ) ;
        sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
        return False ;
    }
    k->k_nxt = NULL ;

    DBG(sqsh_debug( DEBUG_READLINE, "sqsh_readline_add: Adding '%s'\n", 
                    k->k_word ) ;)

    if( sg_keyword_end == NULL )
        sg_keyword_start = sg_keyword_end = k ;
    else {
        sg_keyword_end->k_nxt = k ;
        sg_keyword_end = k ;
    }

    return True ;
#endif
}

/*
 * sqsh_readline_clear():
 *
 * Clears the contents of the readline keyword completion buffer.
 */
int sqsh_readline_clear()
{
#if !defined(USE_READLINE)
    sqsh_set_error(SQSH_E_EXIST, "sqsh compiled without readline support" ) ;
    return False ;
#else
    keyword_t  *k, *n ;

    k = sg_keyword_start ;
    while( k != NULL ) {
        n = k->k_nxt ;
        free( k->k_word ) ;
        free( k ) ;
        k = n ;
    }

    sg_keyword_start = sg_keyword_end = NULL ;
    return True ;
#endif
}


#if defined(USE_READLINE)

/*
 * This giant list is the complete list of key words avialable in 
 * in Sybase System 10.  It is used by the readline completion
 * generator function to do keyword completion. Note, this list
 * must remain in sorted order (case insensitive).
 *
 * It should also be noted that I really don't need to keep all
 * of these words in the list.  For example, 'as' and 'at' are
 * relatively useless for completion.  Also, there are several
 * undocumented key words in here that aren't very useful to most
 * humans.
 */
static char *sqsh_statements[] = {
    "abs",
    "acos",
   "add",
   "all",
   "alter",
   "and",
   "any",
   "arith_overflow",
   "as",
   "asc",
    "ascii",
    "asin",
   "at",
    "atan",
    "atn2",
   "authorization",
   "avg",
   "begin",
   "between",
   "break",
   "browse",
   "bulk",
   "by",
   "cascade",
    "ceiling",
    "char",
   "char_convert",
    "char_length",
    "charindex",
   "check",
   "checkpoint",
   "close",
   "clustered",
    "col_length",
    "col_name",
   "commit",
   "compute",
   "confirm",
   "constraint",
   "continue",
   "controlrow",
   "convert",
    "cos",
    "cot",
   "count",
   "create",
   "current",
   "cursor",
    "curunreservedpgs",
    "data_pgs",
   "database",
    "dateadd",
    "datediff",
    "datename",
    "datepart",
    "db_id",
    "db_name",
   "dbcc",
   "deallocate",
   "declare",
   "default",
   "define",
    "degrees",
   "delete",
   "desc",
    "difference",
   "disk",
   "distinct",
   "double",
   "drop",
   "dummy",
   "dump",
   "else",
   "end",
   "endtran",
   "errlvl",
   "errordata",
   "errorexit",
   "escape",
   "except",
   "execute",
   "exists",
   "exit",
    "exp",
   "fetch",
   "fillfactor",
    "floor",
   "for",
   "foreign",
   "from",
    "getdate",
   "goto",
   "grant",
   "group",
   "having",
    "hextoint",
   "holdlock",
    "host_name",
   "identity",
   "identity_insert",
   "if",
   "in",
   "index",
    "index_col",
   "insert",
   "intersect",
   "into",
    "inttohex",
   "is",
    "isnull",
   "isolation",
   "key",
   "kill",
    "lct_admin",
   "level",
   "like",
   "lineno",
   "load",
   "log",
    "log10",
    "lower",
    "ltrim",
   "max",
   "min",
   "mirror",
   "mirrorexit",
   "national",
   "noholdlock",
   "nonclustered",
   "not",
   "null",
   "numeric_truncation",
    "object_id",
    "object_name",
   "of",
   "off",
   "offsets",
   "on",
   "once",
   "only",
   "open",
   "option",
   "or",
   "order",
   "over",
   "param",
    "patindex",
   "permanent",
    "pi",
   "plan",
    "power",
   "precision",
   "prepare",
   "primary",
   "print",
   "privileges",
    "proc_role",
   "procedure",
   "processexit",
   "public",
    "radians",
   "raiserror",
    "rand",
   "read",
   "readtext",
   "reconfigure",
   "references",
   "replace",
    "replicate",
    "reserved_pgs",
   "restree",
   "return",
    "reverse",
   "revoke",
    "right",
   "role",
   "rollback",
    "round",
    "rowcnt",
   "rowcount",
   "rows",
    "rtrim",
   "rule",
   "save",
   "schema",
   "select",
   "set",
   "setuser",
   "shared",
    "show_role",
   "shutdown",
    "sign",
    "sin",
    "soundex",
    "space",
    "sqrt",
   "statement",
   "statistics",
    "str",
   "stripe",
    "stuff",
    "substring",
   "sum",
    "suser_id",
    "suser_name",
   "syb_identity",
   "table",
    "tan",
   "temporary",
   "terminate",
   "textsize",
   "to",
   "transaction",
   "trigger",
   "truncate",
   "tsequal",
   "union",
   "unique",
   "update",
    "upper",
   "use",
    "used_pgs",
   "user",
    "user_id",
    "user_name",
   "user_option",
   "using",
    "valid_name",
   "values",
   "varying",
   "view",
   "waitfor",
   "where",
   "while",
   "with",
   "work",
   "writetext"
} ;

/*
 * sqsh_generator():
 *
 * This function is used by readline.  It is called repeatededly
 * with the currently chunk of word that the user is sitting on (text)
 * and is expected to return each possible match for that word.
 */
static char* sqsh_generator( text, state )
    char *text;
    int   state;
{
    static      int        idx = 0;
    static      keyword_t *cur = NULL;
    static      char      *keyword_completion;
    int         low, high, middle;
    int         len;
    int         nitems;
    int         r; 
    char        *str;
    char        *cptr;
    char        *word;

    len    = strlen(text);
    nitems = sizeof(sqsh_statements) / sizeof(char*);


    /*
     * If this is the initial state, then we need to check to see if
     * the user even wants statement completion.  If he doesn't, then
     * don't do anything.
     */
    if (state == 0) 
    {
        env_get( g_env, "keyword_completion", &keyword_completion );

        if (keyword_completion == NULL || *keyword_completion == '0')
            return NULL;

        /*
         * If the user has supplied their own keyword completion list
         * then we want to ignore the built-in one.
         */
        if (sg_keyword_start != NULL) 
        {
            DBG(sqsh_debug(DEBUG_READLINE, "sqsh_generator: Using user list\n" );)

            /*
             * How's this for efficiency? A nice O(n) search of our list
             * of keywords.  Ok, so it ain't so elegant.  Sue me.
             */
            cur = sg_keyword_start;
            if (*keyword_completion == '4') /* Exact */
            {
                for (; cur != NULL && strncmp( cur->k_word, text, len ) != 0;
                    cur = cur->k_nxt );
            }
            else
            {
                for (; cur != NULL && strncasecmp( cur->k_word, text, len ) != 0;
                    cur = cur->k_nxt );
            }
            
            /*
             * If we failed to find anything, then give up and return
             * NULL to the caller indicating that we are all done or
             * have not found a match.
             */
            if (cur == NULL)
                return NULL;
            
            /*
             * Otherwise, save a pointer to the word that matched
             * so that we can figure out what to do next.
             */
            word = cur->k_word;
        } 
        else
        {
            DBG(sqsh_debug(DEBUG_READLINE, 
                           "sqsh_generator: Using internal list\n" );)

            /*
             * In the case of our built-in list we know that it is in
             * sorted order, so we will perform a binary search of the
             * array to save a little time.
             */
            low    = 0;
            middle = 0;
            high   = nitems - 1;
            while (low <= high)
            {
                middle = low + (high - low) / 2;
                if ((r = strncasecmp( sqsh_statements[middle], text, len )) == 0)
                    break;

                if( r < 0 )
                    low = middle + 1;
                else
                    high = middle - 1;
            }

            /*
             * If we couldn't even find a partial match, then give up 
             * and return NULL.
             */
            if (low > high)
                return NULL;
            
            /*
             * Now, we have found an entry which matches (at least partially)
             * the value of text.  However, we haven't necessarily found the
             * first on in our sorted list, so we need to back up until we 
             * find it.
             */
            for (idx = middle;
                  idx > 0 && strncasecmp( sqsh_statements[idx-1], text, len ) == 0;
                  --idx);
            
            word = sqsh_statements[idx];
        }

    }
    else   /* state != 0 */
    {

        /*
         * If the user has a keyword list defined of their own, then
         * we want to traverse to the next item in the list and see
         * if it matches the partial text that was passed in.
         */
        if (sg_keyword_start != NULL)
        {
            /*
             * If we are already at the end of the list then don't
             * bother to return anything.
             */
            if (cur == NULL)
                return NULL;

            /*
             * Traverse on through the list until we find another 
             * match or reach the end of the list.
             */
            for (cur = cur->k_nxt;
                  cur != NULL && strncasecmp( cur->k_word, text, len ) != 0;
                 cur = cur->k_nxt );
            
            /*
             * If we hit the end, then we let the caller know that
             * we are all done.
             */
            if (cur == NULL)
                return NULL;

            word = cur->k_word;

        }
        else
        {
            /*
             * If the user doesn't have a keyword list, then we want
             * to move to the next item in our sqsh_statements array.
             */
            ++idx;
            if (idx >= nitems || strncasecmp(sqsh_statements[idx],text,len) != 0)
                return NULL;
            word = sqsh_statements[idx];
        }
    }

    /*
     * At this point one of the sections of code above has set the
     * 'word' pointer to the word that is returned to the caller.
     * All we have to do now is to convert the word in the manner
     * requested the 'keyword_completion' variable and return
     * it.
     */
    str = (char*)malloc( (strlen(word) + 1) * sizeof(char) );

    if (str == NULL)
    {
        return NULL;
    }

    switch (*keyword_completion)
    {
        case '1':        /* Lower case */
            
            for (cptr = str; *word != '\0'; ++word, ++cptr)
            {
                *cptr = tolower(*word);
            }
            *cptr = '\0';
            break;
        
        case '2':        /* Upper case */

            for (cptr = str; *word != '\0'; ++word, ++cptr)
            {
                *cptr = toupper(*word);
            }
            *cptr = '\0';
            break;
        
        case '3':       /* Smart */

            for (cptr = str; *word != '\0'; ++word, ++cptr)
            {
                if (isupper((int)*text))
                {
                    *cptr = toupper(*word);
                }
                else
                {
                    *cptr = tolower(*word);
                }
            }
            *cptr = '\0';
            break;
        
        case '4':       /* Exact */
        default:
            strcpy( str, word );
    }

    return str ;
}

static char** sqsh_completion( text, start, end )
    char *text ;
    int   start ;
    int   end ;
{
    return (char **)rl_completion_matches( text, (rl_compentry_func_t*)sqsh_generator ) ;
}

#endif  /* USE_READLINE */
