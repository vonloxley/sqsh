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
#include <ctpublic.h>
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_global.h"
#include "sqsh_getopt.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "sqsh_job.h"
#include "sqsh_init.h"
#include "sqsh_stdin.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_connect.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * sg_login:  The following variable is set to True when we are in the
 *            middle of the login process.  It prevents the error handler
 *            from exiting due to a dead or NULL CS_CONTEXT structure.
 */
static int sg_login = False;
static int sg_login_failed = False;

/*-- Local Prototypes --*/
static CS_RETCODE syb_server_cb 
	_ANSI_ARGS(( CS_CONTEXT*, CS_CONNECTION*, CS_SERVERMSG* ))
#if defined(_CYGWIN32_)
	__attribute__ ((stdcall))
#endif /* _CYGWIN32_ */
	;

static CS_RETCODE syb_client_cb 
	_ANSI_ARGS(( CS_CONTEXT*, CS_CONNECTION*, CS_CLIENTMSG* ))
#if defined(_CYGWIN32_)
	__attribute__ ((stdcall))
#endif /* _CYGWIN32_ */
	;

static CS_RETCODE syb_cs_cb
	_ANSI_ARGS(( CS_CONTEXT*, CS_CLIENTMSG* ))
#if defined(_CYGWIN32_)
	__attribute__ ((stdcall))
#endif /* _CYGWIN32_ */
	;

static int wrap_print _ANSI_ARGS(( FILE*, char* )) ;

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
 *    interfaces -   Locatio of sybase interfaces file (overridden by -I)
 *    colsep     -   Column seperator
 *    width      -   Display width
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
	char      *colsep ;
	char      *width ;
	char      *packet_size ;
	char      *autouse ;
	char      *session ;
	char      *charset ;
	char      *language ;
	char      *encryption ;
	char      *hostname ;
	char      *password_retry;
	char      *tds_version;
	char      *cp;
	extern    char *sqsh_optarg ;
	extern    int   sqsh_optind ;
	char      use_database[31] ;
	int       c ;
	int       have_error = False ;
	int       preserve_context   = True ;
	char      orig_password[64];
	int       password_changed = False;
	char      sqlbuf[64];
	char      passbuf[64];
	int       len;
	int       exit_status;
	int       i;
	int       return_code;
	CS_INT    version;

#if defined(CTLIB_SIGPOLL_BUG) && defined(F_SETOWN)
	int       ctlib_fd;
#endif

	CS_RETCODE  result_type;
	CS_LOCALE  *locale = NULL;
	CS_COMMAND *cmd;
#if !defined(_WINDOZE_)
	CS_INT      netio_type;
