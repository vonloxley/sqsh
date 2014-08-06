/*
 * sqsh_main.c - Primary entry point into sqsh
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
#include <sys/ioctl.h>
#include <termios.h>
#include <pwd.h>
#include "sqsh_config.h"
#include "sqsh_job.h"
#include "sqsh_error.h"
#include "sqsh_global.h"
#include "sqsh_getopt.h"
#include "sqsh_init.h"
#include "sqsh_fd.h"
#include "sqsh_readline.h"
#include "sqsh_expand.h"
#include "sqsh_alias.h"
#include "sqsh_sig.h"
#include "sqsh_stdin.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_main.c,v 1.24 2014/08/05 15:54:38 mwesdorp Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes --*/
static void print_usage      _ANSI_ARGS(( void ));

#define SQSH_HIDEPWD

#if defined(SQSH_HIDEPWD)
#define MAXPWD 256
static void hide_password _ANSI_ARGS(( int argc, char *argv[]));
#endif


#if defined(TIOCGWINSZ) && defined(SIGWINCH)
static void sigwinch_handler _ANSI_ARGS(( int, void* ));
#endif

/*
 * The following structure is only used by print_usage() to describe
 * the set of valid flags, a the arguments for that flag, and a
 * description of that it does.
 */
typedef struct sqsh_flag_st {
    char *flag;
    char *arg;
    char *description;
} sqsh_flag_t;

/*
 * Array of information about the flags accepted by sqsh. This
 * is maintained in this manner so that I can have print_usage()
 * to the format for me...I got tired of re-doing it every time
 * I added a new flag.
 * sqsh-2.1.6 - New parameters added and list neatly ordered.
 */
static sqsh_flag_t sg_flags[] = {
/* Flag    Argument           Description
   -----   ---------          ----------------------------------- */
    { "-a", "count",          "Max. # of errors before abort"      },
    { "-A", "packet_size",    "Adjust TDS packet size"             },
    { "-b", "",               "Suppress banner message on startup" },
    { "-B", "",               "Turn off file buffering on startup" },
    { "-c", "[cmdend]",       "Alias for the 'go' command"         },
    { "-C", "sql",            "Send sql statement to server"       },
    { "-d", "severity",       "Min. severity level to display"     },
    { "-D", "database",       "Change database context on startup" },
    { "-e", "",               "Echo batch prior to executing"      },
    { "-E", "editor",         "Replace default editor (vi)"        },
    { "-f", "severity",       "Min. severity level for failure"    },
    { "-G", "TDS version",    "TDS version to use"                 },
    { "-h", "",               "Disable headers and footers"        },
    { "-H", "hostname",       "Set the client hostname"            },
    { "-i", "filename",       "Read input from file"               },
    { "-I", "interfaces",     "Alternate interfaces file"          },
    { "-J", "charset",        "Client character set"               },
    { "-k", "keywords",       "Specify alternate keywords file"    },
    { "-K", "keytab",         "Network security keytab file (DCE)" },
    { "-l", "level|flags",    "Set debugging level"                },
    { "-L", "var=value",      "Set the value of a given variable"  },
    { "-m", "style",          "Set display mode"                   },
    { "-n", "{on|off}",       "Set chained transaction mode"       },
    { "-N", "appname",        "Set Application Name (sqsh)"        },
    { "-o", "filename",       "Direct all output to file"          },
    { "-p", "",               "Display performance stats"          },
    { "-P", "[password]",     "Sybase password (NULL)"             },
    { "-Q", "query_timeout",  "Query timeout period in seconds"    },
    { "-r", "[sqshrc]",       "Specify name of .sqshrc"            },
    { "-R", "principal",      "Network security server principal"  },
    { "-s", "colsep",         "Alternate column separator (\\t)"   },
    { "-S", "server",         "Name of Sybase server ($DSQUERY)"   },
    { "-t", "[filter]",       "Filter batches through program"     },
    { "-T", "login_timeout",  "Login timeout period in seconds"    },
    { "-U", "username",       "Name of Sybase user"                },
    { "-v", "",               "Display current version and exit"   },
    { "-V", "[bcdimoqru]",    "Request network security services"  },
    { "-w", "width",          "Adjust result display width"        },
    { "-X", "",               "Enable client password encryption"  },
    { "-y", "directory",      "Override value of $SYBASE"          },
    { "-z", "language",       "Alternate display language"         },
    { "-Z", "[secmech]",      "Network security mechanism"         },
};

