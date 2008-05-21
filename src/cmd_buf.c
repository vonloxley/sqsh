/*
 * cmd_buf.c - Buffer manipulation commands
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
#include <sys/param.h>
#include "sqsh_config.h"
#include "sqsh_global.h"
#include "sqsh_varbuf.h"
#include "sqsh_error.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "sqsh_getopt.h"
#include "sqsh_buf.h"
#include "sqsh_stdin.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_buf.c,v 1.1.1.1 2004/04/07 12:35:04 chunkm0nkey Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_buf_copy():
 *
 * Implements user command \buf-copy, which copies contents of one
 * buffer to another.
 */
int cmd_buf_copy( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	char  *src_buf = "!." ;  /* The source always defaults to !. */
	char  *dst_buf ;         /* Destincation must be supplied */
	char  *buf ;

	/*
	 * Check your arguments.
	 */
	if( argc < 2 || argc > 3 ) {
		fprintf( stderr, "Use: \\buf-copy dest-buf [source-buf]\n" ) ;
		return CMD_FAIL ;
	}

	/*
	 * The destination buffer is always the first argument on the
	 * command line.
	 */
	dst_buf = argv[1] ;

	/*
	 * If there is another argument, then it is the name of the
	 * source buffer.
	 */
	if( argc == 3 )
		src_buf = argv[2] ;

	/*
	 * Attempt to retrieve the contents of the source buffer.
	 */
	if( (buf = buf_get( src_buf )) == NULL ) {
		fprintf( stderr, "\\buf-copy: %s: %s\n", src_buf, sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	/*
	 * Now copy the contents of the source buffer into the destination
	 * buffer.
	 */
	if( buf_put( dst_buf, buf ) == False ) {
		fprintf( stderr, "\\buf-copy: %s: %s\n", dst_buf, sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	/*
	 * If the destination buffer was actually the current work buffer, then
	 * we need to tell the read-eval-print loop to re-display its contents,
	 * otherwise, just return.
	 */
	if( strcmp( dst_buf, "!." ) == 0 )
		return CMD_ALTERBUF ;

	return CMD_LEAVEBUF ;
}

/*
 * cmd_buf_get():
 *
 * Implements user command \buf-get, which is identical to typing
 * \buf-copy !. [buffer].
 */
int cmd_buf_get( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	char   new_bufname[64] ;
	char  *src_buf ;
	char  *buf ;

	/*
	 * Check your arguments.
	 */
	if( argc != 2 ) {
		fprintf( stderr, "Use: \\buf-get buffer-name\n" ) ;
		return CMD_FAIL ;
	}
	src_buf = argv[1] ;

	if( *src_buf != '!' ) {
		sprintf( new_bufname, "!%s", src_buf ) ;
		src_buf = new_bufname ;
	} else if( strcmp( src_buf, "!" ) == 0 ) {
		strcpy( new_bufname, "!!" ) ;
		src_buf = new_bufname ;
	}
		

	/*
	 * Attempt to retrieve the contents of the source buffer.
	 */
	if( (buf = buf_get( src_buf )) == NULL ) {
		fprintf( stderr, "\\buf-get: %s: %s\n", src_buf, sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	if( varbuf_strcat( g_sqlbuf, buf ) == -1 ) {
		fprintf( stderr, "\\buf_get: varbuf_strcat: %s\n", sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}
	return CMD_ALTERBUF ;
}

/*
 * cmd_buf_append():
 *
 * Implements user command \buf-append, which copies contents of one
 * buffer on to the end of another.
 */
int cmd_buf_append( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	varbuf_t  *tmp_buf ;
	char      *src_buf = "!." ;  /* The source always defaults to !. */
	char      *dst_buf ;         /* Destincation must be supplied */
	char      *dst, *src ;

	/*
	 * Check your arguments.
	 */
	if( argc < 2 || argc > 3 ) {
		fprintf( stderr, "Use: \\buf-append dest-buf [source-buf]\n" ) ;
		return CMD_FAIL ;
	}

	/*
	 * The destination buffer is always the first argument on the
	 * command line.
	 */
	dst_buf = argv[1] ;

	/*
	 * If there is another argument, then it is the name of the
	 * source buffer.
	 */
	if( argc == 3 )
		src_buf = argv[2] ;

	/*
	 * Retrieve the contents of the source buffer.
	 */
	if( (src = buf_get( src_buf )) == NULL ) {
		fprintf( stderr, "\\buf-append: %s: %s\n", src_buf, sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	/*
	 * Retrieve the contents of the destination buffer, if there
	 * are any.
	 */
	dst = buf_get( dst_buf ) ;

	/*
	 * Allocate a buffer to hold the "grand-unified" contents of the
	 * source buffer and the destination buffer.
	 */
	if( (tmp_buf = varbuf_create( 1024 )) == NULL ) {
		fprintf( stderr, "\\buf-append: varbuf_create: %s\n",
					sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	/*
	 * If there was anything in the destination buffer, then copy
	 * the conents into our temporary buffer.
	 */
	if( dst != NULL ) {
		if( varbuf_strcpy( tmp_buf, dst ) == -1 ) {
			fprintf( stderr, "\\buf-append: varbuf_strcpy: %s\n",
						sqsh_get_errstr() );
			varbuf_destroy( tmp_buf ) ;
			return CMD_FAIL ;
		}
	}

	/*
	 * Concatinate the contents of the source buffer onto the contents
	 * of the destination buffer.
	 */
	if( varbuf_strcat( tmp_buf, src ) == -1 ) {
		fprintf( stderr, "\\buf-append: varbuf_strcpy: %s\n",
					sqsh_get_errstr() );
		varbuf_destroy( tmp_buf ) ;
		return CMD_FAIL ;
	}
		
	/*
	 * Now copy the contents of the unified buffer back into the
	 * destination buffer.
	 */
	if( buf_put( dst_buf, varbuf_getstr( tmp_buf ) ) == False ) {
		fprintf( stderr, "\\buf-append: %s: %s\n", dst_buf, sqsh_get_errstr() ) ;
		varbuf_destroy( tmp_buf ) ;
		return CMD_FAIL ;
	}

	varbuf_destroy( tmp_buf ) ;

	/*
	 * If the destination buffer was actually the current work buffer, then
	 * we need to tell the read-eval-print loop to re-display its contents,
	 * otherwise, just return.
	 */
	if( strcmp( dst_buf, "!." ) == 0 )
		return CMD_ALTERBUF ;

	return CMD_LEAVEBUF ;
}

/*
 * cmd_buf_save():
 *
 * Saves contents of user buffer into file.
 */
int cmd_buf_save( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	char        *buf_name   = "!." ;
	char        *mode       = "w" ;
	char        *save_file ;
	extern int   sqsh_optind ;
	int          c ;
	int          have_error = False ;

	/*
	 * Process the command line argument(s).  This is a little
	 * overkill for now, but we could have more options in the
	 * future.
	 */
	while( (c = sqsh_getopt( argc, argv, "a" )) != EOF ) {

		switch( c ) {

			case 'a' :             /* Append to save file */
				mode = "w+" ;
				break ;

			default :
				fprintf( stderr, "\\buf-save: %s\n", sqsh_get_errstr() ) ;
				have_error = True ;

		}

	}

	/*
	 * If we encountered an error above, or we don't have any arguments
	 * left on the command line, or we have too many arguments, then
	 * print usage.
	 */
	if( have_error || argc == sqsh_optind || (argc - sqsh_optind) > 2  ) {
		fprintf( stderr, "\\buf-save [-a] filename [source-buf]\n" ) ;
		return CMD_FAIL ;
	}

	save_file = argv[sqsh_optind++] ;
	if( sqsh_optind != argc )
		buf_name = argv[ sqsh_optind++ ] ;

	/*-- Attempt to save the buffer --*/
	if( buf_save( buf_name, save_file, mode ) == False ) {
		fprintf( stderr, "\\buf-save: %s: %s\n", buf_name, sqsh_get_errstr() ) ;
		return CMD_FAIL ;
	}

	return CMD_LEAVEBUF ;
}

int cmd_buf_load( argc, argv )
	int    argc;
	char  *argv[];
{
	char        *buf_name   = "!.";
	char        *load_file;
	int          append = False;
	int          have_error = False;
	int          ch;
	extern int   sqsh_optind;

	while ((ch = sqsh_getopt( argc, argv, "a" )) != -1)
	{
		switch (ch)
		{
			case 'a':
				append = True;
				break;
			default:
				fprintf( stderr, "\\buf-load: -%c: Invalid option\n", ch );
				have_error = True;
				return CMD_FAIL;
		}
	}

	/*
	 * If we don't have any arguments on the command line, or we have too
	 * many arguments, then print usage.
	 */
	if (have_error == True || 
	    (argc - sqsh_optind) < 1 || 
	    (argc - sqsh_optind) > 2)
	{
		fprintf( stderr, "\\buf-load [-a] filename [dest-buf]\n" );
		return CMD_FAIL;
	}

	load_file = argv[sqsh_optind];

	if (sqsh_optind+1 < argc)
	{
		buf_name = argv[sqsh_optind+1];
	}

	/*-- Attempt to save the buffer --*/
	if (buf_load( buf_name, load_file, append ) == False)
	{
		fprintf( stderr, "\\buf-load: %s: %s\n", buf_name, sqsh_get_errstr() );
		return CMD_FAIL;
	}

	/*
	 * If we just loaded the file into the current working buffer, then
	 * we had better have the buffer re-displayed for the user.
	 */
	if (strcmp( buf_name, "!." ) == 0)
	{
		return CMD_ALTERBUF;
	}

	return CMD_LEAVEBUF;
}

int cmd_buf_show( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	var_t   *v ;
	char    *buf_name ;
	char    *line ;
	char    *nl ;
	int      i ;
	int      count ;

	if( argc > 2 ) {
		fprintf( stderr, "Use: \\buf-show [buffername]\n" ) ;
		return False ;
	}

	if( argc == 2 )
		buf_name = argv[1] ;
	else
		buf_name = NULL ;

	/*
	 * Once again, I am breaking my own golden rule by having this
	 * command have access to the inner workings of the env_t data
	 * structure.  Oh well.  Anyway, here I just blast through the
	 * hash table, displaying anything I run across.
	 */
 	count = 0 ;
	for( i = 0; i < g_buf->env_hsize; i++ ) {
		for( v = g_buf->env_htable[i]; v != NULL; v = v->var_nxt ) {

			if( buf_name == NULL || strcmp( buf_name, v->var_name ) == 0 ) {
				printf( "==> %s\n", v->var_name ) ;

				line = v->var_value ;
				while( (nl = strchr( line, '\n' )) != NULL ) {
					printf( "   %*.*s\n", nl - line, nl - line, line ) ;
					line = nl + 1 ;
				}

				if( *line != '\0' )
					printf( "   %s\n", line ) ;

				++count ;
			}
		}
	}

	/*
	 * If the user requested that we show a particular buffer and
	 * we couldn't find it, then print an error message.
	 */
 	if( buf_name != NULL && count == 0 ) {
 		fprintf( stderr, "\\buf-show: Buffer %s does not exist\n", buf_name ) ;
 		return CMD_FAIL ;
	}

	return CMD_LEAVEBUF ;
}

/*
 * cmd_buf_edit:
 */
int cmd_buf_edit( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	char         path[SQSH_MAXPATH+1] ;  /* Location of edit file */
	char         str[SQSH_MAXPATH+1] ;   /* Command to be executed */
	char        *editor ;               /* Which editor to use */
	char        *tmp_dir ;              /* Where file is located */
	int          have_error = False ;   /* No errors encountered */
	char        *read_buf  = "!." ;     /* Default read location */
	char        *write_buf = "!." ;     /* Default write location */
	extern char *sqsh_optarg ;
	extern int   sqsh_optind ;
	int          c ;

	/*
	 * The editor can only be run in interactive mode.
	 */
	if (!sqsh_stdin_isatty())
	{
		fprintf( stderr, "\\buf-edit: Not in interactive mode\n" );
		return CMD_FAIL;
	}

	/*
	 * We default the read buffer to the global buffer (the current
	 * working buffer), if it is not empty.  If it is empty then we
	 * default to the most recent history.
	 */
	if (varbuf_getlen( g_sqlbuf ) == 0)
		read_buf = "!!";

	while ((c = sqsh_getopt( argc, argv, "r:w:" )) != EOF)
	{
		switch( c )
		{
			/*
			 * If the user wants to read from a special buffer then we
			 * need to retrieve the contents of that buffer.
			 */
			case 'r' :
				read_buf = sqsh_optarg;
				break;

			/*
			 * If the user wants us to save to a particular buffer then
			 * we need to check that it is writtable before continuing.
			 */
			case 'w' :
				write_buf = sqsh_optarg;
				break;

			default:
				have_error = True;

		} /* switch */

	} /* while */

	if (sqsh_optind != argc || have_error)
	{
		fprintf( stderr, "Use: \\buf-edit [-r read-buf] [-w write-buf]\n" );
		return CMD_FAIL;
	}

	/*
	 * Now, we need to make sure that the buffers that we want to read
	 * from and write to actually exist.
	 */
 	if (buf_can_put( write_buf ) == False)
	{
 		fprintf( stderr, "\\buf-edit: %s\n", sqsh_get_errstr() );
 		return CMD_FAIL;
	}

	/*
	 * Attempt to retrieve the name of the editor from our environment.
	 */
 	if (strcmp(argv[0], "edit" )       == 0 || 
	    strcmp(argv[0], "\\edit" )     == 0 ||
	    strcmp(argv[0], "\\buf-edit" ) == 0)
	{
		env_get( g_env, "EDITOR", &editor );
		if (editor == NULL)
			env_get( g_env, "VISUAL", &editor );
		if (editor == NULL)
			editor = SQSH_EDITOR;
	} else
		editor = argv[0];

	env_get( g_env, "tmp_dir", &tmp_dir );

	if (tmp_dir == NULL)
		tmp_dir = SQSH_TMP;

	/*
	 * Build the name of the file in which we will place the buffer
	 * to be edited.  Unfortunately I don't like this scheme very
	 * much.  It doesn't lend itself to multiple contexts within
	 * the same program.
	 */
	sprintf( path, "%s/sqsh-edit.%d.sql", tmp_dir, (int)getpid() );

	/*
	 * Attempt to save the read buffer into the file that we built.
	 * If we fail it was probably because we have the name of a bad
	 * buffer.
	 */
	if (buf_save( read_buf, path, "w" ) == False)
	{
		fprintf( stderr, "\\buf-edit: %s\n", sqsh_get_errstr() );
		return CMD_FAIL;
	}

	/*
	 * Build the command to be execed by the system.
	 */
	sprintf( str, "%s %s\n", editor, path );

	/*
	 * Run the editor.
	 */
	system(str);

	if (buf_load( write_buf, path, False ) == False)
	{
		unlink( path );
		fprintf( stderr, "\\buf-edit: %s\n", sqsh_get_errstr() );
		return CMD_FAIL;
	}

	/*
	 * Remove the tmp file from the system.
	 */
	unlink( path );

	return CMD_ALTERBUF;
}
