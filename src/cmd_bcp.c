/*
 * cmd_bcp.c - User command to bcp result set to another server.
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
#include <ctpublic.h>
#include <bkpublic.h>
#include "sqsh_config.h"
#include "sqsh_global.h"
#include "sqsh_expand.h"
#include "sqsh_error.h"
#include "sqsh_varbuf.h"
#include "sqsh_getopt.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "sqsh_sig.h"
#include "cmd.h"


/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_bcp.c,v 1.4 2004/11/22 07:10:23 mpeppler Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * The following macro is used to convert a start time and end
 * time into total elapsed number of seconds, to the decimal
 * place.
 */
#define ELAPSED_SEC(tv_start,tv_end) \
   (((double)(tv_end.tv_sec - tv_start.tv_sec)) + \
      ((double)(tv_end.tv_usec - tv_start.tv_usec)) / ((double)1000000.0))

/*
 * sg_interrupted:  This variable is set by the bcp_signal() signal
 *      handler and is to be polled periodically to determine if 
 *      a signal has been recieved.
 */
static int sg_interrupted  = False;

/*
 * sg_bcp_connection: This variable is used by the signal handler
 *      to determine which connection needs to be canceled.
 */
static CS_CONNECTION *sg_bcp_connection = NULL;

/*
 * The following are used during debugging to convert an internal
 * sybase id (such as a bind-type and a data type) to a readable
 * string.
 */
#if defined(DEBUG)

/*
 * Id vs. name structure.
 */
typedef struct idname_st {
    int    id;
    char  *name;
} idname_t;

#endif /* DEBUG */

/*
 * bcp_col_t: This data structure represents a single column of data
 *            as it returns from the server.  Note that all data is
 *            fetched and sent in its native data type.
 */
typedef struct _bcp_col_t {
    CS_INT      c_colid;       /* Column number */
    CS_DATAFMT  c_format;      /* Format for column */
    CS_INT      c_len;         /* Length of current row */
    CS_INT      c_prevlen;     /* Length of previous row */
    CS_SMALLINT c_nullind;     /* Null indicator for current row */
    CS_SMALLINT c_prevnullind; /* Null indicator for previous row */
    CS_VOID    *c_data;        /* Data space for fetch and bind */
} bcp_col_t;

/*
 * bcp_data_t: Represents an entire row of data.
 */
typedef struct _bcp_data_t {
    CS_INT      d_type;        /* Result type */
    CS_INT      d_ncols;       /* Number of columns to fetch */
    CS_BOOL     d_bound;       /* Have we bound */
    bcp_col_t  *d_cols;        /* Array of columns */
} bcp_data_t;

/*-- Prototypes --*/
static void        bcp_signal       _ANSI_ARGS(( int, void* ));
static bcp_data_t* bcp_data_bind    _ANSI_ARGS(( CS_COMMAND*, CS_INT ));
static CS_INT      bcp_data_xfer    _ANSI_ARGS(( bcp_data_t*, CS_COMMAND*, CS_BLKDESC* ));
static void        bcp_data_destroy _ANSI_ARGS(( bcp_data_t* ));
static CS_RETCODE  bcp_server_cb    
    _ANSI_ARGS(( CS_CONTEXT*, CS_CONNECTION*, CS_SERVERMSG* ))
#if defined(_CYGWIN32_)
   __attribute__ ((stdcall))
#endif /* _CYGWIN32_ */
   ;

static CS_RETCODE  bcp_client_cb
    _ANSI_ARGS(( CS_CONTEXT*, CS_CONNECTION*, CS_CLIENTMSG* ))
#if defined(_CYGWIN32_)
   __attribute__ ((stdcall))
#endif /* _CYGWIN32_ */
   ;