int
main( argc, argv )
    int    argc;
    char  *argv[];
{
    int           ret;
    int           exit_status;

    /*-- Variables required by sqsh_getopt() --*/
    extern  int    sqsh_optind;
    extern  char  *sqsh_optarg;
    char          *username = NULL;
    char          *rcfile;
    char          *history;
    char          *banner;
    char          *thresh_exit;
    char          *batch_failcount;
    char          *exit_failcount;
    char          *exit_value;
    char          *term_title;
    char           str[512];
    int            ch;
    uid_t          uid;
    struct passwd *pwd;
    int            fd;
    int            stdout_tty;  /* True if stdout is a tty */
    int            stdin_tty;   /* True if stdin is a tty */
    int            show_banner = True;
    int            set_width   = False;
    int            read_file   = False;  /* True if -i supplied */
    char          *sql = NULL;
    char          *cptr;
    int            i;
    varbuf_t      *exp_buf;

    /*
     * sqsh-2.2.0 - Variables used by password handling option
     * moved here.
     */
    char           buf[MAXPWD];
    char          *p;
    int            fdin, fdout;


    /*
     * If termios.h defines TIOCGWINSZ, then we need to declare a
     * structure in which to retrieve the current window size.
     */
#if defined(TIOCGWINSZ)
    struct winsize  ws;
#endif

#if defined(SQSH_HIDEPWD)
    /*
     * If the password is passed in with the -P option then we do a little
     * dance to hide it (i.e. we open a pipe, write the password to the pipe
     * re-exec the program and read the pipe there.)
     */
    hide_password(argc, argv);
#endif

    /*
     * Start us our reading from regular old stdin.
     */
    sqsh_stdin_file( stdin );


    /*
     * The first thing we need to do is intialize all commands, variables,
     * etc prior to parsing the command line.  This is important because
     * the command line flags simply alter these variables.
     */
    if( sqsh_init() == False ) {
        fprintf( stderr, "%s\n", sqsh_get_errstr() );
        sqsh_exit(255);
    }

    /*
     * If the first argument on the command line is -r, then we want
     * to process it before we attempt to read the .sqshrc file (since
     * -r allows the user to specify another .sqshrc file.
     */
    if ( argc > 1 && strcmp( argv[1], "-r" ) == 0)
    {
        if (argc > 2 && *argv[2] != '-')
        {
            if (access( argv[2], R_OK ) == -1)
            {
                fprintf( stderr, "sqsh: Unable to open %s: %s\n", argv[2],
                         strerror(errno) );
                sqsh_exit(255);
            }
            rcfile = argv[2];
            env_set( g_env, "rcfile", argv[2] );
        }
        else
        {
            rcfile = NULL;
            /* sqsh-2.1.9 - Set rcfile environment variable also to NULL */
            env_set( g_env, "rcfile", rcfile );
        }
    }
    else
    {
        /*
         * We now want to read the users resource file prior to dealing
         * with any command line options, this way we can override any
         * value defined in the rc file with values on the command line.
         */
        env_get( g_env, "SQSHRC", &rcfile );
        if (rcfile != NULL && *rcfile != '\0')
        {
            env_set( g_env, "rcfile", rcfile );
        }
        else
        {
            env_get( g_env, "rcfile", &rcfile );
        }
    }

    /*
     * Here is a nifty trick; in order to read the rc file we run the
     * \loop command, redirecting $rcfile to its standard in, asking
     * for it to *not* connect to the database using the -n flag.
     */
    if (rcfile != NULL && *rcfile != '\0')
    {
        /*
         * Since we need to chop up the contents of rcfile destructively
         * we make a backup copy of it.
         */
        strcpy(str, rcfile);
        cptr = strtok( str, ":\n\t\r" );

        /*
         * Create a temporary buffer for expansion.
         */
        exp_buf = varbuf_create( 512 );
        while (cptr != NULL)
        {
            if (sqsh_expand( cptr, exp_buf, 0 ) == False)
            {
                fprintf( stderr, "sqsh: Error expanding $rcfile: %s\n",
                    sqsh_get_errstr() );
                sqsh_exit(255);
            }

            cptr = varbuf_getstr(exp_buf);
            if (access( cptr, R_OK ) != -1)
            {
                env_set( g_env, "cur_rcfile", cptr );
                if (jobset_run( g_jobset, "\\loop -n $cur_rcfile", &exit_status) == -1 || exit_status == CMD_FAIL)
                {
                    if ( sqsh_get_error() != SQSH_E_NONE )
                        fprintf( stderr, "\\loop: %s\n", sqsh_get_errstr() );
                    sqsh_exit(255);
                }

                if( exit_status == CMD_FAIL )
                {
                    fprintf( stderr, "sqsh: Error in %s\n", cptr );
                    sqsh_exit(255);
                }
            }

            cptr = strtok( NULL, ":\n\t\r" );
        }
        /* sqsh-2.1.9 - Remove temporary environment variable cur_rcfile */
        env_remove( g_env, "cur_rcfile", 0);
        varbuf_destroy(exp_buf);
    }

    /*
     * Parse the command line options.  Note, that setting most of
     * these variables has side effects.  For example, setting
     * $script automatically sets $interactive to 0 and redirects
     * stdin from the script file.
     * sqsh-2.1.6 - New parameters added to the list and cases neatly ordered
     */
    while ((ch = sqsh_getopt_combined( "SQSH", argc, argv,
        "a:A:bBc;C:d:D:eE:f:G:hH:i:I:J:k:K:l:L:m:n:N:o:pP;Q:r;R:s:S:t;T:U:vV;w:Xy:z:Z;\250:" )) != EOF)
    {
        ret = 0;
        switch (ch)
        {
            case 'a' :
                ret = env_set( g_env, "thresh_exit", sqsh_optarg );
                break;
            case 'A' :
                ret = env_set( g_env, "packet_size", sqsh_optarg );
                break;
            case 'b' :
                ret = env_set( g_env, "banner", "0" );
                break;
            case 'B' :
                setbuf( stdout, NULL );
                setbuf( stderr, NULL );
                setbuf( stdin, NULL );
                ret = True;
                break;
            case 'c' :
                /*-- Cheesy newline-go hack. Suck! --*/
                if (sqsh_optarg == NULL || *sqsh_optarg == '\0')
                {
                    ret = env_set( g_env, "newline_go", "1" );
                }
                else if (alias_add( g_alias, sqsh_optarg, "\\go" ) <= 0)
                {
                    fprintf( stderr, "sqsh: -c: %s\n", sqsh_get_errstr() );
                    sqsh_exit(255);
                }
                ret = True;
                break;
            case 'C' :
                ret = True;
                sql = sqsh_optarg;
                env_set( g_env, "sql", sqsh_optarg );
                break;
            case 'd' :
                ret = env_set( g_env, "thresh_display", sqsh_optarg );
                break;
            case 'D' :
                ret = env_set( g_env, "database", sqsh_optarg );
                break;
            case 'e' :
                ret = env_set( g_env, "echo", "1" );
                break;
            case 'E' :
                ret = env_set( g_env, "EDITOR", sqsh_optarg );
                break;
            case 'f' :
                ret = env_set( g_env, "thresh_fail", sqsh_optarg );
                break;
            case 'G' : /* sqsh-2.1.6 */
                ret = env_set( g_env, "tds_version", sqsh_optarg );
                break;
            case 'h' :
                ret = env_set( g_env, "headers", "0" );

                if (ret != -1)
                {
                    ret = env_set( g_env, "footers", "0" );
                }
                break;
            case 'H' :
                ret = env_set( g_env, "hostname", sqsh_optarg );
                break;
            case 'i' :
                read_file   = True;
                show_banner = False;
                ret = env_set( g_env, "script", sqsh_optarg );
                break;
            case 'I' :
                ret = env_set( g_env, "interfaces", sqsh_optarg );
                break;
            case 'J' :
                ret = env_set( g_env, "charset", sqsh_optarg );
                break;
            case 'k' :
                ret = env_set( g_env, "keyword_file", sqsh_optarg );
                break;
            case 'K' : /* sqsh-2.1.6 */
                ret = env_set( g_env, "keytab_file", sqsh_optarg );
                break;
            case 'l' :
                ret = env_set( g_env, "debug", sqsh_optarg );
                break;
            case 'L' :
                cptr = sqsh_optarg;
                i    = 0;
                while( i < sizeof(str) && *cptr != '\0' && *cptr != '=' )
                    str[i++] = *cptr++;
                str[i] = '\0';
                if( *cptr != '=' ) {
                    fprintf( stderr, "sqsh: -L: Missing '='\n" );
                    sqsh_exit(255);
                }
                ++cptr;
                ret = env_set( g_env, str, cptr );
                break;
            case 'm' :
                ret = env_set( g_env, "style", sqsh_optarg );
                break;
            case 'n' :
                ret = env_set( g_env, "chained", sqsh_optarg );
                break;
            case 'N' :
                ret = env_set( g_env, "appname", sqsh_optarg);
                break;
            case 'o' :
                fflush(stdout);
                fflush(stderr);
                if( (fd = sqsh_open( sqsh_optarg, O_WRONLY|O_CREAT|O_TRUNC, 0 )) == -1 ||
                     sqsh_dup2( fd, fileno(stdout) ) == -1 ||
                     sqsh_dup2( fd, fileno(stderr) ) == -1 ||
                     sqsh_close( fd ) == -1 ) {
                    fprintf( stderr, "sqsh: -o: %s: %s\n", sqsh_optarg,
                                sqsh_get_errstr() );
                    sqsh_exit(255);
                }
                ret = True;
                break;
            case 'p' :
                ret = env_set( g_env, "statistics", "1" );
                break;
            case 'P' :
                /*
                 * Special case: An explicit -P'' should be treated the
                 * space as a -P without the optional argument.
                 */
                if (sqsh_optarg == NULL || *sqsh_optarg == '\0')
                {
                    ret = env_set( g_env, "password", NULL );
                }
                else
                {
                    ret = env_set( g_env, "password", sqsh_optarg );

                    while( *sqsh_optarg != '\0' )
                        *sqsh_optarg++ = ' ';
                }

                break;
            case 'Q' : /* sqsh-2.1.6 */
                ret = env_set( g_env, "query_timeout", sqsh_optarg );
                break;
            case 'r' :
                /*
                 * The alternative sqshrc file should already have been
                 * processed above. Note that the -r option should be the
                 * first option on the command line, otherwise the option
                 * is ignored, also the option and the filename should be
                 * separated by at leat a blank space.
                */
                ret = True;
                break;
            case 'R' : /* sqsh-2.1.6 */
                ret = env_set( g_env, "principal", sqsh_optarg);
                break;
            case 's' :
                ret = env_set( g_env, "colsep", sqsh_optarg );
                break;
            case 'S' :
                ret = env_set( g_env, "DSQUERY", sqsh_optarg );
                break;
            case 't' :
                ret = env_set( g_env, "filter", "1" );
                if (ret == True && sqsh_optarg != NULL)
                {
                    ret = env_set( g_env, "filter_prog", sqsh_optarg );
                }
                break;
            case 'T' : /* sqsh-2.1.6 */
                ret = env_set( g_env, "login_timeout", sqsh_optarg );
                break;
            case 'U' :
                ret = env_set( g_env, "username", sqsh_optarg );
                username = sqsh_optarg;
                break;
            case 'v' :
                printf( "%s\n", g_version );
                sqsh_exit(0);
                break;
            case 'V' : /* sqsh-2.1.6 */
                if (sqsh_optarg == NULL || *sqsh_optarg == '\0')
                    ret = env_set( g_env, "secure_options", "u" );
                else
                    ret = env_set( g_env, "secure_options", sqsh_optarg );
                break;
            case 'w' :
                ret = env_set( g_env, "width", sqsh_optarg );
                set_width = True;
                break;
            case 'X' :
                ret = env_set( g_env, "encryption", "1" );
                break;
            case 'y' :
                sprintf( str, "SYBASE=%s", sqsh_optarg );
                putenv( str );
                ret = env_set( g_env, "SYBASE", sqsh_optarg );
                break;
            case 'z' :
                ret = env_set( g_env, "language", sqsh_optarg );
                break;
            case 'Z' : /* sqsh-2.1.6 */
                if (sqsh_optarg == NULL || *sqsh_optarg == '\0')
                    ret = env_set( g_env, "secmech", "default");
                else
                    ret = env_set( g_env, "secmech", sqsh_optarg );
                break;
            case '\250' :
#if defined(SQSH_HIDEPWD)
                {
                  /*
                   * sqsh-2.1.7 - Incorporated patch by David Wood
                   * to solve a problem with pipes already in use. (Patch-id 2607434)
                   * The actual pipe file descriptors will now be passed on with the \250 option.
                  */
                  memset(buf, 0, MAXPWD);
                  if (sqsh_optarg != NULL)
                    strcpy (buf, sqsh_optarg);
                  if ((p = strchr(buf, '/')) != NULL) {
                    *p    = '\0';
                    fdin  = atoi(buf);
                    fdout = atoi(p+1);

                    memset(buf, 0, MAXPWD);
                    if (read(fdin, buf, MAXPWD-1) <= 0) {
                      fprintf(stderr, "sqsh: Error: Can't read password from pipe (filedes=%d)\n", fdin);
                      ret = False;
                    } else {
                      if (buf[0] == '\n')
                        ret = env_set( g_env, "password", NULL );
                      else
                        ret = env_set( g_env, "password", buf );
                    }
                    close(fdin);
                    close(fdout);
                  } else {
                      fprintf(stderr, "sqsh: Error: Missing pipe file descriptors for password option 250\n");
                      ret = False;
                  }
                }
#endif
                break;

            default :
                fprintf( stderr, "sqsh: %s\n", sqsh_get_errstr() );
                print_usage();
                sqsh_exit(255);
                break;
        }

        /*
         * Check the results from whichever variable we attempted
         * to set.
         */
        if (ret == False)
        {
            fprintf( stderr, "sqsh: -%c: %s\n", ch, sqsh_get_errstr() );
            sqsh_exit( 255 );
        }
    }

    /*
     * The only time that we allow extra parameters on the command line
     * is if the -i flag is supplied.  These are then passed as positional
     * parameters to the underlying script.
     */
    if (read_file == False && argc != sqsh_optind)
    {
        print_usage();
        sqsh_exit( 255 );
    }

    /*
     * If we are reading from a file and there are parameters left then
     * we want to copy them to the internal environment list.
     */
    if (read_file)
    {
        if (sqsh_optind > 1)
        {
            env_get (g_env, "script", &(argv[sqsh_optind-1])) ;
        }
        g_func_args[g_func_nargs].argc = argc - sqsh_optind + 1;
        g_func_args[g_func_nargs].argv = &(argv[sqsh_optind-1]);
        ++g_func_nargs;
    }

    /*
     * Figure out if stdin or stdout is connected to a tty.
     *
     * sqsh-2.1.7 - Note, if a file is provided with -i, stdin will be directed
     * to this file in \cmd_loop. So at this point, technically speaking, stdin
     * is still connected to a tty. That's why we can't use sqsh_stdin_isatty()
     * here as it will always return True.
     */
    stdout_tty = isatty( fileno(stdout) );
    if (isatty (fileno(stdin)) && !read_file && !sql)
        stdin_tty = True;
    else
        stdin_tty = False;
    DBG(sqsh_debug(DEBUG_SCREEN,"sqsh_main: : stdin_tty: %d, stdout_tty: %d\n",
        stdin_tty, stdout_tty));
    if (stdin_tty && !stdout_tty)
    {
        fprintf( stderr, "sqsh: Error: Cannot run in interactive mode with redirected output\n" );
        sqsh_exit( 255 );
    }
    if (stdin_tty && stdout_tty)
        g_interactive = True;

#if defined(TIOCGWINSZ)
    /*
     * If the width hasn't been explicitly set by the user and the
     * stdout is connected to a tty, then we want to request the
     * current screen width from the tty.
     */
    if (set_width == False && stdout_tty)
    {
        /* Check to see if the width has been set via the sqshrc file.
         * To do this we get the current setting - if it is != 80 then
         * the sqshrc file had a \set width directive, which we don't want
         * to override here.
         */
        char *w;
        env_get( g_env, "width", &w);
        if(!w || atoi(w) == 80) {
            if (ioctl(fileno(stdout), TIOCGWINSZ, &ws ) != -1) {
                sprintf( str, "%d", ws.ws_col );
                env_set( g_env, "width", str );

                DBG(sqsh_debug(DEBUG_SCREEN,"sqsh_main: Screen width = %d\n",
                    ws.ws_col);)
            } else {
            DBG(sqsh_debug(DEBUG_SCREEN,"sqsh_main: ioctl(%d,TIOCGWINSZ): %s\n",
                           (int)fileno(stdout), strerror(errno));)
            }
        }

#if defined(SIGWINCH)
        if (stdout_tty)
        {
            sig_install( SIGWINCH, sigwinch_handler, (void*)NULL, 0 );
        }
#endif /* SIGWINCH */
    }
#endif /* TIOGWINSZ */

#if defined (SIGQUIT)
    /*
     * Disable the CTRL-\ (SIGQUIT) signal when in interactive mode.
     */
    if (g_interactive)
        sig_install( SIGQUIT, SIG_H_IGN, NULL, 0 );
#endif /* SIGQUIT */

    /*
     * Set a TERM window title when running in interactive mode
     * and variable term_title is set.
     */
    if (g_interactive)
    {
        env_get( g_env, "term_title", &term_title );
        if (term_title != NULL && *term_title != '\0')
        {
            exp_buf = varbuf_create( 64 );

            if (exp_buf == NULL)
            {
                fprintf( stderr, "sqsh: %s\n", sqsh_get_errstr() );
                sqsh_exit( 255 );
            }

            if (sqsh_expand( term_title, exp_buf, 0 ) == False)
            {
                fprintf( stderr, "sqsh: Error expanding $term_title: %s\n",
                    sqsh_get_errstr() );
                sqsh_exit( 255 );
            }
            fprintf (stdout, "%c]0;%s %s%c",
                     '\033', argv[0], varbuf_getstr( exp_buf ), '\007' );
            varbuf_destroy( exp_buf );
        }
    }

    /*
     * If both input and output are connected to a tty *and* $banner
     * is 1 and the -i flag has not been set, then we display the
     * banner.
     */
    env_get( g_env, "banner", &banner );
    if (show_banner && (banner == NULL || *banner == '1') &&
        g_interactive)
    {
        printf( "%s ", g_version );
        DBG(printf( "(DEBUG) " );)
        printf( "%s", g_copyright );
        printf( "\n" );
        printf( "This is free software with ABSOLUTELY NO WARRANTY\n" );
        printf( "For more information type '\\warranty'\n" );
    }

    /*
     * Now, if username hasn't been explicitly set at the command
     * line then figure out what it is by looking at the current
     * uid.
     */
    env_get( g_env, "username", &username );

    if (username == NULL)
    {
        uid = getuid();
        pwd = getpwuid( uid );

        if (pwd == NULL)
        {
            fprintf( stderr, "sqsh: Unable to get username for uid %d: %s\n",
                     (int)uid, strerror(errno) );
            sqsh_exit(255);
        }

        env_set( g_env, "username", pwd->pw_name );
    }

    /*
     * Before we go any further we need to do a little work if
     * we are connected to a tty.
     */
    if (stdin_tty)
    {
        /*
         * Next, we need to try to load the history file, if one is
         * available.  We ignore the results of history_load, just
         * in case the history file doesn't exist.
         */
        env_get( g_env, "history", &history );

        /*
         * If a history file has been defined, then we want to
         * expand its contents. This will allow folks to have
         * a different history file for each server.
         */
        if (history != NULL && *history != '\0')
        {
            exp_buf = varbuf_create( 512 );

            if (exp_buf == NULL)
            {
                fprintf( stderr, "sqsh: %s\n", sqsh_get_errstr() );
                sqsh_exit( 255 );
            }

            if (sqsh_expand( history, exp_buf, 0 ) == False)
            {
                fprintf( stderr, "sqsh: Error expanding $history: %s\n",
                    sqsh_get_errstr() );
                history_load( g_history, history );
            }
            else
            {
                history_load( g_history, varbuf_getstr( exp_buf ) );
            }
            varbuf_destroy( exp_buf );

            sprintf( str, "%d", history_get_nbr(g_history) );
            env_set( g_env, "histnum", str );
        }

        /*
         * Initialize the readline "sub-system".  This basically consists
         * of installing handlers for readline keyword completion and
         * sucking in the completion keyword list and the readline history file.
         */
        sqsh_readline_init();
    }

    /*
     * If a single SQL statement was supplied on the command line
     * then we simply want to execute the \loop -e.
     */
    if (sql != NULL)
    {
        if (jobset_run( g_jobset, "\\loop -e \"$sql\"", &exit_status ) == -1 || exit_status == CMD_FAIL)
        {
            if ( sqsh_get_error() != SQSH_E_NONE )
                fprintf( stderr, "\\loop: %s\n", sqsh_get_errstr() );
            sqsh_exit( 255 );
        }
    }
    else
    {
        /*
         * Now that everything is configured we can go ahead and enter
         * the read-eval-print loop.  Note, it is the responsibility
         * of the loop to establish the connection to the database.
         */
        if (jobset_run( g_jobset, "\\loop $script", &exit_status ) == -1 || exit_status == CMD_FAIL)
        {
            if ( sqsh_get_error() != SQSH_E_NONE )
                fprintf( stderr, "\\loop: %s\n", sqsh_get_errstr() );
            sqsh_exit( 255 );
        }
    }

    /*
     * CMD_FAIL indicates that something went wrong internally with
     * \loop and that it probably never even executed a command.  If
     * this is the case, then just exit with 255.
     */
    if( exit_status == CMD_FAIL )
        sqsh_exit( 255 );

    /*
     * However, if \loop exited with a CMD_ABORT, then we need to check
     * to see if the reason was because the maximum number of errors
     * was reached.
     */
    if (exit_status == CMD_ABORT || exit_status == CMD_INTERRUPTED)
    {
        /*
         * Retrieve the values of thresh_exit and batch_failcount.  If
         * thresh_exit is not zero, and the two variables are equal, then
         * we need to exit with the total number of batches that failed.
         */
        env_get( g_env, "thresh_exit", &thresh_exit );
        env_get( g_env, "batch_failcount", &batch_failcount );
        if (thresh_exit != NULL && batch_failcount != NULL )
        {
            if( strcmp(thresh_exit,"0") != 0 &&
                strcmp( thresh_exit, batch_failcount ) == 0 )
                sqsh_exit( atoi(batch_failcount) );
        }

        /*
         * Otherwise, we return the abort error code.
         */
        sqsh_exit( 254 );
    }

    /*
     * This point is reached if \loop returned CMD_EXIT. Normally
     * isql would just exit(0) here. However, we are not isql,
     * and if exit_value is explicitly set with \exit n, then
     * we use that return value, otherwise if exit_failcount is 1,
     * then we exit with the total number of batches that failed.
     */
    env_get( g_env, "exit_value", &exit_value );
    if ( exit_value != NULL && atoi(exit_value) != 0 )
        sqsh_exit( atoi(exit_value) );

    env_get( g_env, "exit_failcount", &exit_failcount );
    env_get( g_env, "batch_failcount", &batch_failcount );
    if( exit_failcount != NULL && *exit_failcount == '1' &&
        batch_failcount != NULL ) {
        sqsh_exit( atoi(batch_failcount) );
    }

    /*
     * If exit_failcount is 0, then just perform a normal exit.
     */
    sqsh_exit( 0 );
    /* NOTREACHED */
    return (0);
}

