/*
 * cmd_connect.c - User command to connect to the database
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
#include <ctpublic.h>
#include "sqsh_config.h"
#include "sqsh_debug.h"
#include "sqsh_error.h"
#include "sqsh_global.h"
#include "sqsh_getopt.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "sqsh_job.h"
#include "sqsh_init.h"
#include "sqsh_sig.h"
#include "sqsh_stdin.h"
#include "cmd.h"
#include "sqsh_expand.h" /* sqsh-2.1.6 */

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_connect.c,v 1.42 2014/08/06 09:38:15 mwesdorp Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * sqsh-3.0: Define FreeTDS enumerated values to prevent compile errors
 * when using different versions of FreeTDS.
*/
#if !defined(CS_TDS_70)
#define CS_TDS_70 7365
#endif
#if !defined(CS_TDS_71)
#define CS_TDS_71 7366
#endif
#if !defined(CS_TDS_72)
#define CS_TDS_72 7367
#endif
#if !defined(CS_TDS_73)
#define CS_TDS_73 7368
#endif
#if !defined(CS_TDS_74)
#define CS_TDS_74 7369
#endif

/*
 * sqsh-2.1.6 - Structure for Network Security Options
*/
typedef struct _NetSecService {
    CS_INT  service;
    CS_CHAR optchar;
    const CS_CHAR *name;
} NET_SEC_SERVICE;

/*
 * sg_login:  The following variable is set to True when we are in the
 *            middle of the login process.  It prevents the error handler
 *            from exiting due to a dead or NULL CS_CONTEXT structure.
 */
static int sg_login = False;
static int sg_login_failed = False;

/*
 * sqsh-2.1.6 feature - Keep track of the number of timeouts
 * i.e. occurrences of CS_TIMEOUT for a connection.
*/
static int timeouts;

/*-- Local Prototypes --*/
static CS_RETCODE syb_server_cb
    _ANSI_ARGS(( CS_CONTEXT*, CS_CONNECTION*, CS_SERVERMSG* ))
#if defined(__CYGWIN__)
    __attribute__ ((stdcall))
#endif /* __CYGWIN__ */
    ;

static CS_RETCODE syb_client_cb
    _ANSI_ARGS(( CS_CONTEXT*, CS_CONNECTION*, CS_CLIENTMSG* ))
#if defined(__CYGWIN__)
    __attribute__ ((stdcall))
#endif /* __CYGWIN__ */
    ;

static CS_RETCODE syb_cs_cb
    _ANSI_ARGS(( CS_CONTEXT*, CS_CLIENTMSG* ))
#if defined(__CYGWIN__)
    __attribute__ ((stdcall))
#endif /* __CYGWIN__ */
    ;

#if defined(CS_SSLVALIDATE_CB)
static CS_RETCODE validate_srvname_cb
    _ANSI_ARGS(( CS_VOID*, CS_SSLCERT*, CS_INT, CS_INT ))
#if defined(__CYGWIN__)
    __attribute__ ((stdcall))
#endif /* __CYGWIN__ */
    ;
#endif

static int wrap_print _ANSI_ARGS(( FILE*, char* )) ;
static int check_opt_capability _ANSI_ARGS(( CS_CONNECTION * ));

/* sqsh-2.1.6 - New function SetNetAuth */
static CS_RETCODE SetNetAuth _ANSI_ARGS(( CS_CONNECTION *,
               CS_CHAR *, CS_CHAR *, CS_CHAR *, CS_CHAR *));

/* sqsh-2.1.7 - New function ShowNetAuthCredExp */
static CS_RETCODE ShowNetAuthCredExp _ANSI_ARGS((CS_CONNECTION *,
               CS_CHAR *));

/* sqsh-2.2.0 - Signal handler to respond to SIGINT during cmd_connect */
static JMP_BUF sg_jmp_buf;
static int     sg_interrupted;
static void    connect_run_sigint ( int, void *);

/* sqsh-3.0 - Function execute_cmd to run a simple SQL command */
static CS_RETCODE execute_cmd _ANSI_ARGS((CS_CONNECTION *, CS_CHAR *));

/*
 * cmd_connect:
 *
 * This mostly internal command is used to connect to the database
 * if a connection hasn't already been established.  If one has been
 * established CMD_LEAVEBUF is returned.
 *
 * Variables Used:
 * ---------------
 *    username   -   Sybase username  (overridden by -U)
 *    password   -   Sybase password  (overridden by -P)
 *    server     -   Sybase server    (overridden by -S)
 *    database   -   Sybase database  (overridden by -D)
 *    interfaces -   Location of sybase interfaces file (overridden by -I)
 *    chained    -   transaction mode chained or unchained (-n on|off)
 *    preserve_context - Do not preserve database context when reconnecting
 *                       to a server but use the default database of the
 *                       specified login (-c)
 *
 *    appname        -N
 *    keytab_file    -K
 *    principal      -R
 *    secure_options -V
 *    secmech        -Z
 *    query_timeout  -Q
 *    login_timeout  -T
 */
