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
#include "sqsh_env.h"
#include "sqsh_error.h"
#include "sqsh_expand.h"   /* sqsh-2.1.6 */
#include "sqsh_global.h"
#include "sqsh_init.h"
#include "sqsh_readline.h"
#include "sqsh_stdin.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_readline.c,v 1.6 2013/04/04 10:52:36 mwesdorp Exp $" ;
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
 * sqsh-2.1.8 - Function prototypes for new feature column name completion.
 */
static int sqsh_readline_addcol   _ANSI_ARGS(( char* )) ;
static int sqsh_readline_clearcol _ANSI_ARGS(( void  )) ;
static int DynColnameLoad         _ANSI_ARGS(( char* )) ;

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
static keyword_t     *sg_colname_start = NULL ;
static keyword_t     *sg_colname_end   = NULL ;

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

    DBG(sqsh_debug( DEBUG_READLINE, "sqsh_readline_init: Initializing\n" );)

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

    /*
     * sqsh-2.1.8: Moved loading keyword_file from sqsh_main.c to here.
     * If the user has requested some form of keyword completion
     * then attempt to read the contents of the keywords file.
    */
    (void) sqsh_readline_load ();

    rl_readline_name                 = "sqsh" ;
    rl_completion_entry_function     = (rl_compentry_func_t*)sqsh_completion ;
    rl_attempted_completion_function = (CPPFunction*)sqsh_completion ;

    /*
     * sqsh-2.1.8 - Remove '@' and '$' from the readline default list of word break
     * characters by assigning a new list of word break characters to the variable
     * rl_completer_word_break_characters. The @ and $ characters may be part
     * of object/column names and would otherwise lead to problems with TAB completion
     * when keyword_dynamic is enabled.
     */
    rl_completer_word_break_characters = " \t\n\"\\'`><=;|&{(";

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
    /* sqsh-2.1.6 - New variable */
    char *ignoreeof = NULL;
    /* sqsh-2.2.0 - New variables */
    char *readline_histignore;
    char *p1, *p2;
    int  match;


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
            /*
             * sqsh-2.2.0 - If $readline_histignore is set, then do not add the line to
             * the readline history if it matches an entry in the ':' seperated list.
             * Use case is to filter out the 'go', '\go', 'GO', quit, etc. statements
             * from the readline history.
            */
            match = False;
            env_get( g_env, "readline_histignore", &readline_histignore );
            if (readline_histignore != NULL && *readline_histignore != '\0')
            {
                cp = sqsh_strdup (readline_histignore);
                for (p1 = cp; p1 != NULL && match == False; p1 = p2)
                {
                    if ((p2 = strchr(p1, ':')) != NULL)
                        *p2++ = '\0';
                    if (strcmp (line, p1) == 0)
                        match = True;
                }
                if (cp != NULL)
                    free (cp);
            }
            if (match == False)
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
 * sqsh_readline_load ():
 *
 * sqsh-2.1.8: Check and expand keyword_file variable and call sqsh_readline_read
 * to suck in the keyword list.
 */