#if defined(TIOCGWINSZ) && defined(SIGWINCH)

/*
 * sigwinch_handler():
 *
 * This function is called whenever a SIGWINCH (window size change)
 * signal is recieved during the life of sqsh.  Unfortunately, this
 * function calls quite a few functions that are known not to be
 * signal safe, but I am willing to accept the risk.  Thanks
 * to David Whitemarsh <djw@accessio.demon.co.uk> for supplying
 * this code.
 */
static void sigwinch_handler( sig, user_data )
    int   sig;
    void *user_data;
{
    struct winsize ws;
    char   ctty_path[SQSH_MAXPATH+1];
    int    ctty_fd;
    char   num[10];

    /*
     * Unfortunately, we can recieve the SIGWINCH if our controlling
     * terminal changes size, even though our stdout may have been
     * redirected to a file.  If this is the case, we cannot query
     * stdout for the size, we must ask the controlling terminal.
     */
    if (isatty( fileno(stdout) ))
        ctty_fd = fileno(stdout);
    else {

        /*
         * Attempt to grab the path to our controlling tty.  If we can't
         * find it, then we have to give up.
         */
        if (ctermid( ctty_path ) == NULL) {
            DBG(sqsh_debug(DEBUG_SCREEN,
                "sigwinch: Unable to get controlling tty\n");)
            return;
        }

        /*
         * Now, open a file descriptor to the controlling terminal. We'll
         * use this to query the current size.
         */
        if ((ctty_fd = open( ctty_path, O_RDONLY )) == -1) {
            DBG(sqsh_debug(DEBUG_SCREEN, "sigwinch: Unable to open %s for read\n",
                ctty_path);)
            return;
        }

    }

    if (ioctl( ctty_fd, TIOCGWINSZ, &ws ) != -1) {
        sprintf( num, "%d", (int)ws.ws_col );
        env_set( g_env, "width", num );

        DBG(sqsh_debug(DEBUG_SCREEN, "sigwinch: Screen size changed to %d\n",
             ws.ws_col);)
    } else {
        DBG(sqsh_debug(DEBUG_SCREEN, "sigwinch: ioctl(%d,TIOCGWINSZ): %s\n",
            fileno(stdout), strerror(errno));)
    }

    /*
     * Now, if we created a file descriptor to test the screen size
     * then we need to destroy it.
     */
    if (ctty_fd != fileno(stdout))
        close( ctty_fd );
}