int cmd_bcp( argc, argv )
    int     argc ;
    char   *argv[] ;
{
    char             *password;      /* Current password */
    char             *username;      /* Value of $username variable */
    char             *server;        /* Value of $server variable */
    char             *expand;        /* Value of $expand variable */
    char             *packet_size;   /* Value of $packet_size variable */
    char             *charset;       /* Value of $charset */
    char             *language;      /* Value of $language */
    char             *database;      /* Value of $database */
    char             *encryption;    /* Value of $encryption */
    char             *hostname;      /* Value of $hostname */
    extern int        sqsh_optind;   /* Current option index in sqsh_getopt */
    extern char*      sqsh_optarg;   /* Value of option */
    int               opt;           /* Current option */
    char             *bcp_table;     /* Table to bcp into */
    char             *cmd_sql;       /* SQL command to send to server */
    int               rows_in_batch; /* Rows processed in batch */
    int               total_rows;    /* Total rows processing */
    struct timeval    tv_start;      /* Time we started processing */
    struct timeval    tv_end;        /* Time we finished processing */
    double            secs;          /* Seconds spend transferring data */
    int               nerrors;       /* Number of errors encountered */
    CS_INT            result_type;   /* Result type coming from server */
    CS_INT            return_code;   /* Return code from ct_results() */
    CS_INT            nrows;         /* Rows transferred */
    CS_BOOL           bcp_on = CS_TRUE; /* Flag to turn on bulk login */
    CS_INT            i;
    CS_INT            con_status;

    /*
     * The following variables need to be initialized to check if
     * they need cleaning up before we exit this procedure.
     */
    CS_COMMAND     *bcp_cmd    = NULL;  /* Command sent to server */
    CS_CONNECTION  *bcp_con    = NULL;  /* Connection to use for transfer */
    CS_BLKDESC     *bcp_desc   = NULL;  /* Block descriptor */
    CS_LOCALE      *bcp_locale = NULL;  /* Locale for bcp connection */
    varbuf_t       *exp_buf    = NULL;  /* Variable expansion buffer */
    bcp_data_t     *bcp_dat    = NULL;  /* Bind data */
    CS_INT         blk_ver     = -1;    /* BLK_VERSION_xxx value to use */


#if defined(CTLIB_SIGPOLL_BUG) && defined(F_SETOWN)
    int             ctlib_fd;
#endif

    /*
     * The following variables define default values that
     * may be overriden by command line arguments.
     */
    int               maxerrors     = 10;   /* Errors before abort */
    int               batchsize     = -1;   /* Copy all rows in one batch */
    int               have_error    = False;
    CS_BOOL           have_identity = CS_FALSE;

    /*
     * Retrieve value of variables used to configure the connection.
     * These values may be overriden by command line parameters.
     */
    password = g_password;
    env_get( g_env, "username",   &username );
    env_get( g_env, "DSQUERY",    &server );
    env_get( g_env, "expand",     &expand );
    env_get( g_env, "database",   &database );
    env_get( g_env, "charset",    &charset );
    env_get( g_env, "language",   &language );
    env_get( g_env, "encryption", &encryption );
    env_get( g_env, "hostname",   &hostname );
    env_get( g_env, "packet_size", &packet_size );

    while ((opt = sqsh_getopt( argc, argv, "A:b:I:J:m:NP;S:U:Xz:" )) != EOF) 
    {
        switch (opt) 
        {
            case 'A':
                packet_size = sqsh_optarg;
                break;

            case 'b':
                if ((batchsize = atoi(sqsh_optarg)) <= 0) 
                {
                    fprintf(stderr, "\\bcp: -b: Invalid value '%s'\n", sqsh_optarg);
                    return CMD_FAIL;
                }
                break;

            case 'I' :
                if (env_set( g_env, "interfaces", sqsh_optarg ) == False)
                {
                    fprintf( stderr, "\\bcp: -I: %s\n", sqsh_get_errstr() );
                    return CMD_FAIL;
                }
                break;

            case 'J' :
                charset = sqsh_optarg;
                break;

            case 'm':
                if ((maxerrors = atoi(sqsh_optarg)) <= 0) 
                {
                    fprintf(stderr, "\\bcp: -m: Invalid value '%s'\n", sqsh_optarg);
                    return CMD_FAIL;
                }
                break;
            
            case 'N':
                have_identity = CS_TRUE;
                break;

            case 'P' :
                password = sqsh_optarg;
                break;

            case 'S' :
                server = sqsh_optarg;
                break;

            case 'U' : 
                username = sqsh_optarg;
                break;

            case 'X' :
                encryption = "1";
                break;

            case 'z' :
                language = sqsh_optarg;
                break;

            default:
                fprintf(stderr,"\\bcp: %c: %s\n", opt, sqsh_get_errstr() );
                have_error = True;
        }
    }

    /*
     * If there isn't a table name left on the command line, or an
     * invalid argument was supplied, then print out usage 
     * information.
     */
    if ((argc - sqsh_optind) != 1 || have_error) 
    {
        fprintf(stderr, 
           "Use: \\bcp [-A packsetsize] [-b batchsize] [-I interfaces]\n"
           "          [-J charset] [-m maxerrors] [-N] [-P password]\n"
           "          [-S server] [-U username] [-X] table_name\n");
        return CMD_FAIL;
    }

    /*
     * Keep around a handy pointer.
     */
    bcp_table = argv[sqsh_optind];

    /*
     * Now, install our signal handlers.  At this point, all code should
     * perform a goto {return_fail or return_interrupt} to return an error
     * condition.  This is important in order to clean up the signal
     * handlers and various other data structures created.
     */
    sg_interrupted    = False;
    sg_bcp_connection = NULL;

    sig_install( SIGINT, bcp_signal, (void*)NULL, 0 );
    sig_install( SIGPIPE, bcp_signal, (void*)NULL, 0 );

    /*
     * Before we actually attempt to establish a connection to the
     * remote database (that we are bcp'ing too), lets launch our
     * query and see if it is valid.
     */
    if (expand == NULL || *expand != '0') 
    {
        /*
         * Temporarily create a buffer in which to store the expanded
         * SQL buffer.
         */
        if ((exp_buf = varbuf_create(512)) == NULL) 
        {
            fprintf(stderr, "\\bcp: varbuf_create: %s\n", sqsh_get_errstr());
            goto return_fail;
        }

        if (sqsh_expand( varbuf_getstr( g_sqlbuf ), exp_buf,
                         EXP_STRIPESC|EXP_COMMENT|EXP_COLUMNS ) == False) 
        {
            fprintf(stderr, "\\bcp: sqsh_expand: %s\n", sqsh_get_errstr());
            goto return_fail;
        }

        cmd_sql = varbuf_getstr( exp_buf );

    } 
    else
    {
        cmd_sql = varbuf_getstr( g_sqlbuf );
    }

    /*-- Initialize timing --*/
    gettimeofday( &tv_start, NULL );
    total_rows = 0;

    /*-- Allocate a new command structure --*/
    if (ct_cmd_alloc( g_connection, &bcp_cmd ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\bcp: Unable to allocate new command\n" );
        goto return_fail;
    }

    /*-- Find the appropriate BLK_VERSION_xxx value --*/
#if defined(CS_VERSION_150)
    if(blk_ver == -1 && g_cs_ver == CS_VERSION_150) {
	blk_ver = BLK_VERSION_150;
    }
#endif
#if defined(CS_VERSION_125)
    if(blk_ver == -1 && g_cs_ver == CS_VERSION_125) {
	blk_ver = BLK_VERSION_125;
    }
#endif
#if defined(CS_VERSION_120)
    if(blk_ver == -1 && g_cs_ver == CS_VERSION_120)
	blk_ver = BLK_VERSION_120;
#endif
#if defined(CS_VERSION_110)
    if(blk_ver == -1 && g_cs_ver == CS_VERSION_110)
	blk_ver = BLK_VERSION_110;
#endif
    if(blk_ver == -1)
	blk_ver = BLK_VERSION_100;

    /*-- Initialize the command --*/
    if (ct_command( bcp_cmd,                /* Command */
                    CS_LANG_CMD,            /* Type */
                    (CS_VOID*)cmd_sql,      /* Buffer */
                    CS_NULLTERM,            /* Buffer Length */
                    CS_UNUSED ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\bcp: Unable to initialize command structure\n" );
        goto return_fail;
    }

    /*-- Send command to server --*/
    if (ct_send( bcp_cmd ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\bcp: Unable to send command to SQL Server\n" );
        goto return_fail;
    }

    if (sg_interrupted)
        goto return_interrupt;
    
    /*
     * If we have reached this point, then everything looks like it
     * went OK, so it is now time to create a new connection to the
     * destination database.  Note, that we are going to share the
     * same error handler and message handler for this connection as
     * our parent connection.
     */
    if (ct_con_alloc( g_context, &bcp_con ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\bcp: Unable to allocate new BCP connection\n" );
        goto return_fail;
    }

    /*-- Client callback --*/
    if (ct_callback( (CS_CONTEXT*)NULL,         /* Context */
                     bcp_con,                   /* Connection */
                     CS_SET,                    /* Action */
                     CS_CLIENTMSG_CB,           /* Type */
                     (CS_VOID*)bcp_client_cb    /* Callback Pointer */
                   ) != CS_SUCCEED)
        goto return_fail;

    /*-- Server callback --*/
    if (ct_callback( (CS_CONTEXT*)NULL,         /* Context */
                     bcp_con,                   /* Connection */
                     CS_SET,                    /* Action */
                     CS_SERVERMSG_CB,           /* Type */
                     (CS_VOID*)bcp_server_cb    /* Callback Pointer */
                   ) != CS_SUCCEED)
        goto return_fail;             

    /*-- Set Bulk Login --*/
    if (ct_con_props( bcp_con,                    /* Connection */
                      CS_SET,                     /* Action */
                      CS_BULK_LOGIN,              /* Property */
                      (CS_VOID*)&bcp_on,          /* Buffer */
                      CS_UNUSED,                  /* Buffer Lenth */
                      (CS_INT*)NULL               /* Output Length */
                         ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\bcp: Unable to mark connection for BCP\n" );
        goto return_fail;
    }

    /*-- Set username --*/
    if (ct_con_props( bcp_con,                    /* Connection */
                      CS_SET,                     /* Action */
                      CS_USERNAME,                /* Property */
                      (CS_VOID*)username,         /* Buffer */
                      CS_NULLTERM,                /* Buffer Lenth */
                      (CS_INT*)NULL               /* Output Length */
                         ) != CS_SUCCEED)
    {
        fprintf( stderr, 
            "\\bcp: Unable to set username to '%s' for BCP connection\n",
            username );
        goto return_fail;
    }

    /*-- Set password --*/
    if (ct_con_props( bcp_con,                    /* Connection */
                      CS_SET,                     /* Action */
                      CS_PASSWORD,                /* Property */
                      (CS_VOID*)password,         /* Buffer */
                      (password == NULL)?CS_UNUSED:CS_NULLTERM,
                      (CS_INT*)NULL               /* Output Length */
                         ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\bcp: Unable to set password BCP connection\n" );
        goto return_fail;
    }

    /*-- Set application name --*/
    if (ct_con_props( bcp_con,                    /* Connection */
                      CS_SET,                     /* Action */
                      CS_APPNAME,                 /* Property */
                      (CS_VOID*)"sqsh-bcp",       /* Buffer */
                      CS_NULLTERM,                /* Buffer Lenth */
                      (CS_INT*)NULL               /* Output Length */
                          ) != CS_SUCCEED)
    {
        fprintf( stderr, 
            "\\bcp: Unable to set appname to 'sqsh-bcp' for BCP connection\n" );
        goto return_fail;
    }

    /*-- Hostname --*/
    if (hostname != NULL && *hostname != '\0') {
        if (ct_con_props( bcp_con,                 /* Connection */
                              CS_SET,                  /* Action */
                              CS_HOSTNAME,             /* Property */
                              (CS_VOID*)hostname,      /* Buffer */
                              CS_NULLTERM,             /* Buffer Length */
                              (CS_INT*)NULL            /* Output Length */
                             ) != CS_SUCCEED)
        {
            fprintf( stderr, 
                "\\bcp: Unable to set hostname to '%s' for BCP connection\n",
                hostname );
            goto return_fail;
        }
    }

    /*-- Packet Size --*/
    if (packet_size != NULL) {
        i = atoi(packet_size);
        if (ct_con_props( bcp_con,                 /* Connection */
                              CS_SET,                  /* Action */
                              CS_PACKETSIZE,           /* Property */
                              (CS_VOID*)&i,            /* Buffer */
                              CS_UNUSED,               /* Buffer Length */
                              (CS_INT*)NULL            /* Output Length */
                             ) != CS_SUCCEED)
        {
            fprintf( stderr, 
                "\\bcp: Unable to set packetsize to %d for BCP connection\n", 
                (int)i );
            goto return_fail;
        }
    }

    /*-- Encryption --*/
    if (encryption != NULL && *encryption == '1') {
        i = CS_TRUE;
        if (ct_con_props( bcp_con,                 /* Connection */
                              CS_SET,                  /* Action */
                              CS_SEC_ENCRYPTION,       /* Property */
                              (CS_VOID*)&i,            /* Buffer */
                              CS_UNUSED,               /* Buffer Length */
                              (CS_INT*)NULL            /* Output Length */
                             ) != CS_SUCCEED)
        {
            fprintf( stderr, 
                "\\bcp: Unable to set enable encryption for BCP connection\n" );
            goto return_fail;
        }
    }

    /*
     * The following section initializes all locale type information.
     * First, we need to create a locale structure and initialize it
     * with information.
     */
    if (cs_loc_alloc( g_context, &bcp_locale ) != CS_SUCCEED)
    {
        fprintf( stderr, 
            "\\bcp: Unable to allocate locale for BCP connection\n" );
        goto return_fail;
    }

    /*-- Initialize --*/
    if (cs_locale( g_context,                    /* Context */
                   CS_SET,                       /* Action */
                      bcp_locale,                   /* Locale Structure */
                   CS_LC_ALL,                    /* Property */
                      (CS_CHAR*)NULL,               /* Buffer */
                   CS_UNUSED,                    /* Buffer Length */
                   (CS_INT*)NULL                 /* Output Length */
                     ) != CS_SUCCEED)
    {
        fprintf( stderr, 
            "\\bcp: Unable to initialize locale for BCP connection\n" );
        goto return_fail;
    }

    /*-- Language --*/
    if( language != NULL && *language != '\0' ) {
        if (cs_locale( g_context,                 /* Context */
                       CS_SET,                    /* Action */
                       bcp_locale,                /* Locale Structure */
                          CS_SYB_LANG,               /* Property */
                          (CS_CHAR*)language,        /* Buffer */
                          CS_NULLTERM,               /* Buffer Length */
                          (CS_INT*)NULL              /* Output Length */
                         ) != CS_SUCCEED)
        {
            fprintf( stderr, 
                "\\bcp: Unable to set language to '%s' for BCP connection\n",
                language );
            goto return_fail;
        }
    }

    /*-- Character Set --*/
    if (charset != NULL) {
        if (cs_locale( g_context,                 /* Context */
                       CS_SET,                    /* Action */
                       bcp_locale,                /* Locale Structure */
                          CS_SYB_CHARSET,            /* Property */
                          (CS_CHAR*)charset,         /* Buffer */
                          CS_NULLTERM,               /* Buffer Length */
                          (CS_INT*)NULL              /* Output Length */
                         ) != CS_SUCCEED)
        {
            fprintf( stderr, 
                "\\bcp: Unable to set charset to '%s' for BCP connection\n",
                charset );
            goto return_fail;
        }
    }

    /*-- Locale Property --*/
    if (ct_con_props( bcp_con,                 /* Connection */
                          CS_SET,                  /* Action */
                          CS_LOC_PROP,             /* Property */
                          (CS_VOID*)bcp_locale,    /* Buffer */
                          CS_UNUSED,               /* Buffer Length */
                          (CS_INT*)NULL            /* Output Length */
                         ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\bcp: Unable to set locale for BCP connection\n" );
        goto return_fail;
    }

    /*-- Now, connect --*/
    if (ct_connect( bcp_con,
                    server,
                    (server == NULL) ? CS_UNUSED : CS_NULLTERM ) != CS_SUCCEED )
    {
        fprintf( stderr, "\\bcp: Unable to connect to '%s' for BCP connection\n",
                 (server == NULL) ? "NULL" : server );
        goto return_fail;
    }

#if defined(CTLIB_SIGPOLL_BUG) && defined(F_SETOWN)
    /*
     * Please refer to cmd_connect.c for detailed description of
     * why this code is here.
     */
    if (ct_con_props( g_connection,            /* Connection */
                      CS_GET,                  /* Action */
                      CS_ENDPOINT,             /* Property */
                      (CS_VOID*)&ctlib_fd,     /* Buffer */
                      CS_UNUSED,               /* Buffer Length */
                      (CS_INT*)NULL            /* Output Length */
                    ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\bcp: WARNING: Unable to fetch CT-Lib file\n" );
        fprintf( stderr, "\\bcp: descriptor to work around SIGPOLL bug.\n" );
    }
    else
    {
        if (fcntl( ctlib_fd, F_SETOWN, getpid() ) == -1)
        {
            fprintf( stderr,
                "\\bcp: WARNING: Cannot work around CT-Lib SIGPOLL bug: %s\n",
                strerror(errno) );
        }
    }
#endif /* CTLIB_SIGPOLL_BUG */
    
    /*-- Inform signal handler of connection --*/
    sg_bcp_connection = bcp_con;

    /*-- Allocate a block descriptor --*/
    DBG(sqsh_debug(DEBUG_BCP, "bcp: blk_alloc(blk_ver)\n");)

    if (blk_alloc( bcp_con, blk_ver, &bcp_desc ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\bcp: Unable to allocate bulk descriptor\n" );
        goto return_fail;
    }
    
    /*
     * Configure whether or not this connection is to contain the
     * value for the identity column in a result set.  We default
     * this to true.
     */
    if (have_identity == CS_TRUE)
    {
        if (blk_props( bcp_desc,                    /* Descriptor */
                       CS_SET,                      /* Action */
                       BLK_IDENTITY,                /* Property */
                       (CS_VOID*)&have_identity,    /* Buffer */  
                       CS_UNUSED,                   /* Buffer Length*/  
                       (CS_INT*)NULL                /* Output Length */
                      ) == CS_FAIL)
        {
            fprintf( stderr, "\\bcp: Unable to set BLK_IDENTITY option to %s\n",
                        have_identity == CS_TRUE ? "CS_TRUE" : "CS_FALSE" );
            goto return_fail;
        }
    }

    /*-- Initialize the bulk copy --*/
    DBG(sqsh_debug(DEBUG_BCP, "bcp: blk_init(CS_BLK_IN,'%s',%d)\n",
        bcp_table, strlen(bcp_table));)

    if (blk_init( bcp_desc, CS_BLK_IN, bcp_table, 
                  strlen(bcp_table) ) != CS_SUCCEED)
    {
        fprintf( stderr, "\\bcp: Unable to initialize bulk copy on table '%s'\n",
                 bcp_table );
        goto return_fail;
    }

    fprintf(stderr, "\nStarting copy...\n" );
    
    /*
     * Allrightythen.  We have already sent the command to retrieve
     * data through g_dbproc, and have succesfully created bcp_dbproc
     * in order to jam the data into another database, so lets start
     * processing results.
     */
    rows_in_batch = 0;
    nerrors  = 0;

    while ((return_code = ct_results( bcp_cmd, &result_type ))
        != CS_END_RESULTS)
    {
        if (sg_interrupted)
            goto return_interrupt;
        
        if (return_code != CS_SUCCEED)
            break;

        switch (result_type)
        {
            case CS_ROW_RESULT:

                /*-- Destroy old data --*/
                if (bcp_dat != NULL)
                {
                    bcp_data_destroy( bcp_dat );
                }

                /*-- Create new data --*/
                if ((bcp_dat = bcp_data_bind( bcp_cmd, result_type )) == NULL)
                    goto return_fail;

                while ((return_code = 
                        bcp_data_xfer( bcp_dat, 
                                            bcp_cmd,
                                         bcp_desc )) != CS_END_DATA)
                {
                    if (sg_interrupted)
                        goto return_interrupt;

                    if (return_code != CS_SUCCEED)
                    {
                        DBG(sqsh_debug(DEBUG_BCP, "bcp: bcp_data_xfer failed...\n");)

                        if (++nerrors == maxerrors)
                            goto return_fail;
                    }
                    else
                    {
                        ++rows_in_batch;
                    }

                    if (rows_in_batch == batchsize)
                    {
                        DBG(sqsh_debug(DEBUG_BCP, "bcp: blk_done(CS_BLK_BATCH)\n");)

                        if (blk_done( bcp_desc,
                                      CS_BLK_BATCH,
                                      &nrows ) != CS_SUCCEED)
                        {
                            DBG(sqsh_debug(DEBUG_BCP, "bcp: blk_done failed!\n");)

                            if (++nerrors == maxerrors)
                                goto return_fail;
                        }
                        else
                        {
                            total_rows += rows_in_batch;

                            fprintf(stderr,
                                    "Batch successfully bulk-copied to SQL Server.\n");
                        }

                        if (sg_interrupted)
                            goto return_interrupt;

                        rows_in_batch = 0;
                    }
                } /* while (bcp_data_xfer()) */
                break;
            
            case CS_PARAM_RESULT:
            case CS_STATUS_RESULT:
            case CS_COMPUTE_RESULT:
                while ((return_code = ct_fetch( bcp_cmd, CS_UNUSED, CS_UNUSED, 
                    CS_UNUSED, &nrows )) == CS_SUCCEED);
                
                if (return_code != CS_END_DATA)
                {
                    fprintf( stderr,
                        "\\bcp: Error discarding extraneous result sets\n" );
                    
                    goto return_fail;
                }
                break;

            default:
                break;
        } /* switch (result_type) */

    }

    if (rows_in_batch > 0) 
    {
        DBG(sqsh_debug(DEBUG_BCP, "bcp: FINAL: blk_done(CS_BLK_BATCH)\n");)
        if (blk_done( bcp_desc,
                          CS_BLK_BATCH,
                          &nrows ) != CS_SUCCEED)
        {
            goto return_fail;
        }

        if (sg_interrupted)
            goto return_interrupt;

        if (nrows != rows_in_batch) 
        {
            if (++nerrors == maxerrors) 
                goto return_fail;
        }
        total_rows += nrows;
    }

    DBG(sqsh_debug(DEBUG_BCP, "bcp: blk_done(CS_BLK_ALL)\n");)
    if (blk_done( bcp_desc,
                      CS_BLK_ALL,
                      &nrows ) != CS_SUCCEED)
        goto return_fail;

    gettimeofday( &tv_end, NULL );
    
    if (rows_in_batch != 0)
        fprintf(stderr,"Batch successfully bulk-copied to SQL Server\n");

    fprintf( stderr, "\n%d row%s copied.\n", total_rows, 
                (total_rows != 1) ? "s" : "" );

    /* add check for non-zero number of rows passed to avoid
       potential division by 0 error.
       patch by Onno van der Linden */
    if(total_rows > 0) {
        secs = ELAPSED_SEC(tv_start,tv_end);
        fprintf( stderr, 
                 "Clock Time (sec.): Total = %-.4f  Avg = %-.4f (%.2f rows per sec.)\n",
                 secs, secs / (double)total_rows, (double)total_rows / secs );
    }
    
    return_code = CMD_CLEARBUF;
    goto leave;

return_interrupt:
    if (bcp_desc != NULL)
        blk_done( bcp_desc, CS_BLK_CANCEL, &nrows );
    ct_cancel( bcp_con, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
    ct_cancel( g_connection, (CS_COMMAND*)NULL, CS_CANCEL_ALL );

    return_code = CMD_CLEARBUF;
    goto leave;

return_fail:
    DBG(sqsh_debug(DEBUG_ERROR, "bcp: Failure encountered, cleaning up.\n");)

    if (bcp_con != NULL)
    {
        if (ct_con_props( bcp_con,                /* Connection */
                          CS_GET,                 /* Action */
                          CS_CON_STATUS,          /* Property */
                          (CS_VOID*)&con_status,  /* Buffer */
                          CS_UNUSED,              /* Buffer Length */
                          (CS_INT*)NULL ) != CS_SUCCEED)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "bcp:    Unable to get con status.\n");)
            con_status = CS_CONSTAT_CONNECTED;
        }

      /*-- If connected, disconnect --*/
      if (con_status == CS_CONSTAT_CONNECTED)
      {  
            if (bcp_desc != NULL)
            {
                DBG(sqsh_debug(DEBUG_ERROR, "bcp:    Cancelling bcp batch.\n");)
                blk_done( bcp_desc, CS_BLK_CANCEL, &nrows );
            }

            DBG(sqsh_debug(DEBUG_ERROR, "bcp:    Cancelling bcp connection.\n");)
            ct_cancel( bcp_con, (CS_COMMAND*)NULL, CS_CANCEL_ALL );

            DBG(sqsh_debug(DEBUG_ERROR, "bcp:    Closing bcp connection.\n");)
         ct_close( bcp_con, CS_FORCE_CLOSE );
      }

        ct_con_drop( bcp_con );
        bcp_con = NULL;
    }

    if (g_connection != NULL)
    {
        if (ct_con_props( g_connection,           /* Connection */
                          CS_GET,                 /* Action */
                          CS_CON_STATUS,          /* Property */
                          (CS_VOID*)&con_status,  /* Buffer */
                          CS_UNUSED,              /* Buffer Length */
                          (CS_INT*)NULL ) != CS_SUCCEED)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "bcp:    Unable to get con status.\n");)
            con_status = CS_CONSTAT_CONNECTED;
        }

        if (con_status == CS_CONSTAT_CONNECTED)
        {
            DBG(sqsh_debug(DEBUG_ERROR, "bcp:    Cancelling result set.\n");)
            ct_cancel( g_connection, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
        }
        else
        {
            DBG(sqsh_debug(DEBUG_ERROR, "bcp:    Connection dead.\n");)
        }
    }

    return_code = CMD_FAIL;

leave:
    if (bcp_dat != NULL)
        bcp_data_destroy( bcp_dat );

    if (exp_buf != NULL)
        varbuf_destroy( exp_buf );

    if (bcp_cmd != NULL)
        ct_cmd_drop( bcp_cmd );
    
    if (bcp_desc != NULL)
    {
        DBG(sqsh_debug(DEBUG_BCP, "bcp: blk_drop()\n");)
        blk_drop( bcp_desc );
    }
    
    if (bcp_con != NULL)
    {
        ct_close( bcp_con, CS_FORCE_CLOSE );
        ct_con_drop( bcp_con );
    }

    if (bcp_locale != NULL)
        cs_loc_drop( g_context, bcp_locale );
    
    return return_code;
}

static bcp_data_t* bcp_data_bind ( cmd, result_type )
    CS_COMMAND    *cmd;
    CS_INT         result_type;
{
    CS_INT        ncols;
    CS_INT        i;
    bcp_data_t   *d;
    bcp_col_t    *c;

    /*-- Retrieve the number of columns in the result set --*/
    if (ct_res_info( cmd,               /* Command */
                     CS_NUMDATA,        /* Type */
                     (CS_VOID*)&ncols,  /* Buffer */
                     CS_UNUSED,         /* Buffer Length */
                     (CS_INT*)NULL) != CS_SUCCEED)
    {
        fprintf( stderr, "bcp_data_bind: Unable to fetch number of columns\n" );
        return NULL;
    }

    /*-- Fix for bug report 2920048, using calloc instead of malloc --*/
    d = (bcp_data_t*)calloc( 1, sizeof( bcp_data_t ) );
    c = (bcp_col_t*)calloc( ncols, sizeof( bcp_col_t ) );

    if (d == NULL || c == NULL) 
    {
        fprintf( stderr, "bcp_data_bind: Memory allocation failure.\n" );
        if (d != NULL)
            free( d );
        if (c != NULL)
            free( c );
        return NULL;
    }

    /*-- Inialize d --*/
    d->d_type  = result_type;
    d->d_ncols = ncols;
    d->d_cols  = c;

    for (i = 0; i < ncols; i++) 
        d->d_cols[i].c_data = NULL;
    
    for (i = 0; i < ncols; i++)
    {
        c = &d->d_cols[i];
        c->c_colid   = i + 1;
        c->c_len     = -1;
        c->c_nullind = 0;

        /*-- Get description for column --*/
        if (ct_describe( cmd, i+1, &c->c_format ) != CS_SUCCEED) 
        {
            fprintf( stderr, 
                "bcp_data_bind: Unable to fetch column %d description\n",
                (int) i+1 );
            bcp_data_destroy( d );
            return NULL;
        }

        /*-- Allocate enough space to hold data --*/
        /*-- Fix for bug report 2920048, using calloc instead of malloc --*/
        c->c_data = (CS_VOID*)calloc( 1, c->c_format.maxlength );

        /*-- Clean up format --*/
        c->c_format.count  = 1;
        c->c_format.locale = NULL;

        /*-- Bind to the data space --*/
        if (ct_bind( cmd,                            /* Command */
                         i + 1,                          /* Item */
                         &c->c_format,                   /* Format */
                         (CS_VOID*)c->c_data,            /* Buffer */
                         (CS_INT*)&c->c_len,             /* Data Copied */
                         &c->c_nullind                   /* NULL Indicator */
                      ) != CS_SUCCEED)
        {
            fprintf( stderr, 
                "bcp_data_bind: Unable to bind column %d\n",
                (int) i+1 );
            bcp_data_destroy( d );
            return NULL;
        }
    }

    return d;
}

static CS_INT bcp_data_xfer( d, cmd, blkdesc )
    bcp_data_t  *d;
    CS_COMMAND  *cmd;
    CS_BLKDESC  *blkdesc;
{
    CS_RETCODE  return_code;
    CS_INT      nrows;
    CS_INT      i;
    bcp_col_t  *c;

    return_code = ct_fetch( cmd,              /* Command */
                            CS_UNUSED,        /* Type */
                            CS_UNUSED,        /* Offset */
                            CS_UNUSED,        /* Option */
                            &nrows );
    
    if (return_code != CS_SUCCEED)
    {
        return return_code;
    }

    for (i = 0; i < d->d_ncols; i++)
    {
        c = &d->d_cols[i];

        /*
         * Here we determine whether or not we need to (re-) bind to
         * the incoming data.  If the length of the data has changed
         * since the last row, or if it has become NULL, then we
         * want to re-bind.
         */
        if (c->c_len == -1 || c->c_len != c->c_prevlen ||
            c->c_nullind != c->c_prevnullind)
        {
            DBG(sqsh_debug(DEBUG_BCP,
                "bcp: blk_bind( %d, DATAFMT, %d, %d )\n",
                (int)i + 1, (int)c->c_len, (int)c->c_nullind);)

            if (c->c_nullind == -1)
            {
                c->c_len = 0;
            }

            c->c_prevlen     = c->c_len;
            c->c_prevnullind = c->c_nullind;

            if (blk_bind( blkdesc,              /* Block Descriptor */
                              i + 1,                /* Column Number */
                              &c->c_format,         /* Data Format */
                              (CS_VOID*)c->c_data,  /* Buffer */
                              &c->c_len,                /* Buffer Length */
                              &c->c_nullind ) != CS_SUCCEED)
            {
                fprintf( stderr, 
                    "bcp_data_xfer: Unable to bind results for column %d\n", 
                    (int) i+1 );
                return CS_FAIL;
            }
        }
    }

    DBG(sqsh_debug(DEBUG_BCP, "bcp: blk_rowxfer()\n");)
    if (blk_rowxfer( blkdesc ) != CS_SUCCEED)
    {
        fprintf( stderr, 
            "bcp_data_xfer: Unable to transfer row to remote SQL Server\n" );
        return CS_FAIL;
    }

    return CS_SUCCEED;
}

/*
 * bcp_data_destroy():
 *
 * Destroys a bcp_data_t structure.
 */
static void bcp_data_destroy( d )
    bcp_data_t   *d;
{
    CS_INT  i;

    if (d != NULL)
    {
        if (d->d_cols != NULL)
        {
            for (i = 0; i < d->d_ncols; i++)
            {
                if (d->d_cols[i].c_data != NULL)
                    free( d->d_cols[i].c_data );
            }

            free( d->d_cols );
        }

        free( d );
    }
}

/*
 * bcp_signal():
 *
 * This function is called whenever a signal is received, its
 * only real job is to register the fact that a signal was caught
 * during processing via the sg_interrupted flag.
 */
static void bcp_signal( sig, user_data )
    int sig;
    void *user_data;
{
    sg_interrupted = True;

    if (sg_bcp_connection != NULL)
    {
        ct_cancel( sg_bcp_connection, (CS_COMMAND*)NULL, CS_CANCEL_ATTN );
    }
    ct_cancel( g_connection, (CS_COMMAND*)NULL, CS_CANCEL_ALL );
}


/*
 * bcp_server_cb():
 *
 * Sybase callback message handler for all messages coming from the
 * SQL Server or Open Server.
 *
 */
static CS_RETCODE bcp_server_cb (ctx, con, msg)
    CS_CONTEXT     *ctx;
    CS_CONNECTION  *con;
    CS_SERVERMSG   *msg;
{
    /*
     * Ignore "database changed", or "language changed" messages from
     * the server.
     */
    if( msg->msgnumber == 5701 ||    /* database context change */
         msg->msgnumber == 5703 ||    /* language changed */
         msg->msgnumber == 5704 )     /* charset changed */
    {
        return CS_SUCCEED;
    }

    if (msg->severity >= 0)
    {
        /*
         * If the severity is something other than 0 or the msg number is
         * 0 (user informational messages).
         */
        if (msg->severity >= 0 || msg->msgnumber == 0) 
        {
            /*
             * If the message was something other than informational, and
             * the severity was greater than 0, then print information to
             * stderr with a little pre-amble information.  According to 
             * the Sybase System Administrator's guide, severity level 10
             * messages should not display severity information.
             */
            if (msg->msgnumber > 0 && msg->severity > 10)
            {
                fprintf( stderr, "Msg %d, Level %d, State %d\n",
                            (int)msg->msgnumber, (int)msg->severity, (int)msg->state );
                if (msg->svrnlen > 0)
                    fprintf( stderr, "Server '%s'", (char*)msg->svrname );
                if (msg->proclen > 0)
                    fprintf( stderr, ", Procedure '%s'", (char*)msg->proc );
                if( msg->line > 0 )
                    fprintf( stderr, ", Line %d", (int)msg->line );
                fprintf( stderr, "\n" );
                fprintf( stderr, "%s\n", msg->text );
                fflush( stderr );
            }
            else 
            {
                /*
                 * Otherwise, it is just an informational (e.g. print) message
                 * from the server, so send it to stdout.
                 */
                fprintf( stdout, "%s\n", msg->text );
                fflush( stdout );
            }
        }
    }

    return CS_SUCCEED;
}

/*
 * bcp_client_cb():
 */
static CS_RETCODE bcp_client_cb ( ctx, con, msg )
    CS_CONTEXT    *ctx;
    CS_CONNECTION *con;
    CS_CLIENTMSG  *msg;
{
    /*
     * Let the CS-Lib handler print the message out.
     */
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
    fprintf( stderr, "%s\n", msg->msgstring );
    fflush( stderr ) ;

    return CS_SUCCEED ;
}