int sqsh_readline_load ( void )
{
#if !defined(USE_READLINE)
    sqsh_set_error(SQSH_E_EXIST, "sqsh compiled without readline support" ) ;
    return False ;
#else
    char          *keyword_file;
    varbuf_t      *exp_buf;
    int           ret = False ;

    env_get( g_env, "keyword_file", &keyword_file );
    if ( keyword_file != NULL && *keyword_file != '\0')
    {
        exp_buf = varbuf_create( 512 );
        if (exp_buf == NULL)
        {
            fprintf( stderr, "sqsh: %s\n", sqsh_get_errstr() );
            sqsh_exit( 255 );
        }
        if (sqsh_expand( keyword_file, exp_buf, 0 ) != False)
            ret = sqsh_readline_read( varbuf_getstr( exp_buf) );
        else
            fprintf( stderr, "sqsh: Error expanding $keyword_file: %s\n",
                   sqsh_get_errstr() );
        varbuf_destroy( exp_buf );
    }
    return ret;
#endif
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
 * sqsh-2.1.8 - Added all missing reserved words of ASE 15.7.
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
    "case",
    "ceiling",
    "char",
    "char_convert",
    "char_length",
    "charindex",
    "check",
    "checkpoint",
    "close",
    "clustered",
    "coalesce",
    "col_length",
    "col_name",
    "commit",
    "compressed",
    "compute",
    "confirm",
    "connect",
    "constraint",
    "continue",
    "controlrow",
    "convert",
    "cos",
    "cot",
    "count",
    "count_big",
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
    "decrypt",
    "decrypt_default",
    "default",
    "define",
    "degrees",
    "delete",
    "desc",
    "deterministic",
    "difference",
    "disk",
    "distinct",
    "double",
    "drop",
    "dual_control",
    "dummy",
    "dump",
    "else",
    "encrypt",
    "end",
    "endtran",
    "errlvl",
    "errordata",
    "errorexit",
    "escape",
    "except",
    "exclusive",
    "exec",
    "execute",
    "exists",
    "exit",
    "exp",
    "exp_row_size",
    "external",
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
    "identity_gap",
    "identity_insert",
    "identity_start",
    "if",
    "in",
    "index",
    "index_col",
    "inout",
    "insensitive",
    "insert",
    "install",
    "intersect",
    "into",
    "inttohex",
    "is",
    "isnull",
    "isolation",
    "jar",
    "join",
    "key",
    "kill",
    "lct_admin",
    "level",
    "like",
    "lineno",
    "load",
    "lob_compression",
    "lock",
    "log",
    "log10",
    "lower",
    "ltrim",
    "materialized",
    "max",
    "max_rows_per_page",
    "min",
    "mirror",
    "mirrorexit",
    "modify",
    "national",
    "noholdlock",
    "nonclustered",
    "not",
    "null",
    "nullif",
    "numeric_truncation",
    "object_id",
    "object_name",
    "of",
    "off",
    "offsets",
    "on",
    "once",
    "online",
    "only",
    "open",
    "option",
    "or",
    "order",
    "out",
    "output",
    "over",
    "param",
    "partition",
    "patindex",
    "perm",
    "permanent",
    "pi",
    "plan",
    "power",
    "precision",
    "prepare",
    "primary",
    "print",
    "privileges",
    "proc",
    "proc_role",
    "procedure",
    "processexit",
    "proxy_table",
    "public",
    "quiesce",
    "radians",
    "raiserror",
    "rand",
    "read",
    "readpast",
    "readtext",
    "reconfigure",
    "references",
    "release_locks_on_close",
    "remove",
    "reorg",
    "replace",
    "replicate",
    "replication",
    "reserved_pgs",
    "reservepagegap",
    "restree",
    "return",
    "returns",
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
    "scroll",
    "select",
    "semi_sensitive",
    "set",
    "setuser",
    "shared",
    "show_role",
    "shutdown",
    "sign",
    "sin",
    "some",
    "soundex",
    "space",
    "sqrt",
    "statement",
    "statistics",
    "str",
    "stringsize",
    "stripe",
    "stuff",
    "substring",
    "sum",
    "suser_id",
    "suser_name",
    "syb_identity",
    "syb_restree",
    "syb_terminate",
    "table",
    "tan",
    "temp",
    "temporary",
    "terminate",
    "textsize",
    "to",
    "tracefile",
    "tran",
    "transaction",
    "trigger",
    "truncate",
    "tsequal",
    "union",
    "unique",
    "unpartition",
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
    "when",
    "where",
    "while",
    "with",
    "work",
    "writetext",
    "xmlextract",
    "xmlparse",
    "xmltable",
    "xmltest"
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
    char        *keyword_dynamic;
    char        objname[256];

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
         * sqsh-2.1.8: In case keyword_dynamic is enabled, check if the user
         * wants to autocomplete a column/parameter name.
         */
        env_get( g_env, "keyword_dynamic", &keyword_dynamic );
        if (keyword_dynamic != NULL && *keyword_dynamic != '0')
        {
            /*
             * sqsh-2.1.8: If the text string contains a dot, then assume the
             * first part is an object name and the second part is the column
             * name we want to autocomplete. Obtain the objname from the text
             * and query all the column/parameter names for this object and
             * store them in a linked list.
             */
            for ( idx = 0; text[idx] != '\0' && text[idx] != '.'; idx++ );
            if ( text[idx] == '.' )
            {
                strncpy ( objname, text, idx );
                objname[idx] = '\0';
                (void) DynColnameLoad ( objname );
            }
            else if (sg_colname_start != NULL)
                sqsh_readline_clearcol ();
        }

        /*
         * If the user has supplied their own keyword completion list
         * then we want to ignore the built-in one.
         */
        if ((sg_keyword_start != NULL) || (sg_colname_start != NULL))
        {
            DBG(sqsh_debug(DEBUG_READLINE, "sqsh_generator: Using user list\n" );)

            /*
             * How's this for efficiency? A nice O(n) search of our list
             * of keywords.  Ok, so it ain't so elegant.  Sue me.
             */
            cur = sg_colname_start != NULL ? sg_colname_start : sg_keyword_start;
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
            {
                if (sg_colname_start != NULL)
                    sqsh_readline_clearcol();
                return NULL;
            }

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
        if ((sg_keyword_start != NULL) || (sg_colname_start != NULL))
        {
            /*
             * If we are already at the end of the list then don't
             * bother to return anything.
             */
            if (cur == NULL)
            {
                if (sg_colname_start != NULL)
                    sqsh_readline_clearcol();
                return NULL;
            }

            /*
             * Traverse on through the list until we find another
             * match or reach the end of the list.
             */
            if (*keyword_completion == '4') /* Exact */
            {
                for (cur = cur->k_nxt;
                     cur != NULL && strncmp( cur->k_word, text, len ) != 0;
                     cur = cur->k_nxt );
            }
            else
            {
                for (cur = cur->k_nxt;
                     cur != NULL && strncasecmp( cur->k_word, text, len ) != 0;
                     cur = cur->k_nxt );
            }

            /*
             * If we hit the end, then we let the caller know that
             * we are all done.
             */
            if (cur == NULL)
            {
                if (sg_colname_start != NULL)
                    sqsh_readline_clearcol();
                return NULL;
            }

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
        if (sg_colname_start != NULL)
            sqsh_readline_clearcol();
        return NULL;
    }

    switch (*keyword_completion)
    {
        case '1':        /* Lower case */

            for (cptr = str; *word != '\0'; ++word, ++cptr)
            {
                *cptr = tolower( (int) *word);
            }
            *cptr = '\0';
            break;

        case '2':        /* Upper case */

            for (cptr = str; *word != '\0'; ++word, ++cptr)
            {
                *cptr = toupper( (int) *word);
            }
            *cptr = '\0';
            break;

        case '3':       /* Smart */

            for (cptr = str; *word != '\0'; ++word, ++cptr)
            {
                if (isupper((int)*text))
                {
                    *cptr = toupper( (int) *word);
                }
                else
                {
                    *cptr = tolower( (int) *word);
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


/*
 * sqsh-2.1.8 - New feature: dynamic column name completion.
 *
 * The next local functions implement the dynamic column name completion feature.
 * To keep it simple, I just used my own copies of some of the above functions to
 * distinguish them from the existing functions that are being used by the keyword
 * completion function, instead of adapting these functions for generic use.
 */


/*
 * sqsh_readline_addcol()
 *
 * sqsh-2.1.8: Add a new column name to the readline keyword completion list.
 */
static int sqsh_readline_addcol( keyword )
    char *keyword ;
{
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

    if( sg_colname_end == NULL )
        sg_colname_start = sg_colname_end = k ;
    else {
        sg_colname_end->k_nxt = k ;
        sg_colname_end        = k ;
    }

    return True ;
}


/*
 * sqsh_readline_clearcol():
 *
 * sqsh-2.1.8: Clears the contents of the object specific column name linked list.
 */
static int sqsh_readline_clearcol()
{
    keyword_t  *k, *n ;

    k = sg_colname_start ;
    while( k != NULL ) {
        n = k->k_nxt ;
        free( k->k_word ) ;
        free( k ) ;
        k = n ;
    }

    sg_colname_start = sg_colname_end = NULL ;
    return True ;
}


/*
 * Function: DynColnameLoad()
 *
 * sqsh-2.1.8 - Dynamically execute a query to obtain the column names of
 * an existing table, view or stored procedure.
 *
 */
static int DynColnameLoad (objname)
    char *objname;
{
    CS_COMMAND *cmd;
    CS_DATAFMT  columns[1];
    CS_RETCODE  ret;
    CS_RETCODE  results_ret;
    CS_INT      result_type;
    CS_INT      count;
    CS_INT      idx;
    CS_INT      datalength[1];
    CS_SMALLINT indicator [1];
    CS_CHAR     name   [256];
    CS_CHAR     query  [768];


    if (sg_colname_start != NULL)
    {
        sqsh_readline_clearcol();
    }
    if ( g_connection == NULL )
    {
        DBG(sqsh_debug(DEBUG_ERROR, "DynColnameLoad: g_connection is not initialized.\n"));
        return (CS_FAIL);
    }
    if (ct_cmd_alloc( g_connection, &cmd ) != CS_SUCCEED)
    {
        DBG(sqsh_debug(DEBUG_ERROR, "DynColnameLoad: Call to ct_cmd_alloc failed.\n"));
        return (CS_FAIL);
    }
    sprintf (query, "select \'%s.\' + name from syscolumns where id=object_id(\'%s\') order by name"
                  ,objname, objname);
    if (ct_command( cmd,                /* Command Structure */
                    CS_LANG_CMD,        /* Command Type      */
                    query,              /* Query Buffer      */
                    CS_NULLTERM,        /* Buffer Length     */
                    CS_UNUSED           /* Options           */
                  ) != CS_SUCCEED)
    {
        ct_cmd_drop( cmd );
        DBG(sqsh_debug(DEBUG_ERROR, "DynColnameLoad: Call to ct_command failed.\n"));
        return (CS_FAIL);
    }
    if (ct_send( cmd ) != CS_SUCCEED)
    {
        ct_cmd_drop( cmd );
        DBG(sqsh_debug(DEBUG_ERROR, "DynColnameLoad: Call to ct_send failed.\n"));
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

                while (ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count) == CS_SUCCEED)
                {
                    /* Remove trailing blanks, tabs and newlines, just in case */
                    for ( idx = strlen(name) - 1;
                          idx >= 0 && (name[idx] == ' ' || name[idx] == '\t' || name[idx] == '\n');
                          name[idx--] = '\0');

                    sqsh_readline_addcol ( name );    /* Add name to linked list of colnames */
                }
                break;

            case CS_CMD_SUCCEED:
                DBG(sqsh_debug(DEBUG_ERROR, "DynColnameLoad: No rows returned from query.\n"));
                ret = CS_FAIL;
                break;

            case CS_CMD_FAIL:
                DBG(sqsh_debug(DEBUG_ERROR, "DynColnameLoad: Error encountered during query processing.\n"));
                ret = CS_FAIL;
                break;

            case CS_CMD_DONE:
                break;

            default:
                DBG(sqsh_debug(DEBUG_ERROR, "DynColnameLoad: Unexpected error encountered. (1)\n"));
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
            DBG(sqsh_debug(DEBUG_ERROR, "DynColnameLoad: Unexpected error encountered. (2)\n"));
            ret = CS_FAIL;
            break;

        default:
            DBG(sqsh_debug(DEBUG_ERROR, "DynColnameLoad: Unexpected error encountered. (3)\n"));
            ret = CS_FAIL;
            break;
    }
    ct_cmd_drop( cmd );
    if (ret == CS_FAIL)
        sqsh_readline_clearcol();

    return ( ret );
}

#endif  /* USE_READLINE */