#endif

/*
 * print_usage():
 *
 * Displays the contents of the sg_flags array in a pretty manner.
 */
static void print_usage()
{
    char  str[40];
    int   nflags;
    int   line_len;
    int   len;
    int   middle;
    int   i, j;

    /*-- Calculate the size of our array of flags --*/
    nflags = sizeof(sg_flags) / sizeof(sqsh_flag_t);

    /*
     * The following nastyness is responsible for formatting the
     * initial usage line.  Don't bother to try to understand it...
     * it would be easier to re-write it.
     */
    fprintf( stderr, "Use: sqsh" );
    line_len = 0;
    for( i = 0; i < nflags; i++ ) {
        if( sg_flags[i].arg == NULL || *sg_flags[i].arg == '\0' )
            sprintf( str, " [%s]", sg_flags[i].flag );
        else
            sprintf( str, " [%s %s]", sg_flags[i].flag, sg_flags[i].arg );

        len = strlen( str );

        if( (line_len + len) > 68 ) {
            fprintf( stderr, "\n         " );
            line_len = 0;
        }
        fputs( str, stderr );
        line_len += len;
    }

    fputs( "\n\n", stderr );

    /*
     * The splits the descriptions for each flag in our array into
     * two nice columns.
     */
    middle = (nflags+1) / 2;
    j = middle;
    for( i = 0; i < middle; i++ ) {
        fprintf( stderr, " %2s  %-35.35s", sg_flags[i].flag,
                                          sg_flags[i].description );
        if( j < nflags ) {
            fprintf( stderr, " %2s  %s\n", sg_flags[j].flag,
                                           sg_flags[j].description );
            ++j;
        } else {
            fputc( '\n', stderr );
        }
    }
    fputc( '\n', stderr );
}