#endif
	CS_INT      con_status;

	/*
	 * First, we want to establish an environment "transaction". This
	 * will allow us to change whichever environment variables we
	 * would like, then roll things back if we want to fail.
	 */
	env_tran( g_env );

	/*
	 * Parse the command line options.
	 */
	while ((c = sqsh_getopt( argc, argv, "D:U:P;S;I;c" )) != EOF) 
	{
		switch( c ) 
		{
			case 'D' :
					if (env_put( g_env, "database", sqsh_optarg, 
					             ENV_F_TRAN ) == False)
					{
						fprintf( stderr, "\\connect: -D: %s\n",
									sqsh_get_errstr() );
						have_error = True;
					}
					break;
			case 'c' :
					preserve_context = False ;
					break ;
			case 'I'  :
					if (env_put( g_env, "interfaces", sqsh_optarg, 
					             ENV_F_TRAN ) == False)
					{
						fprintf( stderr, "\\connect: -I: %s\n",
									sqsh_get_errstr() );
						have_error = True;
					}
				break ;
			case 'S'  :    /* Server name */
					if (env_put( g_env, "DSQUERY", sqsh_optarg,
					             ENV_F_TRAN ) == False)
					{
						fprintf( stderr, "\\connect: -S: %s\n",
									sqsh_get_errstr() );
						have_error = True;
					}
				break ;
			case 'U' :
					if (env_put( g_env, "username", sqsh_optarg,
					             ENV_F_TRAN ) == False)
					{
						fprintf( stderr, "\\connect: -U: %s\n",
									sqsh_get_errstr() );
						have_error = True;
					}
				break ;
			case 'P' :
					strcpy( orig_password, g_password );
					password_changed = True;

					if (env_put( g_env, "password", sqsh_optarg,
					             ENV_F_TRAN ) == False)
					{
						fprintf( stderr, "\\connect: -P: %s\n",
									sqsh_get_errstr() );
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
	 * If there are any options left on the end of the line, then
	 * we have an error.
	 */
	if( have_error || sqsh_optind != argc ) 
	{
		fprintf( stderr, 
			"Use: \\connect [-c] [-I interfaces] [-U username] [-P password]\n"
			"               [-S server]\n" ) ;
	
		env_rollback( g_env );
		return CMD_FAIL;
	}

	/*
	 * Retrieve the values of the various environment variables that
	 * may be used to connect.  These may be overridden with flags
	 * to connect.
	 */
	env_get( g_env, "username", &username ) ;
	env_get( g_env, "DSQUERY",  &server ) ;
	env_get( g_env, "interfaces", &interfaces ) ;
	env_get( g_env, "width", &width ) ;
	env_get( g_env, "colsep", &colsep ) ;
	env_get( g_env, "packet_size", &packet_size ) ;
	env_get( g_env, "autouse", &autouse ) ;
	env_get( g_env, "database", &database ) ;
	env_get( g_env, "charset", &charset ) ;
	env_get( g_env, "language", &language ) ;
	env_get( g_env, "encryption", &encryption ) ;
	env_get( g_env, "hostname", &hostname ) ;
	env_get( g_env, "password_retry", &password_retry ) ;
	env_get( g_env, "tds_version", &tds_version ) ;
	password = g_password;

	/*
	 * If we are already connected to the database, then don't
	 * bother to do anything.
	 */
	if (g_connection != NULL)
	{
		env_rollback( g_env );
		return CMD_LEAVEBUF ;
	}

	/*
	 * If the $session variable is set and the path that it contains
	 * is a valid path name, then we want to execute the contents of
	 * this file immediatly prior to connecting to the database.
	 */
	env_get( g_env, "session", &session );
	if (session != NULL && access( session, R_OK ) != -1) 
	{
		if ((jobset_run( g_jobset, "\\loop -n $session", &exit_status )) == -1 ||
		    exit_status == CMD_FAIL) 
		{
			fprintf( stderr, "%s\n", sqsh_get_errstr() );
			goto connect_fail;
		}
	}

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
	if (g_password_set == False)
	{
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
	}

	/*
	 * If the $database variable is set *prior* to connecting to the
	 * database, then that indicates that we are to automatically
	 * connect to $database prior to returning.
	 */
	if( preserve_context && database != NULL && *database != '\0' ) 
	{
		sprintf( use_database, "%s", database ) ;
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
		if (cs_ctx_alloc( CS_VERSION_100, &g_context ) != CS_SUCCEED)
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
		if (ct_init( g_context, CS_VERSION_100 ) != CS_SUCCEED)
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
		 * If the user overrode the default interfaces file location, then
		 * configure the interfaces file as such.
		 */
		if (interfaces != NULL) 
		{
			if (ct_config( g_context,                  /* Context */
								CS_SET,                     /* Action */
								CS_IFILE,                   /* Property */
								(CS_VOID*)interfaces,       /* Buffer */
								CS_NULLTERM,                /* Buffer Length */
								NULL                        /* Output Length */
							 ) != CS_SUCCEED)
				goto connect_fail;
		}

	} /* if (g_context == NULL) */

	/*
	 * Allocate a new connection and initialize the username and
	 * password for the connection.
	 */
	if (ct_con_alloc( g_context, &g_connection ) != CS_SUCCEED)
		goto connect_fail;

	/*-- Set username --*/
	if (ct_con_props( g_connection,               /* Connection */
	                  CS_SET,                     /* Action */
	                  CS_USERNAME,                /* Property */
	                  (CS_VOID*)username,         /* Buffer */
	                  CS_NULLTERM,                /* Buffer Lenth */
	                  (CS_INT*)NULL               /* Output Length */
						 ) != CS_SUCCEED)
		goto connect_fail;

	/*-- Set password --*/
	if (ct_con_props( g_connection,               /* Connection */
	                  CS_SET,                     /* Action */
	                  CS_PASSWORD,                /* Property */
	                  (CS_VOID*)g_password,       /* Buffer */
	                  (g_password == NULL) ? CS_UNUSED : CS_NULLTERM,
	                  (CS_INT*)NULL               /* Output Length */
						 ) != CS_SUCCEED)
		goto connect_fail;

	/*-- Set application name --*/
	if (ct_con_props( g_connection,               /* Connection */
	                  CS_SET,                     /* Action */
	                  CS_APPNAME,                 /* Property */
	                  (CS_VOID*)"sqsh",           /* Buffer */
	                  CS_NULLTERM,                /* Buffer Lenth */
	                  (CS_INT*)NULL               /* Output Length */
						  ) != CS_SUCCEED)
		goto connect_fail;
	
	if (tds_version != NULL && tds_version != '\0')
	{
		if (strcmp(tds_version, "4.0") == 0)
			version = CS_TDS_40;
		else if (strcmp(tds_version, "4.2") == 0)
			version = CS_TDS_42;
		else if (strcmp(tds_version, "4.6") == 0)
			version = CS_TDS_46;
		else if (strcmp(tds_version, "5.0") == 0)
			version = CS_TDS_50;

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
	if (packet_size != NULL) 
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
		if (cs_locale( g_context,                 /* Context */
		               CS_SET,                    /* Action */
		               locale,                    /* Locale Structure */
					      CS_SYB_LANG,               /* Property */
					      (CS_CHAR*)language,        /* Buffer */
					      CS_NULLTERM,               /* Buffer Length */
					      (CS_INT*)NULL              /* Output Length */
						 ) != CS_SUCCEED)
			goto connect_fail;
	}

	/*-- Character Set --*/
	if (charset != NULL) 
	{
		if (cs_locale( g_context,                 /* Context */
		               CS_SET,                    /* Action */
		               locale,                    /* Locale Structure */
					      CS_SYB_CHARSET,            /* Property */
					      (CS_CHAR*)charset,         /* Buffer */
					      CS_NULLTERM,               /* Buffer Length */
					      (CS_INT*)NULL              /* Output Length */
						 ) != CS_SUCCEED)
			goto connect_fail;
	}

	/*-- Locale Property --*/
	if (ct_con_props( g_connection,            /* Connection */
					      CS_SET,                  /* Action */
					      CS_LOC_PROP,             /* Property */
					      (CS_VOID*)locale,        /* Buffer */
					      CS_UNUSED,               /* Buffer Length */
					      (CS_INT*)NULL            /* Output Length */
						 ) != CS_SUCCEED)
		goto connect_fail;
	

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
							 (server == NULL)?CS_UNUSED:CS_NULLTERM ) != CS_SUCCEED)
		{
			if (*password_retry != '1' || !sqsh_stdin_isatty() ||
				sg_login_failed != True)
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

	/*-- If autouse has been set, use it --*/
	if (autouse != NULL && *autouse != '\0') 
	{

		if (ct_cmd_alloc( g_connection, &cmd ) != CS_SUCCEED)
			goto connect_succeed;

		sprintf( sqlbuf, "use %s", autouse );

		if (ct_command( cmd,                /* Command Structure */
		                CS_LANG_CMD,        /* Command Type */
		                sqlbuf,             /* Buffer */
		                CS_NULLTERM,        /* Buffer Length */
		                CS_UNUSED           /* Options */
		              ) != CS_SUCCEED) 
		{
			ct_cmd_drop( cmd );
			goto connect_succeed;
		}

		if (ct_send( cmd ) != CS_SUCCEED) 
		{
			ct_cmd_drop( cmd );
			goto connect_succeed;
		}

		while (ct_results( cmd, &result_type ) != CS_END_RESULTS);
		ct_cmd_drop( cmd );

	}

connect_succeed:
	/*
	 * Now, since the connection was succesfull, we want to change
	 * any environment variables to accurately reflect the current
	 * connection.
	 */
	env_commit( g_env );

	if (locale != NULL)
		cs_loc_drop( g_context, locale );

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
	if (g_connection != NULL) 
	{

		/*-- Find out if the we are connected or not --*/
		if (ct_con_props( g_connection,           /* Connection */
		                  CS_GET,                 /* Action */
		                  CS_CON_STATUS,          /* Property */
		                  (CS_VOID*)&con_status,  /* Buffer */
		                  CS_UNUSED,              /* Buffer Length */
								(CS_INT*)NULL ) != CS_SUCCEED)
		{
			DBG(sqsh_debug(DEBUG_ERROR, "connect:   Unable to get con status.\n");)
			con_status = CS_CONSTAT_CONNECTED;
		}

		/*-- If connected, disconnect --*/
		if (con_status == CS_CONSTAT_CONNECTED)
		{
			DBG(sqsh_debug(DEBUG_ERROR, "connect:   Closing connection\n");)
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

	if (locale != NULL)
	{
		DBG(sqsh_debug(DEBUG_ERROR, "connect:   Dropping locale\n");)
		cs_loc_drop( g_context, locale );
	}

	if (g_context != NULL)
	{
		DBG(sqsh_debug(DEBUG_ERROR, "connect:   Dropping context\n");)
		ct_exit( g_context, CS_FORCE_EXIT );
		cs_ctx_drop( g_context );
	}

	g_connection = NULL;
	g_context    = NULL;
	return_code = CMD_FAIL;

connect_leave:
	sg_login = False;
	return return_code;
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

	/*
	 * Record last error in $?
	 */
	if (msg->severity > 10)
	{
		sprintf( var_value, "%d", msg->msgnumber );
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
	 * If we got a login failed message, then record it as such as return.
	 */
	if (msg->msgnumber == 4002)
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
				wrap_print( stderr, msg->text );
				fflush( stderr );
			}
			else 
			{
				/*
				 * Otherwise, it is just an informational (e.g. print) message
				 * from the server, so send it to stdout.
				 */
				wrap_print( stdout, msg->text );
				fflush( stdout );
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
	 * the server, then ignore it.
	 */
	if (CS_NUMBER(msg->msgnumber) == 44 && sg_login_failed == True)
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
 * syb_err_handler():
 */
static CS_RETCODE syb_client_cb ( ctx, con, msg )
	CS_CONTEXT    *ctx;
	CS_CONNECTION *con;
	CS_CLIENTMSG  *msg;
{
	/*
	 * Let the CS-Lib handler print the message out.
	 */
	(void)syb_cs_cb( ctx, msg );

	/*
	 * If the dbprocess is dead or the dbproc is a NULL pointer and
	 * we are not in the middle of logging in, then we need to exit.
	 * We can't do anything from here on out anyway.
	 */
	if (sg_login == False)
	{
		if (CS_SEVERITY(msg->msgnumber) >= CS_SV_COMM_FAIL || ctx == NULL || 
		    con == NULL)
		{
			DBG(sqsh_debug(DEBUG_ERROR,
				"syb_client_cb: Aborting on severity %d\n", 
				CS_SEVERITY(msg->msgnumber) );)

			sqsh_exit(255) ;
		}
	}

	return CS_SUCCEED ;
}