int cmd_connect( argc, argv )
    int    argc ;
    char  *argv[] ;
{
    char      *database ;
    char      *username ;
    char      *password ;
    char      *server ;
    char      *interfaces ;
    char      *packet_size ;
    char      *autouse ;
    char      *session ;
    char      *charset ;
    char      *language ;
    char      *encryption ;
    char      *hostname ;
    char      *password_retry;
    char      *tds_version;
    char      *chained;
    char      *appname;
    char      *cp;
    extern    char *sqsh_optarg ;
    extern    int   sqsh_optind ;
    char      use_database[128] ;
    char      *usedbcheck ;
    int       c ;
    int       have_error = False ;
    int       preserve_context   = True ;
    char      orig_password[SQSH_PASSLEN+1];
    int       password_changed = False;
    char      sqlbuf[64];
    char      passbuf[64];
    int       len;
    int       exit_status;
    int       i;
    int       return_code;
    CS_INT    version;
    /* sqsh-2.1.6 - New variables */
    char      *keytab_file;
    char      *principal;
    char      *secmech;
    char      *secure_options;
    char      *login_timeout;
    char      *query_timeout;
    char      *clientname;
    char      *clienthostname;
    char      *clientapplname;
    CS_INT    SybTimeOut;
    CS_BOOL   NetAuthRequired;
    varbuf_t  *exp_buf = NULL;

    /* sqsh-2.2.0 - New variables for TDS debugging with ct_debug() */
#if defined(DEBUG) && defined(CS_SET_DBG_FILE) && defined(CS_SET_PROTOCOL_FILE)
    char      *debug_tds_logdata;
    char      *debug_tds_capture;
#endif

#if defined(CTLIB_SIGPOLL_BUG) && defined(F_SETOWN)
    int       ctlib_fd;
#endif

    CS_LOCALE  *locale = NULL;
#if !defined(_WINDOZE_)
    CS_INT      netio_type;
#endif
    CS_INT      con_status;
    CS_INT      retcode;


    /*
     * If we are already connected to the database, then don't
     * bother to do anything.
     */
    if (g_connection != NULL)
    {
        return CMD_LEAVEBUF ;
    }

    /*
     * First, we want to establish an environment "transaction". This
     * will allow us to change whichever environment variables we
     * would like, then roll things back if we want to fail.
     */
    env_tran( g_env );

    /*
     * Parse the command line options.
     * sqsh-2.1.6 - New options added and case evaluation neatly ordered.
     * sqsh-2.2.0 - -J option added. -I requires an optarg.
     */
    while ((c = sqsh_getopt( argc, argv, "A:cD:G:I:J:K:n:N:P;Q:R:S:T:U:V;XZ;z:" )) != EOF)
    {
        switch( c )
        {
            case 'A' : /* sqsh-2.2.0 - Option added */
                if (env_put( g_env, "packet_size", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -A: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break;
            case 'c' :
                preserve_context = False ;
                break ;
            case 'D' :
                if (env_put( g_env, "database", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -D: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break;
            case 'G' : /* sqsh-2.2.0 - Option added */
                if (env_put( g_env, "tds_version", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -G: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break;
            case 'I' :
                if (env_put( g_env, "interfaces", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -I: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'J' : /* sqsh-2.2.0 - Option added */
                if (env_put( g_env, "charset", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -J: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'K' : /* sqsh-2.1.6 */
                if (env_put( g_env, "keytab_file", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -K: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'n' :
                if (env_put( g_env, "chained", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -n: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'N' : /* sqsh-2.1.5 */
                if (env_put( g_env, "appname", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -N: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break;
            case 'P' :
                if (g_password_set == True && g_password != NULL)
                    strcpy( orig_password, g_password );
                password_changed = True;

                if (env_put( g_env, "password", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -P: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break;
            case 'Q' : /* sqsh-2.1.6 */
                if (env_put( g_env, "query_timeout", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -Q: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'R' : /* sqsh-2.1.6 */
                if (env_put( g_env, "principal", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -R: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'S' :
                if (env_put( g_env, "DSQUERY", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -S: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'T' : /* sqsh-2.1.6 */
                if (env_put( g_env, "login_timeout", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -T: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'U' :
                if (env_put( g_env, "username", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -U: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'V' : /* sqsh-2.1.6 */
                if (sqsh_optarg == NULL || *sqsh_optarg == '\0')
                    return_code = env_put( g_env, "secure_options", "u", ENV_F_TRAN);
                else
                    return_code = env_put( g_env, "secure_options", sqsh_optarg, ENV_F_TRAN);

                if (return_code == False)
                {
                    fprintf( stderr, "\\connect: -V: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'X' : /* sqsh-2.1.9 */
                if (env_put( g_env, "encryption", "1", ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -X: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'z' : /* sqsh-2.2.0 - Option added */
                if (env_put( g_env, "language", sqsh_optarg, ENV_F_TRAN ) == False)
                {
                    fprintf( stderr, "\\connect: -z: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;
            case 'Z' : /* sqsh-2.1.6 */
                if (sqsh_optarg == NULL || *sqsh_optarg == '\0')
                    return_code = env_put( g_env, "secmech", "default", ENV_F_TRAN);
                else
                    return_code = env_put( g_env, "secmech", sqsh_optarg, ENV_F_TRAN);
                if (return_code == False)
                {
                    fprintf( stderr, "\\connect: -Z: %s\n", sqsh_get_errstr() );
                    have_error = True;
                }
                break ;

            default :
                fprintf( stderr, "\\connect: %s\n", sqsh_get_errstr() ) ;
                have_error = True ;
                break ;
        }
    }

    /*
     * If there are any options left on the end of the line, then we have an error.
     * sqsh-2.1.6 - New options added to the list.
     * sqsh-2.2.0 - -J charset added to the list.
     */
    if( have_error || sqsh_optind != argc )
    {
        fprintf( stderr,
            "Use: \\connect [-A packet size] [-c] [-I interfaces] [-U username] [-P pwd] [-S server]\n"
            "       [-D database ] [-G tds version] [-J charset] [-N appname] [-n {on|off}]\n"
            "       [-Q query_timeout] [-T login_timeout] [-K keytab_file] [-R principal]\n"
            "       [-V [bcdimoqru]] [-X] [-z language] [-Z [secmech|default|none]]\n"
        ) ;

        env_rollback( g_env );
        return CMD_FAIL;
    }

    /*
     * Retrieve the memory addresses of the various environment variables that
     * may be used to connect. These may be overridden with flags to connect.
     * So basically our environment variables point to memory addresses. The
     * contents can still be modified by the execution of the $session file.
     */
    env_get( g_env, "username", &username ) ;
    env_get( g_env, "DSQUERY",  &server ) ;
    env_get( g_env, "interfaces", &interfaces ) ;
    env_get( g_env, "packet_size", &packet_size ) ;
    env_get( g_env, "autouse", &autouse ) ;
    env_get( g_env, "database", &database ) ;
    env_get( g_env, "charset", &charset ) ;
    env_get( g_env, "language", &language ) ;
    env_get( g_env, "encryption", &encryption ) ;
    env_get( g_env, "hostname", &hostname ) ;
    env_get( g_env, "password_retry", &password_retry ) ;
    env_get( g_env, "tds_version", &tds_version ) ;
    env_get( g_env, "chained", &chained ) ;
    env_get( g_env, "appname", &appname);
    /* sqsh-2.1.6 - New variables */
    env_get( g_env, "keytab_file", &keytab_file);
    env_get( g_env, "login_timeout", &login_timeout);
    env_get( g_env, "principal", &principal);
    env_get( g_env, "query_timeout", &query_timeout);
    env_get( g_env, "secmech", &secmech);
    env_get( g_env, "secure_options", &secure_options);

    password = g_password;

    /*
     * Allocate an expansion buffer for general use.
    */
    if ((exp_buf = varbuf_create( 512 )) == NULL)
    {
        fprintf( stderr, "sqsh: %s\n", sqsh_get_errstr() );
        sqsh_exit( 255 );
    }

   /*
    * sqsh-2.2.0 - Install a signal handler to catch SIGINT during cmd_connect.
    * The current signals are saved first and restored at the end of cmd_connect.
    */
    sig_save();
    sig_install( SIGINT, connect_run_sigint, (void *) NULL, 0);
    sg_interrupted = False;
    if (SETJMP( sg_jmp_buf ) != 0)
        goto connect_fail;

    /*
     * If the $session variable is set and the path that it contains
     * is a valid path name, then we want to execute the contents of
     * this file immediately, and prior to connecting to the database.
     */
    env_get( g_env, "session", &session );
    if ( session != NULL && *session != '\0' )
    {
        if (sqsh_expand( session, exp_buf, 0 ) != False)
        {
            session = varbuf_getstr( exp_buf );
            if (session != NULL && access( session, R_OK ) != -1)
            {
                DBG(sqsh_debug(DEBUG_ENV, "cmd_connect: session file is %s.\n",
                               session);)
                cp = malloc (strlen(session) + 12) ;
                sprintf (cp, "\\loop -n %s", session);
                if (jobset_run( g_jobset, cp, &exit_status ) == -1 || exit_status == CMD_FAIL)
                {
                    if ( sqsh_get_error() != SQSH_E_NONE )
                        fprintf( stderr, "%s: %s\n", cp, sqsh_get_errstr() );
                    free (cp);
                    goto connect_fail;
                }
                free (cp);
            }
        }
        else
        {
            fprintf( stderr, "sqsh: Error expanding $session: %s\n",
                     sqsh_get_errstr() );
            goto connect_fail;
        }
    }

    /*
     * sqsh-2.1.6 feature - Network Authentication
     *
     * If network authentication is requested by passing on options
     * -V and/or -Z, then we do not need to set the password.
     *
     * Note: Options -K and/or -R do not determine to use network
     * authentication, they are just helper variables to netauth.
     * Only the parameters -V and/or -Z will request network authentication.
     * This code relies on the fact that sqsh will fill in default values for
     * the parameters -V and -Z if the user does not supply a parameter value.
     * (Parameter values for -V and -Z are optional)
     * If a value of "none" is passed on to -Z, then we ignore all Network auth
     * settings and will login using a password altogether.
    */
    if (secmech != NULL && strcasecmp(secmech, "none") == 0)
    {
        env_put (g_env, secmech, NULL, ENV_F_TRAN);
        env_put (g_env, secure_options, NULL, ENV_F_TRAN);
        secmech         = NULL;
        secure_options  = NULL;
    }
    if ((secmech != NULL && *secmech != '\0') ||
        (secure_options != NULL && *secure_options != '\0'))
    {
        NetAuthRequired = CS_TRUE;
        DBG(sqsh_debug(DEBUG_ERROR, "cmd_connect: Network user authentication required.\n");)
    }
    else
    {
        NetAuthRequired = CS_FALSE;
        DBG(sqsh_debug(DEBUG_ERROR, "cmd_connect: Regular user authentication using password.\n");)
        if (password != NULL && strcmp( password, "-" ) == 0)
        {
            if (sqsh_stdin_fgets( passbuf, sizeof(passbuf) ) != NULL)
            {
                cp = strchr( passbuf, (int)'\n' );
                if (cp != NULL)
                {
                    *cp = '\0';
                }
                env_put( g_env, "password", passbuf, ENV_F_TRAN );
                password = g_password;
            }
        }

        /*
         * If we don't have a password to use (i.e. the $password isn't set
         * or -P was not supplied), then ask the user for one.
         */
        else if (g_password_set == False)
        {
            len = sqsh_getinput( "Password: ", passbuf, sizeof(passbuf), 0);

            if (len < 0)
            {
                if (len == -1)
                {
                    fprintf( stderr, "\\connect: %s\n", sqsh_get_errstr() );
                }
                goto connect_fail;
            }

            if (passbuf[len] == '\n')
                passbuf[len] = '\0';

            env_put( g_env, "password", passbuf, ENV_F_TRAN );
        }
    }

    /*
     * If the $database variable is set *prior* to connecting to the
     * database, then that indicates that we are to automatically
     * connect to $database prior to returning.
     */
    if( preserve_context && database != NULL && *database != '\0' )
    {
        strncpy( use_database, database, sizeof(use_database)-1 ) ;
        use_database[sizeof(use_database)-1] = '\0';
        autouse = use_database ;
    }

    /*
     * In order to keep the error handler from exiting if something
     * goes wrong here, we need to mark ourselves as being in the
     * middle of logging in.
     */
    sg_login = True ;

    /*
     * The context for the application need only be created once, we'll
     * re-use it as needed.
     */
    if (g_context == NULL)
    {
        /*-- Allocate a new context structure --*/
        /*-- mpeppler 4/9/2004
          we loop through the CS_VERSION_xxx values to try
          to use the highest one we find */

        retcode = CS_FAIL;

#if defined(CS_CURRENT_VERSION)
        if(retcode != CS_SUCCEED) {
            g_cs_ver = CS_CURRENT_VERSION;
            retcode = cs_ctx_alloc(g_cs_ver, &g_context);
        }
#endif
#if defined(CS_VERSION_160)
        if(retcode != CS_SUCCEED) {
            g_cs_ver = CS_VERSION_160;
            retcode = cs_ctx_alloc(g_cs_ver, &g_context);
        }
#endif
#if defined(CS_VERSION_157)
        if(retcode != CS_SUCCEED) {
            g_cs_ver = CS_VERSION_157;
            retcode = cs_ctx_alloc(g_cs_ver, &g_context);
        }
#endif
#if defined(CS_VERSION_155)
        if(retcode != CS_SUCCEED) {
            g_cs_ver = CS_VERSION_155;
            retcode = cs_ctx_alloc(g_cs_ver, &g_context);
        }
#endif
#if defined(CS_VERSION_150)
        if(retcode != CS_SUCCEED) {
            g_cs_ver = CS_VERSION_150;
            retcode = cs_ctx_alloc(g_cs_ver, &g_context);
        }
#endif
#if defined(CS_VERSION_125)
        if(retcode != CS_SUCCEED) {
            g_cs_ver = CS_VERSION_125;
            retcode = cs_ctx_alloc(g_cs_ver, &g_context);
        }
#endif
#if defined(CS_VERSION_120)
        if(retcode != CS_SUCCEED) {
            g_cs_ver = CS_VERSION_120;
            retcode = cs_ctx_alloc(g_cs_ver, &g_context);
        }
#endif
#if defined(CS_VERSION_110)
        if(retcode != CS_SUCCEED) {
            g_cs_ver = CS_VERSION_110;
            retcode = cs_ctx_alloc(g_cs_ver, &g_context);
        }
#endif

        if(retcode != CS_SUCCEED) {
            g_cs_ver = CS_VERSION_100;
            retcode = cs_ctx_alloc(g_cs_ver, &g_context);
        }
        DBG(sqsh_debug(DEBUG_TDS, "cmd_connect: g_cs_ver (CS_VERSION) set to: %d\n", g_cs_ver);)

        if (retcode != CS_SUCCEED) /* nothing worked... */
            goto connect_fail;

        /*-- Install callbacks into context --*/
        if (cs_config( g_context,                 /* Context */
                       CS_SET,                    /* Action */
                       CS_MESSAGE_CB,             /* Property */
                       (CS_VOID*)syb_cs_cb,       /* Buffer */
                       CS_UNUSED,                 /* Buffer length */
                       (CS_INT*)NULL              /* Output length */
                     ) != CS_SUCCEED)
        {
            fprintf( stderr, "\\connect: Unable to install message callback\n" );
            goto connect_fail;
        }

        /*-- Initialize the context --*/
        if (ct_init( g_context, g_cs_ver ) != CS_SUCCEED)
            goto connect_fail;

        if (ct_callback( g_context,                 /* Context */
                         (CS_CONNECTION*)NULL,      /* Connection */
                         CS_SET,                    /* Action */
                         CS_CLIENTMSG_CB,           /* Type */
                         (CS_VOID*)syb_client_cb    /* Callback Pointer */
                       ) != CS_SUCCEED)
            goto connect_fail;

        if (ct_callback( g_context,                 /* Context */
                         (CS_CONNECTION*)NULL,      /* Connection */
                         CS_SET,                    /* Action */
                         CS_SERVERMSG_CB,           /* Type */
                         (CS_VOID*)syb_server_cb    /* Callback Pointer */
                       ) != CS_SUCCEED)
            goto connect_fail;

        /*
         * Set the I/O type to syncronous (things would really freak out
         * in an async environment).
         */
#if !defined(_WINDOZE_)
        netio_type = CS_SYNC_IO;
        if (ct_config( g_context,                  /* Context */
                       CS_SET,                     /* Action */
                       CS_NETIO,                   /* Property */
                       (CS_VOID*)&netio_type,      /* Buffer */
                       CS_UNUSED,                  /* Buffer Length */
                       NULL                        /* Output Length */
                     ) != CS_SUCCEED)
            goto connect_fail;
#endif

        /*
         * sqsh-2.1.6 feature - Set CS_TIMEOUT and CS_LOGIN_TIMEOUT
        */
        if (query_timeout != NULL && *query_timeout != '\0' && atoi (query_timeout) > 0)
        {
            SybTimeOut = atoi (query_timeout);
            if (ct_config( g_context,                  /* Context */
                           CS_SET,                     /* Action */
                           CS_TIMEOUT,                 /* Property */
                           (CS_VOID*)&SybTimeOut,      /* Buffer */
                           CS_UNUSED,                  /* Buffer Length */
                           NULL                        /* Output Length */
                         ) != CS_SUCCEED)
            {
                DBG(sqsh_debug(DEBUG_ERROR, "ct_config: Failed to set CS_TIMEOUT to %d seconds.\n", SybTimeOut);)
                goto connect_fail;
            }
            DBG(sqsh_debug(DEBUG_ERROR, "ct_config: CS_TIMEOUT set to %d seconds.\n", SybTimeOut);)
        }

        if (login_timeout != NULL && *login_timeout != '\0' && atoi (login_timeout) > 0)
        {
            SybTimeOut = atoi (login_timeout);
            if (ct_config( g_context,                  /* Context */
                           CS_SET,                     /* Action */
                           CS_LOGIN_TIMEOUT,           /* Property */
                           (CS_VOID*)&SybTimeOut,      /* Buffer */
                           CS_UNUSED,                  /* Buffer Length */
                           NULL                        /* Output Length */
                         ) != CS_SUCCEED)
            {
                DBG(sqsh_debug(DEBUG_ERROR, "ct_config: Failed to set CS_LOGIN_TIMEOUT to %d seconds.\n", SybTimeOut);)
                goto connect_fail;
            }
            DBG(sqsh_debug(DEBUG_ERROR, "ct_config: CS_LOGIN_TIMEOUT set to %d seconds.\n", SybTimeOut);)
        }

        /*
         * sqsh-2.1.6/2.1.7 - If an interfaces file is specified, make sure
         * environment variables get expanded correctly.
        */
        if ( interfaces != NULL && *interfaces != '\0' )
        {
            if (sqsh_expand( interfaces, exp_buf, 0 ) != False)
            {
                cp = varbuf_getstr( exp_buf );
                DBG(sqsh_debug(DEBUG_ENV, "cmd_connect: Interfaces file is %s.\n", cp);)
                /*
                 * If the user overrode the default interfaces file location,
                 * then configure the interfaces file as such.
                 */
                if (cp == NULL || ct_config( g_context,   /* Context */
                                             CS_SET,      /* Action */
                                             CS_IFILE,    /* Property */
                                            (CS_VOID*)cp, /* Buffer */
                                             CS_NULLTERM, /* Buffer Length */
                                             NULL         /* Output Length */
                                           ) != CS_SUCCEED)
                    goto connect_fail;
            }
            else
            {
                fprintf( stderr, "sqsh: Error expanding $interfaces: %s\n",
                         sqsh_get_errstr() );
                goto connect_fail;
            }
        }

    } /* if (g_context == NULL) */

    /*
     * Allocate a new connection and initialize the username and
     * password for the connection.
     */
    if (ct_con_alloc( g_context, &g_connection ) != CS_SUCCEED)
        goto connect_fail;

    /*-- Set username --*/
    if (username != NULL && *username != '\0')    /* sqsh-2.1.6 sanity check */
    {
        if (ct_con_props( g_connection,           /* Connection */
                          CS_SET,                 /* Action */
                          CS_USERNAME,            /* Property */
                          (CS_VOID*)username,     /* Buffer */
                          CS_NULLTERM,            /* Buffer Length */
                          (CS_INT*)NULL           /* Output Length */
                        ) != CS_SUCCEED)
            goto connect_fail;
    }

    /*
     * sqsh-2.1.6 - If a keytab_file is specified, make sure environment
     * variables get expanded correctly.
    */
    if ( keytab_file != NULL && *keytab_file != '\0' )
    {
        if (sqsh_expand( keytab_file, exp_buf, 0 ) != False)
        {
            cp = varbuf_getstr( exp_buf );
            DBG(sqsh_debug(DEBUG_ENV, "cmd_connect: Keytab_file is %s.\n", cp);)
        }
        else
        {
            fprintf( stderr, "sqsh: Error expanding $keytab_file: %s\n",
                     sqsh_get_errstr() );
            cp = NULL;
        }
    }
    else
        cp = NULL;

    /*-- sqsh-2.1.6 feature - Use network authentication or set password --*/
    if (NetAuthRequired == CS_TRUE)
    {
        if (SetNetAuth ( g_connection,    /* Connection */
                         principal,       /* Server principal */
                         cp,              /* Expanded DCE keytab file */
                         secmech,         /* Security mechanism */
                         secure_options   /* Security options */
                       ) != CS_SUCCEED)
            goto connect_fail;
    }
    else
    {
        if (ct_con_props( g_connection,           /* Connection */
                          CS_SET,                 /* Action */
                          CS_PASSWORD,            /* Property */
                          (CS_VOID*)g_password,   /* Buffer */
                          (g_password == NULL) ? CS_UNUSED : CS_NULLTERM,
                          (CS_INT*)NULL           /* Output Length */
                        ) != CS_SUCCEED)
            goto connect_fail;
    }

    /*-- Set application name --*/
    if (appname != NULL && *appname != '\0')      /* sqsh-2.1.5 */
    {
        if (ct_con_props( g_connection,           /* Connection */
                          CS_SET,                 /* Action */
                          CS_APPNAME,             /* Property */
                          (CS_VOID*)appname,      /* Buffer */
                          CS_NULLTERM,            /* Buffer Length */
                          (CS_INT*)NULL           /* Output Length */
                        ) != CS_SUCCEED)
            goto connect_fail;
    }

    if (tds_version != NULL && *tds_version != '\0') /* sqsh-2.1.6 fix on *tds_version */
    {
        if (strcmp(tds_version, "4.0") == 0)
            version = CS_TDS_40;
        else if (strcmp(tds_version, "4.2") == 0)
            version = CS_TDS_42;
        else if (strcmp(tds_version, "4.6") == 0)
            version = CS_TDS_46;
        else if (strcmp(tds_version, "4.9.5") == 0)
            version = CS_TDS_495;
        else if (strcmp(tds_version, "5.0") == 0)
            version = CS_TDS_50;
#if defined(SQSH_FREETDS)
        /* Then we use freetds which uses enum instead of defines */
        else if (strcmp(tds_version, "7.0") == 0)
            version = CS_TDS_70;
        else if (strcmp(tds_version, "7.1") == 0)
            version = CS_TDS_71;
        else if (strcmp(tds_version, "7.2") == 0)
            version = CS_TDS_72;
        else if (strcmp(tds_version, "7.3") == 0)
            version = CS_TDS_73;
        else if (strcmp(tds_version, "7.4") == 0)
            version = CS_TDS_74;
#endif
        else version = CS_TDS_50; /* default version */


        if (ct_con_props(g_connection, CS_SET, CS_TDS_VERSION,
            (CS_VOID*)&version, CS_UNUSED, (CS_INT*)NULL) != CS_SUCCEED)
                goto connect_fail;
    }

    /*-- Hostname --*/
    if (hostname != NULL && *hostname != '\0')
    {
        if (ct_con_props( g_connection,            /* Connection */
                          CS_SET,                  /* Action */
                          CS_HOSTNAME,             /* Property */
                          (CS_VOID*)hostname,      /* Buffer */
                          CS_NULLTERM,             /* Buffer Length */
                          (CS_INT*)NULL            /* Output Length */
                        ) != CS_SUCCEED)
            goto connect_fail;
    }

    /*-- Packet Size --*/
    if (packet_size != NULL && *packet_size != '\0') /* sqsh-2.1.6 sanity check */
    {
        i = atoi(packet_size);
        if (ct_con_props( g_connection,            /* Connection */
                          CS_SET,                  /* Action */
                          CS_PACKETSIZE,           /* Property */
                          (CS_VOID*)&i,            /* Buffer */
                          CS_UNUSED,               /* Buffer Length */
                          (CS_INT*)NULL            /* Output Length */
                        ) != CS_SUCCEED)
            goto connect_fail;
    }

    /*-- Encryption --*/
    if (encryption != NULL && *encryption == '1')
    {
        i = CS_TRUE;
        if (ct_con_props( g_connection,            /* Connection */
                          CS_SET,                  /* Action */
                          CS_SEC_ENCRYPTION,       /* Property */
                          (CS_VOID*)&i,            /* Buffer */
                          CS_UNUSED,               /* Buffer Length */
                          (CS_INT*)NULL            /* Output Length */
                        ) != CS_SUCCEED)
            goto connect_fail;

#if defined(CS_SEC_EXTENDED_ENCRYPTION)
        /*
         * sqsh-2.1.9: Enable extended password encryption to be able to
         * connect to ASE servers with 'net password encryption reqd'
         * configured to 2 (RSA).
        */
        if (ct_con_props( g_connection,               /* Connection */
                          CS_SET,                     /* Action */
                          CS_SEC_EXTENDED_ENCRYPTION, /* Property */
                          (CS_VOID*)&i,               /* Buffer */
                          CS_UNUSED,                  /* Buffer Length */
                          (CS_INT*)NULL               /* Output Length */
                        ) != CS_SUCCEED)
            goto connect_fail;
#endif

    }

    /*
     * The following section initializes all locale type information.
     * First, we need to create a locale structure and initialize it
     * with information.
     */
    if (cs_loc_alloc( g_context, &locale ) != CS_SUCCEED)
        goto connect_fail;

    /*-- Initialize --*/
    if (cs_locale( g_context,                    /* Context */
                   CS_SET,                       /* Action */
                   locale,                       /* Locale Structure */
                   CS_LC_ALL,                    /* Property */
                   (CS_CHAR*)NULL,               /* Buffer */
                   CS_UNUSED,                    /* Buffer Length */
                   (CS_INT*)NULL                 /* Output Length */
                 ) != CS_SUCCEED)
        goto connect_fail;

    /*-- Language --*/
    if( language != NULL && *language != '\0' )
    {
        if (cs_locale( g_context,                /* Context */
                       CS_SET,                   /* Action */
                       locale,                   /* Locale Structure */
                       CS_SYB_LANG,              /* Property */
                       (CS_CHAR*)language,       /* Buffer */
                       CS_NULLTERM,              /* Buffer Length */
                       (CS_INT*)NULL             /* Output Length */
                     ) != CS_SUCCEED)
            goto connect_fail;
    }

    /*-- Character Set --*/
    if (charset != NULL && *charset != '\0')     /* sqsh-2.1.6 sanity check */
    {
        if (cs_locale( g_context,                /* Context */
                       CS_SET,                   /* Action */
                       locale,                   /* Locale Structure */
                       CS_SYB_CHARSET,           /* Property */
                       (CS_CHAR*)charset,        /* Buffer */
                       CS_NULLTERM,              /* Buffer Length */
                       (CS_INT*)NULL             /* Output Length */
                     ) != CS_SUCCEED)
            goto connect_fail;
    }

    /*-- Locale Property --*/
    if (ct_con_props( g_connection,              /* Connection */
                      CS_SET,                    /* Action */
                      CS_LOC_PROP,               /* Property */
                      (CS_VOID*)locale,          /* Buffer */
                      CS_UNUSED,                 /* Buffer Length */
                      (CS_INT*)NULL              /* Output Length */
                    ) != CS_SUCCEED)
        goto connect_fail;

    /* Handle case where server is defined as host:port */
    /*
     * sqsh-2.5 - Work around a bug in FreeTDS and do not call ct_con_props
     * for server:port connections but let ct_connect handle it.
     * OpenClient supports an optional filter that can be specified as third
     * parameter like host:port:filter. Filters are defined in libtcl.cfg.
     */
#if defined(CS_SERVERADDR) && !defined(SQSH_FREETDS)
    if ( server != NULL && (cp = strchr(server, ':')) != NULL )
    {
        char  *cp2;

        *cp = ' ';
        if ( (cp2 = strchr(cp+1, ':')) != NULL) /* Optional filter specified? */
            *cp2 = ' ';

        if (ct_con_props( g_connection,
                          CS_SET,
                          CS_SERVERADDR,
                          (CS_VOID*)server,
                          CS_NULLTERM,
                          (CS_INT*)NULL
                        ) != CS_SUCCEED)
            goto connect_fail;

        if (cp2 != NULL)
            *cp2 = '\0'; /* Remove optional filter from the servername */
        *cp = ':';       /* Put a ':' back into the display servername */

#if defined(CS_SSLVALIDATE_CB)
        if (ct_callback( g_context,                    /* Context */
                         (CS_CONNECTION*)NULL,         /* Connection */
                         CS_SET,                       /* Action */
                         CS_SSLVALIDATE_CB,            /* Type */
                         (CS_VOID*)validate_srvname_cb /* Callback Pointer */
                       ) != CS_SUCCEED)
            goto connect_fail;
#endif
    }
#endif

#if defined(DEBUG) && defined(CS_SET_DBG_FILE) && defined(CS_SET_PROTOCOL_FILE)
    /*
     * sqsh-2.2.0 - Setup TDS debugging using a logdata file and or a capture file
     * for tracing TDS packets.
     */
    if ( sqsh_debug_show (DEBUG_TDS) )
    {
        /*
         * Note, this requires the CT-lib development libraries to be linked/loaded with sqsh.
        */
        env_get (g_env, "debug_tds_logdata",  &debug_tds_logdata);
        if (debug_tds_logdata != NULL && *debug_tds_logdata != '\0')
        {
            if (sqsh_expand( debug_tds_logdata, exp_buf, 0 ) != False)
            {
                cp = varbuf_getstr( exp_buf );
                if (ct_debug ( g_context,             /* Context */
                               NULL,                  /* Connection */
                               CS_SET_DBG_FILE,       /* Action */
                               CS_UNUSED,             /* Flag */
                               cp,                    /* Buffer value */
                               CS_NULLTERM            /* String '\0' terminated */
                             ) != CS_SUCCEED)
                    fprintf (stderr, "\\connect: ct_debug - Unable to set CS_SET_DBG_FILE to %s\n", cp);
                else
                {
                    fprintf (stdout, "\\connect: ct_debug - Successfully set CS_SET_DBG_FILE to %s\n", cp);

                    if (ct_debug ( g_context,         /* Context */
                                   g_connection,      /* Connection */
                                   CS_SET_FLAG,       /* Action */
                                   CS_DBG_ALL,        /* Flag */
                                   NULL,              /* Buffer value */
                                   CS_UNUSED          /* Buffer length */
                                 ) != CS_SUCCEED)
                        fprintf (stderr, "\\connect: ct_debug - Unable to set flag CS_DBG_ALL\n");
                    else
                        fprintf (stdout, "\\connect: ct_debug - Flag CS_DBG_ALL successfully set\n");
                }
            }
            else
            {
               fprintf( stderr, "sqsh: Error expanding $debug_tds_logdata: %s\n", sqsh_get_errstr() );
            }
        }

        /*
         * For protocol tracing regular CT-lib libraries will do.
         * The created trace file can be decoded using Ribo.
        */
        env_get (g_env, "debug_tds_capture", &debug_tds_capture);
        if (debug_tds_capture != NULL && *debug_tds_capture != '\0')
        {
            if (sqsh_expand( debug_tds_capture, exp_buf, 0 ) != False)
            {
                cp = varbuf_getstr( exp_buf );
                if (ct_debug ( NULL,                  /* Context */
                               g_connection,          /* Connection */
                               CS_SET_PROTOCOL_FILE,  /* Action */
                               CS_UNUSED,             /* Flag */
                               cp,                    /* Buffer value */
                               CS_NULLTERM            /* String '\0' terminated */
                             ) != CS_SUCCEED)
                    fprintf (stderr, "\\connect: ct_debug - Unable to set CS_SET_PROTOCOL_FILE to %s\n", cp);
                else
                {
                    fprintf (stdout, "\\connect: ct_debug - Successfully set CS_SET_PROTOCOL_FILE to %s\n", cp);
                    if (ct_debug ( NULL,              /* Context */
                                   g_connection,      /* Connection */
                                   CS_SET_FLAG,       /* Action */
                                   CS_DBG_PROTOCOL,   /* Flag */
                                   NULL,              /* Buffer value */
                                   CS_UNUSED          /* Buffer length */
                                 ) != CS_SUCCEED)
                        fprintf (stderr, "\\connect: ct_debug - Unable to set falg CS_DBG_PROTOCOL\n");
                    else
                        fprintf (stdout, "\\connect: ct_debug - Flag CS_DBG_PROTOCOL successfully set\n");
                }
            }
            else
            {
               fprintf( stderr, "sqsh: Error expanding $debug_tds_capture: %s\n", sqsh_get_errstr() );
            }
        }
    }
#elif defined(DEBUG)
    if ( sqsh_debug_show (DEBUG_TDS) )
    {
        fprintf( stderr, "\\connect: ct_debug - TDS debugging is not supported in this version of CT-lib\n" );
    }
#endif

    /*
     * We sit in a loop and attempt to connect while we are getting
     * "Login failed" messages.
     */
    do
    {
        /*
         * Reset the failed login indicater. This will be set by
         * the server message handler if the login failed.
         */
        sg_login_failed = False;

        /*
         * Attempt to establish the connection to the database.  If this
         * fails the error messages will be displayed by the message and/or
         * error handlers.
         */
        if (ct_connect( g_connection, server,
            (server == NULL) ? CS_UNUSED : CS_NULLTERM ) != CS_SUCCEED)
        {
            /*
             * sqsh-2.1.9: Check for g_interactive instead of sqsh_stdin_isatty()
             * because that may not be determined at this point. Do not prompt
             * for a password if Network authentication is used but just failed.
             */
            if (*password_retry != '1'  || g_interactive   == False ||
                sg_login_failed != True || NetAuthRequired == True)
            {
                goto connect_fail;
            }

            /*
             * We failed due to a bad password, so lets prompt again.
             */
            len = sqsh_getinput( "Password: ", passbuf, sizeof(passbuf), 0 );

            if (len < 0)
            {
                if (len == -1)
                {
                    fprintf( stderr, "\\connect: %s\n", sqsh_get_errstr() );
                }
                goto connect_fail;
            }

            if (passbuf[len] == '\n')
                passbuf[len] = '\0';

            env_put( g_env, "password", passbuf, ENV_F_TRAN );
            password = g_password;

            ct_con_props( g_connection, CS_SET, CS_PASSWORD, (CS_VOID*)g_password,
                (g_password == NULL) ? CS_UNUSED : CS_NULLTERM, (CS_INT*)NULL );
        }
    }
    while (sg_login_failed == True);
    sg_login = False;

    /* XXX */
    /* ct_cancel(g_connection, NULL, CS_CANCEL_ALL); */

#if defined(CTLIB_SIGPOLL_BUG) && defined(F_SETOWN)
    /*
     * On certain platforms (older versions of CT-Lib on AIX and even
     * recent versions on SGI) there exists a bug having to do with
     * fork() in conjunction with the way that CT-Lib sets things up
     * for async I/O.  The following hack works around this bug (hopefully)
     * but should only be used as a last resort as it relies on knowing
     * that CT-Lib uses a file descriptor as its communication mechanism.
     */
    if (ct_con_props( g_connection,            /* Connection */
                      CS_GET,                  /* Action */
                      CS_ENDPOINT,             /* Property */
                      (CS_VOID*)&ctlib_fd,     /* Buffer */
                      CS_UNUSED,               /* Buffer Length */
                      (CS_INT*)NULL            /* Output Length */
                    ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\connect: WARNING: Unable to fetch CT-Lib file\n" );
        fprintf( stderr, "\\connect: descriptor to work around SIGPOLL bug.\n" );
    }
    else
    {
        /*
         * Here is the cruxt of the situation.  Async I/O works by delivering
         * the signal SIGPOLL or SIGIO every time a socket is ready to do
         * some work (either send or receive data).  Normally, this signal
         * will only be delivered to the process that requested the
         * facility.  Unfortunately, on some platforms, CT-Lib is explicitly
         * requesting that the signal be sent to every process in the same
         * process group!  This means that when sqsh spawns a child process
         * during a pipe (e.g. "go | more") the child process will receive
         * the SIGPOLL signal as well as sqsh...and, since 99% of the
         * programs out there don't know how to deal with SIGPOLL, they
         * will simply exit.  Not a good situation.
         */
        if (fcntl( ctlib_fd, F_SETOWN, getpid() ) == -1)
        {
            fprintf( stderr,
                "\\connect: WARNING: Cannot work around CT-Lib SIGPOLL bug: %s\n",
                strerror(errno) );
        }
    }
#endif /* CTLIB_SIGPOLL_BUG */

    /*
     * sqsh-2.1.7 - Determine the real tds_version being used if the user
     * requested to set a specific tds_version.
     */
    if (tds_version != NULL && *tds_version != '\0')
    {
        if (ct_con_props( g_connection,           /* Connection */
                          CS_GET,                 /* Action */
                          CS_TDS_VERSION,         /* Property */
                          (CS_VOID*)&version,     /* Buffer */
                          CS_UNUSED,              /* Buffer Length */
                          (CS_INT*)NULL
                        ) == CS_SUCCEED)
        {
            switch (version) {
                case CS_TDS_40:
                    env_set( g_env, "tds_version", "4.0" );
                    break;
                case CS_TDS_42:
                    env_set( g_env, "tds_version", "4.2" );
                    break;
                case CS_TDS_46:
                    env_set( g_env, "tds_version", "4.6" );
                    break;
                case CS_TDS_495:
                    env_set( g_env, "tds_version", "4.9.5" );
                    break;
                case CS_TDS_50:
                    env_set( g_env, "tds_version", "5.0" );
                    break;
#if defined(SQSH_FREETDS)
                case CS_TDS_70:
                    env_set( g_env, "tds_version", "7.0" );
                    break;
                case CS_TDS_71:
                    env_set( g_env, "tds_version", "7.1" );
                    break;
                case CS_TDS_72:
                    env_set( g_env, "tds_version", "7.2" );
                    break;
                case CS_TDS_73:
                    env_set( g_env, "tds_version", "7.3" );
                    break;
                case CS_TDS_74:
                    env_set( g_env, "tds_version", "7.4" );
                    break;
#endif
                default:
                    env_set( g_env, "tds_version", "unknown" );
            }
        }
    }

    /*
     * If we use network authentication, then determine the credential timeout value
     * and issue a notification message.
     */
    if (NetAuthRequired == CS_TRUE)
    {
        ShowNetAuthCredExp( g_connection, argv[0] );
    }

    /*-- If autouse has been set, use it --*/
    if (autouse != NULL && *autouse != '\0')
    {
        sprintf( sqlbuf, "use %s", autouse );
        (void) execute_cmd (g_connection, sqlbuf);

        /*
         * sqsh-2.4 - Check in batch mode if the -D <database> is correctly set.
         *            Otherwise, abort to prevent script execution in wrong default
         *            database.
         */
        env_get ( g_env, "database",   &database ) ;    /* Need to refresh var pointer */
        env_get ( g_env, "usedbcheck", &usedbcheck ) ;
        if (g_interactive != True && *usedbcheck == '1' && strcmp (autouse, database) != 0)
        {
            fprintf (stderr, "sqsh: ERROR: Unable to use database '%s' in batch mode\n", autouse);
            sqsh_exit(254);
        }
    }

    /*
     * Now, since the connection was succesfull, we want to change
     * any environment variables to accurately reflect the current
     * connection.
     */
    env_commit( g_env );
    timeouts = 0; /* sqsh-2.1.6 */

    if (locale != NULL)
        cs_loc_drop( g_context, locale );

    /* Set chained mode, if necessary. */
    if ( chained != NULL && *chained != '\0') /* sqsh-2.1.6 sanity check */
    {
        if ( check_opt_capability( g_connection ) )
        {
            CS_BOOL value = (*chained == '1' ? CS_TRUE : CS_FALSE);
            if (ct_options( g_connection, CS_SET, CS_OPT_CHAINXACTS, &value, CS_UNUSED, NULL) != CS_SUCCEED)
            {
                fprintf (stderr, "\\connect: WARNING: Unable to set transaction mode %s\n", (*chained == '1' ? "on" : "off"));
            }
        }
    }

    /*
     * sqsh-3.0: Set clientapplname, clienthostname and clientname.
    */
    env_get( g_env, "clientapplname", &clientapplname );
    if ( clientapplname != NULL && *clientapplname != '\0' )
    {
        sprintf (sqlbuf, "set clientapplname '%s'", clientapplname);
        (void) execute_cmd (g_connection, sqlbuf);
    }
    env_get( g_env, "clienthostname", &clienthostname );
    if ( clienthostname != NULL && *clienthostname != '\0' )
    {
        sprintf (sqlbuf, "set clienthostname '%s'", clienthostname);
        (void) execute_cmd (g_connection, sqlbuf);
    }
    env_get( g_env, "clientname",     &clientname );
    if ( clientname != NULL && *clientname != '\0' )
    {
        sprintf (sqlbuf, "set clientname '%s'", clientname);
        (void) execute_cmd (g_connection, sqlbuf);
    }

    return_code = CMD_LEAVEBUF;
    goto connect_leave;

connect_fail:
    DBG(sqsh_debug(DEBUG_ERROR, "connect: Failure encountered, cleaning up.\n");)

    /*
     * Restore the environment back to its previous state.
     */
    env_rollback( g_env );

    /*
     * And reset the password.
     */
    if (password_changed)
    {
        env_set( g_env, "password", orig_password );
    }

    /*-- Clean up the connection if established --*/
    if (g_connection != NULL && sg_interrupted == False)
    {
        /*-- Find out if the we are connected or not --*/
        if (ct_con_props( g_connection,           /* Connection */
                          CS_GET,                 /* Action */
                          CS_CON_STATUS,          /* Property */
                          (CS_VOID*)&con_status,  /* Buffer */
                          CS_UNUSED,              /* Buffer Length */
                          (CS_INT*)NULL
                        ) != CS_SUCCEED)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "connect:   Unable to get con status.\n");)
            con_status = CS_CONSTAT_CONNECTED;
        }

        /*-- If connected, disconnect --*/
        if (con_status == CS_CONSTAT_CONNECTED)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "connect:   Closing connection\n");)
            if (ct_close( g_connection, CS_UNUSED ) != CS_SUCCEED)
                ct_close( g_connection, CS_FORCE_CLOSE );
        }
        else
        {
            DBG(sqsh_debug(DEBUG_ERROR, "connect:   Connection alread closed\n");)
        }

        /*-- Free the memory --*/
        DBG(sqsh_debug(DEBUG_ERROR, "connect:   Dropping connection\n");)
        ct_con_drop( g_connection );
    }

    if (sg_interrupted == False)
    {
        if (locale != NULL)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "connect:   Dropping locale\n");)
            cs_loc_drop( g_context, locale );
        }

        if (g_context != NULL)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "connect:   Dropping context\n");)
            if (ct_exit( g_context, CS_UNUSED ) != CS_SUCCEED)
                ct_exit( g_context, CS_FORCE_EXIT );
            cs_ctx_drop( g_context );
        }
    }
    g_connection = NULL;
    g_context    = NULL;
    return_code = CMD_FAIL;

connect_leave:
    if ( exp_buf != NULL)
        varbuf_destroy( exp_buf );

    sig_restore();
    return return_code;
}

/*
 * Function: cmd_snace()
 *
 * Show network authenticated credential expiration.
*/
int cmd_snace( argc, argv )
    int    argc ;
    char  *argv[] ;
{
    /*
    * No arguments allowed.
    */
    if( argc != 1 ) {
        fprintf( stderr, "\\snace: Too many arguments; Use: \\snace\n" ) ;
        return CMD_FAIL ;
    }

    ShowNetAuthCredExp ( g_connection, argv[0] );

    return CMD_LEAVEBUF ;
}

static int check_opt_capability( g_connection )
    CS_CONNECTION *g_connection;
{
    CS_BOOL    val;
    CS_RETCODE ret;

    ret = ct_capability( g_connection,
                         CS_GET,
                         CS_CAP_REQUEST,
                         CS_OPTION_GET, (CS_VOID*)&val
                       );
    if (ret != CS_SUCCEED || val == CS_FALSE)
        return False;

    return True;
}

static int wrap_print( outfile, str )
    FILE *outfile ;
    char *str ;
{
    char *cptr ;
    char *start ;
    char *cur_end ;
    char *end ;
    int   width ;
    int   len ;

    env_get( g_env, "width", &cptr ) ;
    if( cptr == NULL || (width = atoi(cptr)) <= 10 )
        width = 80 ;

    len = strlen( str ) ;
    start   = str ;
    cur_end = str + min(len, width) ;
    end     = str + len ;

    while( len > 0 && cur_end <= end )
    {

        /*-- Move backwards until we hit whitespace --*/
        if( cur_end < end )
        {
            while( cur_end != start && !(isspace((int)*cur_end)) )
                --cur_end ;

            /*-- If there were none, then give up --*/
            if( cur_end == start )
                cur_end = start + (min(len, width)) ;
        }

        /*-- Print out the line --*/
        fprintf( outfile, "%*.*s", (int)(cur_end - start),
                 (int)(cur_end - start),
                 start ) ;

        if (*(cur_end-1) != '\n')
        {
            fputc( '\n', outfile );
        }

        while( cur_end != end && isspace((int)*cur_end) )
            ++cur_end ;

        /*-- Less to print now --*/
        len    -= (int)(cur_end - start) ;
        start   = cur_end ;
        cur_end = cur_end + (min(len, width)) ;

    }

    return True ;
}

/*
 * syb_server_cb():
 *
 * Sybase callback message handler for all messages coming from the
 * SQL Server or Open Server.
 *
 */
static CS_RETCODE syb_server_cb (ctx, con, msg)
    CS_CONTEXT     *ctx;
    CS_CONNECTION  *con;
    CS_SERVERMSG   *msg;
{
    char   *thresh_display;
    char   *thresh_fail;
    char    var_value[31];
    int     i;
    char   *c;
    /* sqsh-2.5 - New variables to support feature p2f */
    char   *p2faxm;
    int     p2faxm_int;
    char   *p2fname;
    FILE   *dest_fp;
    int     p2fstat = False;

    /*
     * Record last error in $?
     */
    if (msg->severity > 10)
    {
        sprintf( var_value, "%d", (int) msg->msgnumber );
        env_set( g_internal_env, "?", var_value );
    }

    /*
     * Check for "database changed", or "language changed" messages from
     * the client.  If we get one of these, then we need to pull the
     * name of the database or charset from the message and set the
     * appropriate variable.
     */
    if( msg->msgnumber == 5701 ||    /* database context change */
        msg->msgnumber == 5703 ||    /* language changed */
        msg->msgnumber == 5704 )     /* charset changed */
    {

        if (msg->text != NULL && (c = strchr( msg->text, '\'' )) != NULL)
        {
            i = 0;
            for( ++c; i <= 30 && *c != '\0' && *c != '\''; ++c )
                var_value[i++] = *c;
            var_value[i] = '\0';

            /*
             * On some systems, if the charset is mis-configured in the
             * SQL Server, it will come back as the string "<NULL>".  If
             * this is the case, then we want to ignore this value.
             */
            if (strcmp( var_value, "<NULL>" ) != 0)
            {
                switch (msg->msgnumber)
                {
                    case 5701 :
                        env_set( g_env, "database", var_value );
#if defined(USE_READLINE)
                        /* sqsh-2.1.8 - Feature dynamic keyword load */
                        env_set( g_internal_env, "keyword_refresh", "1" );
#endif
                        break;
                    case 5703 :
                        env_set( g_env, "language", var_value );
                        break;
                    case 5704 :
                        env_set( g_env, "charset", var_value );
                        break;
                    default :
                        break;
                }
            }
        }
        return CS_SUCCEED;
    }

    /*
     * If we got a login failed message, then record it as such and return.
     * sqsh-2.1.8 : Fix password_retry for Sybase RepServer and Microsoft MSSQL connections.
     * We also need to check for sg_login == True, i.e. we are in the middle of a login attempt.
     */
    if (sg_login == True && (msg->msgnumber == 1017  || /* DCO/ECDA Oracle   */
                             msg->msgnumber == 4002  || /* Sybase ASE/ASA/IQ */
                             msg->msgnumber == 14021 || /* Sybase RepServer  */
                             msg->msgnumber == 18456))  /* Microsoft MSSQL   */
    {
        wrap_print( stderr, msg->text );
        sg_login_failed = True;
        return CS_SUCCEED;
    }

    /*
     * Retrieve the threshold severity level for ignoring errors.
     * If the variable is set and severity is less than the
     * threshold, then return without printing out the message.
     */
    env_get( g_env, "thresh_display", &thresh_display );

    if (thresh_display == NULL || *thresh_display == '\0' ||
        msg->severity >= (CS_INT)atoi(thresh_display))
    {
        /*
         * If the severity is something other than 0 or the msg number is
         * 0 (user informational messages).
         */
        if (msg->severity >= 0 || msg->msgnumber == 10)
        {
            /*
             * sqsh-2.5 : Implementation of p2f feature.
             * When the number of messages handled by this callback handler exceeds the limit specified in p2faxm,
             * and a file is provided in $p2fname and is succesfully opened and we are in interactive mode, then
             * write the remaining messages from the current batch to this file instead of on screen.
             * Note that global variable g_p2fc will be reset to zero for each new batch in dsp.c.
             */
            env_get( g_env, "p2faxm",  &p2faxm );
            p2faxm_int = (p2faxm != NULL) ? atoi(p2faxm) : 0;
            if (++g_p2fc       > p2faxm_int &&
                p2faxm_int     > 0          &&
                g_p2f_fp      != NULL       &&
                g_interactive == True)
            {
                if (g_p2fc == p2faxm_int + 1) {
                    env_get( g_env, "p2fname", &p2fname );
                    fprintf (stderr,"Warning: Number of printed server messages exceeds p2faxm=%d limit for current batch.\n", p2faxm_int);
                    fprintf (stderr,"         Remaining server messages will be printed to file: %s\n", p2fname);
                    fflush  (stderr );
                    fprintf (g_p2f_fp, "--------\n");
                }
                p2fstat = True;
            }

            /*
             * If the message was something other than informational, and
             * the severity was greater than 0, then print information to
             * stderr with a little pre-amble information.  According to
             * the Sybase System Administrator's guide, severity level 10
             * messages should not display severity information.
             */
            if (msg->msgnumber > 0 && msg->severity > 10)
            {
                dest_fp = (p2fstat == True) ? g_p2f_fp : stderr;
                fprintf( dest_fp, "Msg %d, Level %d, State %d\n",
                            (int)msg->msgnumber, (int)msg->severity, (int)msg->state );
                if (msg->svrnlen > 0)
                    fprintf( dest_fp, "Server '%s'", (char*)msg->svrname );
                if (msg->proclen > 0)
                    fprintf( dest_fp, ", Procedure '%s'", (char*)msg->proc );
                if( msg->line > 0 )
                    fprintf( dest_fp, ", Line %d", (int)msg->line );
                fprintf( dest_fp, "\n" );
                wrap_print( dest_fp, msg->text );
                fflush( dest_fp );
            }
            else
            {
                /*
                 * Otherwise, it is just an informational (e.g. print) message
                 * from the server, so send it to stdout.
                 */
                dest_fp = (p2fstat == True) ? g_p2f_fp : stdout;
                wrap_print( dest_fp, msg->text );
                fflush( dest_fp );
            }
        }
    }

    /*
     * If the user specifies a threshold to be considered a failure
     * then increment our failcount.
     */
    env_get( g_env, "thresh_fail", &thresh_fail );

    if (thresh_fail != NULL && msg->severity >= (CS_INT)atoi(thresh_fail))
    {
        DBG(sqsh_debug(DEBUG_ERROR,
            "syb_server_cb: thresh_fail = %s, severity = %d, incrementing batch_failcount\n",
           thresh_fail, (int)msg->severity);)

        /*-- Increment number of failed batches --*/
        env_set( g_env, "batch_failcount", "1" );
    }

    return CS_SUCCEED;
}

static CS_RETCODE syb_cs_cb ( ctx, msg )
    CS_CONTEXT    *ctx;
    CS_CLIENTMSG  *msg;
{
    /*
     * For any other type of severity (that is not a server
     * message), we increment the batch_failcount.
     */
    env_set( g_env, "batch_failcount", "1" ) ;

    /*
     * If this is the "The attempt to connect to the server failed"
     * message and we previously got a "Login Failed" message from
     * the server (44), then ignore it.
     *
     * sqsh-2.1.9: The same for the freetds "Adaptive Server connection failed"
     * message number 34. Also check for g_interactive and display the Client
     * Message if the session is not interactive.
     */
    if ((CS_NUMBER(msg->msgnumber) == 34 || CS_NUMBER(msg->msgnumber) == 44)
        && sg_login_failed == True && g_interactive == True)
    {
        return CS_SUCCEED;
    }
    fprintf( stderr, "Open Client Message\n" );

    if (CS_NUMBER(msg->msgnumber) > 0)
    {
        fprintf( stderr, "Layer %d, Origin %d, Severity %d, Number %d\n",
                    (int)CS_LAYER(msg->msgnumber),
                    (int)CS_ORIGIN(msg->msgnumber),
                    (int)CS_SEVERITY(msg->msgnumber),
                    (int)CS_NUMBER(msg->msgnumber) );
    }
    if (msg->osstringlen > 0)
    {
        fprintf( stderr, "OS Error: %s\n", (char*)msg->osstring );
    }
    wrap_print( stderr, msg->msgstring );
    fflush( stderr ) ;

    return CS_SUCCEED ;
}

/*
 * Function syb_client_cb() error handler
 *
 * sqsh-2.1.6 feature - Client call back modified to facilitate handling
 *                      of query and login timeouts.
*/
#define ERROR_SNOL(e,s,n,o,l) \
  ( (CS_SEVERITY(e) == s) && (CS_NUMBER(e) == n) \
  &&  (CS_ORIGIN(e) == o) && (CS_LAYER(e)  == l) )

static CS_RETCODE syb_client_cb ( ctx, con, msg )
    CS_CONTEXT    *ctx;
    CS_CONNECTION *con;
    CS_CLIENTMSG  *msg;
{
    CS_INT status = 0;
    char  *server;
    char  *max_timeout;


    /*
     * sqsh-2.1.7 - Ignore a "security context has already expired" message
     * when this is caused by a ct_close.
     */
    if ( ERROR_SNOL(msg->msgnumber, 5, 2, 9, 7 ) )
        return CS_SUCCEED;

    /*
     * Let the CS-Lib handler print the message out.
     */
    (void) syb_cs_cb( ctx, msg );

    /*
     * If the dbprocess is dead or the dbproc is a NULL pointer and
     * we are not in the middle of logging in, then we need to exit.
     * We can't do anything from here on out anyway.
     */
    if (sg_login == False)
    {
        env_get( g_env, "DSQUERY", &server ) ;
#if defined(SQSH_FREETDS)
        if (CS_SEVERITY(msg->msgnumber) >= CS_SV_COMM_FAIL ||
            ctx == NULL || con == NULL)
#else
        /* Then we use freetds which uses enum instead of defines */
        if ((CS_SEVERITY(msg->msgnumber) >= CS_SV_COMM_FAIL &&
             CS_SEVERITY(msg->msgnumber) <= CS_SV_FATAL)    ||
            ctx == NULL || con == NULL)
#endif
        {
            fprintf (stderr, "%s: Aborting on severity %d\n", server, (int) CS_SEVERITY(msg->msgnumber) );
            sqsh_exit(254);
        }
    }

    /*
    ** Is it a timeout error?
    */
    if ( ERROR_SNOL(msg->msgnumber, CS_SV_RETRY_FAIL, 63, 2, 1 ) )
    {
        timeouts++;
        env_get( g_env, "DSQUERY", &server ) ;
        env_get( g_env, "max_timeout", &max_timeout ) ;
        if (max_timeout != NULL && *max_timeout != '\0' && atoi(max_timeout) > 0)
        {
            if (timeouts >= atoi(max_timeout))
            {
                fprintf (stderr, "%s: Query or command timeout detected, session aborted\n", server);
                fprintf (stderr, "%s: The client connection has detected this %d time(s).\n", server, timeouts);
                fprintf (stderr, "%s: Aborting on max_timeout limit %s\n", server, max_timeout );
                sqsh_exit(254);
            }
        }
        if (ct_con_props (con, CS_GET, CS_LOGIN_STATUS, (CS_VOID *)&status, CS_UNUSED, NULL) != CS_SUCCEED)
        {
            fprintf (stderr,"%s: ct_con_props() failed\n", server);
            return CS_FAIL;
        }

        if (status)
        {
            /* Result set timeout */
            (CS_VOID) ct_cancel(con, (CS_COMMAND *) NULL, CS_CANCEL_ATTN);
            fprintf (stderr, "%s: Query or command timeout detected, command/batch cancelled\n", server);
            fprintf (stderr, "%s: The client connection has detected this %d time(s).\n", server, timeouts);
            return CS_SUCCEED;
        }
        else
        {
            /* Login timeout */
            fprintf (stderr, "%s: Login timeout detected, aborting connection attempt\n", server);
            return CS_FAIL;
        }
    }
    return CS_SUCCEED ;
}


/*
 * sqsh-2.1.6 feature
 *
 * Function: SetNetAuth
 *
 * Purpose:  Enable network user authentication using the appropriate security
 *           mechanism (Kerberos, DCE or PAM) and set available options.
 *
 * Return :  CS_FAIL or CS_SUCCEED
*/
static CS_RETCODE SetNetAuth (conn, principal, keytab_file, secmech, req_options)
    CS_CONNECTION *conn;
    CS_CHAR       *principal;
    CS_CHAR       *keytab_file;
    CS_CHAR       *secmech;
    CS_CHAR       *req_options;
{

#if defined(CS_SEC_NETWORKAUTH)

    CS_CHAR buf[CS_MAX_CHAR+1];
    CS_INT  buflen;
    CS_INT  i;
    CS_BOOL OptSupported;

    static const NET_SEC_SERVICE nss[] = {
      /*
       * CS_SEC_NETWORKAUTH must be the first entry
      */
    { CS_SEC_NETWORKAUTH,    'u', "Network user authentication (unified login)" },
    { CS_SEC_CHANBIND,       'b', "Channel binding" },
    { CS_SEC_CONFIDENTIALITY,'c', "Data confidentiality" },
    { CS_SEC_DELEGATION,     'd', "Credentials delegation" },
    { CS_SEC_INTEGRITY,      'i', "Data integrity" },
    { CS_SEC_MUTUALAUTH,     'm', "Mutual client/server authentication" },
    { CS_SEC_DATAORIGIN,     'o', "Data origin stamping" },
    { CS_SEC_DETECTSEQ,      'q', "Data out-of-sequence detection" },
    { CS_SEC_DETECTREPLAY,   'r', "Data replay detection" },
    { CS_UNUSED,             ' ', "" }
    };


    DBG(sqsh_debug(DEBUG_ENV, "SetNetAuth - principal:   %s\n", (principal   == NULL ? "NULL" : principal));)
    DBG(sqsh_debug(DEBUG_ENV, "SetNetAuth - keytab_file: %s\n", (keytab_file == NULL ? "NULL" : keytab_file));)
    DBG(sqsh_debug(DEBUG_ENV, "SetNetAuth - secmech:     %s\n", (secmech     == NULL ? "NULL" : secmech));)
    DBG(sqsh_debug(DEBUG_ENV, "SetNetAuth - req_options: %s\n", (req_options == NULL ? "NULL" : req_options));)

    /*
     * Set optional server principal. (Server principal name without the realm name)
     */
    if (principal != NULL && *principal != '\0')
    {
        if (ct_con_props( g_connection,           /* Connection */
                          CS_SET,                 /* Action */
                          CS_SEC_SERVERPRINCIPAL, /* Property */
                          (CS_VOID*)principal,    /* Buffer */
                          CS_NULLTERM,            /* Buffer Length */
                          (CS_INT*)NULL           /* Output Length */
                        ) != CS_SUCCEED)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Failed to set principal %s.\n", principal);)
            return CS_FAIL;
        }
        DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Succesfully set principal %s.\n", principal);)
    }

    /*
     * Set optional keytab file for DCE network authentication.
     */
    if (keytab_file != NULL && *keytab_file != '\0')
    {
        if (ct_con_props( g_connection,           /* Connection */
                          CS_SET,                 /* Action */
                          CS_SEC_KEYTAB,          /* Property */
                          (CS_VOID*)keytab_file,  /* Buffer */
                          CS_NULLTERM,            /* Buffer Length */
                          (CS_INT*)NULL           /* Output Length */
                        ) != CS_SUCCEED)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Failed to set keytab %s.\n", keytab_file);)
            return CS_FAIL;
        }
        DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Succesfully set keytab %s.\n", keytab_file);)
    }

    /*
     * Set the security mechanism.
     * Note: If you do not supply a mechanism, or use "default", the first available
     * configured security mechanism from the libtcl.cfg file will be used.
    */
    if (secmech != NULL && *secmech != '\0' && strcasecmp(secmech, "default") != 0)
    {
        if (ct_con_props( g_connection,           /* Connection */
                          CS_SET,                 /* Action */
                          CS_SEC_MECHANISM,       /* Property */
                          (CS_VOID*)secmech,      /* Buffer */
                          CS_NULLTERM,            /* Buffer Length */
                          (CS_INT*)NULL           /* Output Length */
                        ) != CS_SUCCEED)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Failed to set secmech %s.\n", secmech);)
            return CS_FAIL;
        }
        DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Succesfully set secmech %s.\n", secmech);)
    }

    /*
     * Always set the CS_SEC_NETWORKAUTH option. (Option defined in nss[0]).
    */
    OptSupported = CS_TRUE;
    if (ct_con_props(conn, CS_SET, nss[0].service, &OptSupported,
        CS_UNUSED, NULL) != CS_SUCCEED)
    {
        DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Failed to enable option %s.\n", nss[0].name);)
        return CS_FAIL;
    }
    DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Succesfully enabled option %s.\n", nss[0].name);)

    /*
     * Request for the secmech actually in use.
    */
    if (ct_con_props( g_connection,           /* Connection */
                      CS_GET,                 /* Action */
                      CS_SEC_MECHANISM,       /* Property */
                      (CS_VOID*)buf,          /* Buffer */
                      CS_MAX_CHAR,            /* Buffer Length */
                      (CS_INT*)&buflen        /* Output Length */
                    ) != CS_SUCCEED)
    {
        DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Failed to get secmech name.\n");)
        return CS_FAIL;
    }
    DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Succesfully obtained secmech name %s.\n", buf);)
    env_put( g_env, "secmech", buf, ENV_F_TRAN );

    /*
     * Loop through the list of all other available security options
     * and check if the current option is requested (-V option),
     * otherwise continue with the next option.
     * Note: If the req_options pointer is NULL or is empty, all
     * possible options supported by the secmech are enabled.
     * (Should we report on non-existent options requested, or leave
     * it just the way it is?)
    */
    for (i = 1; nss[i].service != CS_UNUSED; i++)
    {
        if (req_options != NULL && strlen(req_options) > 0 &&
            strchr (req_options, nss[i].optchar) == NULL)
            continue;

        /*
        * First check if the secmech supports the requested service.
        */
        if (ct_con_props(conn, CS_SUPPORTED, nss[i].service, &OptSupported,
            CS_UNUSED, NULL) != CS_SUCCEED)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Failed to request support for option %s.\n",
                           nss[i].name);)
            return CS_FAIL;
        }

        /*
        * Is the service supported by the secmech, then activate it.
        */
        if (OptSupported == CS_TRUE)
        {
            if ( ct_con_props(conn, CS_SET, nss[i].service, &OptSupported,
                             CS_UNUSED, NULL) != CS_SUCCEED)
            {
                DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Failed to enable option %s.\n",
                               nss[i].name);)
                return CS_FAIL;
            }
            DBG(sqsh_debug(DEBUG_ERROR, "SetNetAuth: Succesfully enabled option %s.\n",
                           nss[i].name);)
        }
    }
    return CS_SUCCEED;
#else
    fprintf (stderr, "sqsh: ERROR: Network Authentication not supported\n");
    return CS_FAIL;
#endif
}


/*
 * Function: ShowNetAuthCredExp()
 *
 * Show the credential expiration timeout period of a network authenticated session.
 */
static CS_RETCODE ShowNetAuthCredExp (conn, cmdname)
    CS_CONNECTION *conn;
    CS_CHAR       *cmdname;
{
#if defined(CS_SEC_NETWORKAUTH)
    CS_INT     CredTimeOut;
    CS_BOOL    NETWORKAUTH;
    char      *datetime;
    char       dttm[32];
    time_t     exp_time;
    char       *fmt;
    char       *cp;

    /*
     * Check if current session is Network Authenticated.
    */
    if (ct_con_props( g_connection,       /* Connection */
                  CS_GET,                 /* Action */
                  CS_SEC_NETWORKAUTH,     /* Property */
                  (CS_VOID*)&NETWORKAUTH, /* Buffer */
                  CS_UNUSED,              /* Buffer Length */
                  (CS_INT*)NULL ) != CS_SUCCEED)
    {
        fprintf (stderr, "%s: ERROR: Unable to determine status of CS_SEC_NETWORKAUTH\n",
                 cmdname );
        return CS_FAIL;
    }

    if (NETWORKAUTH == CS_TRUE)
    {
        /*
         * If we use network authentication, then determine the credential timeout period.
        */
        if (ct_con_props( g_connection,       /* Connection */
                      CS_GET,                 /* Action */
                      CS_SEC_CREDTIMEOUT,     /* Property */
                      (CS_VOID*)&CredTimeOut, /* Buffer */
                      CS_UNUSED,              /* Buffer Length */
                      (CS_INT*)NULL ) != CS_SUCCEED)
        {
            fprintf (stderr, "%s: ERROR: Unable to determine value of CS_SEC_CREDTIMEOUT\n",
                     cmdname );
            return CS_FAIL;
        }

        switch (CredTimeOut)
        {
            case CS_NO_LIMIT:
                fprintf (stdout, "%s: Network Authenticated session does not expire\n", cmdname);
                break;

            case CS_UNEXPIRED:
                fprintf (stdout, "%s: Network Authenticated session is not expired\n", cmdname);
                break;

            case 0:
                fprintf (stdout, "%s: Network Authenticated session is already expired\n", cmdname);
                break;

            default:
                exp_time = time (NULL) + CredTimeOut;
                env_get( g_env, "datetime", &datetime ) ;
                if (datetime == NULL || datetime[0] == '\0' || strcmp(datetime, "default") == 0)
                {
                    cftime( dttm, "%Y%m%d %H:%M:%S", &exp_time );
                }
                else
                {
                    fmt = (char *) malloc (sizeof(char) * strlen(datetime)+2);
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
                    cftime( dttm, fmt, &exp_time );
                    free (fmt);
                }
                fprintf (stdout, "%s: Network Authenticated session expires at: %s (%d secs)\n",
                             cmdname, dttm, (int) CredTimeOut );
        }
    }
    else
        fprintf (stdout, "%s: No Network Authenticated session active\n", cmdname );
    return CS_SUCCEED;
#else
    fprintf (stdout, "%s: Network Authentication not supported\n", cmdname);
    return CS_SUCCEED;
#endif
}


/*
 * connect_run_sigint():
 *
 * This function is called whenever a SIGINT signal is received while processing cmd_connect.
 * Its only real job is to return to the point where the SETJMP function was executed.
 */
static void connect_run_sigint (sig, user_data )
    int   sig;
    void *user_data;
{
    sg_interrupted = True;
    LONGJMP (sg_jmp_buf, 1);
}


#if defined(CS_SSLVALIDATE_CB)
/*
 * sqsh-2.5 - Validate the servername in a host:port:ssl type of connection
 *            to be valid for the chosen certificate if the servername does
 *            not match the CN in the certificate.
 */
static CS_RETCODE validate_srvname_cb (userdata, certptr, certcount, valid)
    CS_VOID    *userdata;
    CS_SSLCERT *certptr;
    CS_INT      certcount;
    CS_INT      valid;
{
    if (valid == CS_SSL_INVALID_MISMATCHNAME)
    {
        return CS_SSL_VALID_CERT;
    }
    else
    {
        return valid;
    }
}
#endif


/*
 * sqsh-3.0 - Function to execute a simple SQL command without any result set processing.
*/
static CS_RETCODE execute_cmd (g_connection, sqlbuf)
    CS_CONNECTION *g_connection;
    CS_CHAR       *sqlbuf;
{
    CS_COMMAND    *cmd;
    CS_RETCODE     result_type;

    if (ct_cmd_alloc( g_connection, &cmd ) != CS_SUCCEED)
        return CS_FAIL;

    if (ct_command( cmd,                /* Command Structure */
                    CS_LANG_CMD,        /* Command Type */
                    sqlbuf,             /* Buffer */
                    CS_NULLTERM,        /* Buffer Length */
                    CS_UNUSED           /* Options */
                  ) != CS_SUCCEED)
    {
        ct_cmd_drop( cmd );
        return CS_FAIL;
    }

    if (ct_send( cmd ) != CS_SUCCEED)
    {
        ct_cmd_drop( cmd );
        return CS_FAIL;
    }

    while (ct_results( cmd, &result_type ) != CS_END_RESULTS);
    ct_cmd_drop( cmd );

    return CS_SUCCEED;
}