#if defined(SQSH_HIDEPWD)
/*
 * sqsh-2.1.7 - Incorporated patch by David Wood
 * to solve a problem with pipes already in use. (Patch-id 2607434)
 * The actual pipe file descriptors will now be passed on with the \250 option.
 * sqsh-2.2.0 - Function reworked.
*/
static void hide_password (argc, argv)
  int   argc;
  char *argv[];
{
  int    i, j;
  char   buf[32];
  int    filedes[2];
  char   nullpwd[2];
  pid_t  pid;
  char  *pwd = NULL;
  int    status;


  nullpwd[0] = '\n';
  nullpwd[1] = '\0';

  /*
   * Loop through the list of arguments only once and skip all intermediate -P
   * entries, but remember the last password specified.
  */
  for (i = 0, j = 0; argv[i] != NULL; ++i)
  {
    if (*(argv[i]) == '-' && *(argv[i]+1) == 'P')
    {
      /*
       * New password parameter encounterd.
      */
      pwd = NULL;
      if (*(argv[i]+2) != '\0')
      {
        /*
         * Password passed on as: "sqsh -SSYBASE -Usa -Pxxxxxx" , or as -P-
        */
        pwd = (argv[i]+2);
      }
      else if ((i+1 < argc) && (*(argv[i+1]) != '-' || (*(argv[i+1]) == '-' && *(argv[i+1]+1) == '\0')))
      {
        /*
         * Password passed on as: "sqsh -SSYBASE -Usa -P xxxxxx" , or as -P -
        */
        pwd = argv[++i];
      }
      if (pwd == NULL || (pwd != NULL && strlen(pwd) == 0))
      {
        /*
         * Empty (NULL) password passed on as:
         * "sqsh -SSYBASE -Usa -P " or "sqsh -SSYBASE -Usa -P '' "
        */
        pwd = nullpwd;
      }
    }
    else
      argv[j++] = argv[i];
  }

  /*
   * If pwd == NULL then no -P was specified as argument and we do not
   * have to hide anything from the argument list.
  */
  if (pwd == NULL)
  {
    return;
  }

  /*
   * Create the pipe.
  */
  if (pipe (filedes) == -1)
  {
    perror ("sqsh: Error: Can't pipe()");
    return;
  }
  sprintf (buf, "-%c%d/%d", '\250', filedes[0], filedes[1]);
  argv[j++] = buf;
  argv[j]   = NULL;

  if ((pid = fork()) != 0)
  {
    /*
     * sqsh-2.2.0 - This code is executed by the parent process.
     * Wait for the child process to finish execution before we continue.
     */
    (void) waitpid (pid, &status, 0);
    /*
     * Re-execute ourselves in the parent process, with the modified argv[] list.
     */
    if (WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0)
      (void) execvp (argv[0], argv);
      /* Not reached */
    else
    {
      fprintf (stderr, "sqsh: Error: Processing of function hide_password failed unexpectedly\n");
      sqsh_exit (255);
    }
  }
  else
  {
    /*
     * The child process writes the password to the pipe, closes the pipe
     * and exits.
    */
    if ((int) write (filedes[1], pwd, strlen(pwd)) != (int) strlen(pwd))
    {
      fprintf (stderr, "sqsh: Error: Failed to write password to pipe (filedes=%d)\n", filedes[1]);
      sqsh_exit (255);
    }
    (void) close (filedes[0]);
    (void) close (filedes[1]);
    sqsh_exit (0);
  }
}

#endif

