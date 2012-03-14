/*
 * sqsh_buf.c - "Smart" named buffer functions
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
#include "sqsh_global.h"
#include "sqsh_error.h"
#include "sqsh_stdin.h"
#include "sqsh_buf.h"
#include "sqsh_readline.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_buf.c,v 1.3 2010/01/26 15:03:50 mwesdorp Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * Note: This whole module is quite a bit different from all other
 * sqsh_*.c modules, in that it provides no real data for itself.
 * Instead it is intended as a single interfaces for manipulating
 * the text buffers that are stored in various global data structure
 * (see sqsh_global.h). I don't think I am particularly happy
 * with this design right now.
 */

/*
 * buf_get():
 *
 * Returns the buffer named buf_name to the caller, if buffer doesn't
 * exist, then NULL is returned.  buf_get() recogizes several special
 * buffer names:
 *
 *     !! - Most recent history 
 *     !. - Current working buffer
 *     !+ - Next available history
 *     !n - History #n
 *     !y - Named buffer y
 */
char* buf_get( buf_name )
	char   *buf_name ;
{
	char  *buf ;
	int    i ;

	/*-- Check parameters --*/
	if( buf_name == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return NULL ;
	}

	if( *buf_name == '!' ) {
		switch( *(buf_name + 1) ) {

			/*-- !. = Current working buffer --*/
			case '.' :
				return varbuf_getstr( g_sqlbuf ) ;

			/*-- !! = Most recent history --*/
			case '!' :
				if( history_find( g_history, HISTORY_HEAD, &buf ) == False )
					return NULL ;
				return buf ;

			/*-- !+ = Next available history --*/
			case '+' :
				sqsh_set_error( SQSH_E_INVAL, "Invalid to read from !+" ) ;
				return NULL ;

			/*-- !n = History #n, !yyy = Named buffer yyy --*/
			default :
				if( isdigit( (int)*(buf_name + 1) ) ) {
					i = atoi( buf_name + 1 ) ;
					if( history_find( g_history, i, &buf ) == False )
						return NULL ;
					return buf ;
				}

				env_get( g_buf, buf_name + 1, &buf ) ;
				return buf ;
		}
	}

	/*
	 * If the buffer name didn't start with a !, then we need to
	 * assume that it is just a named buffer.
	 */
 	env_get( g_buf, buf_name, &buf ) ;
 	return buf ;
}

/*
 * buf_can_put():
 *
 * Returns True if buf_name is a writtable buffer.
 */
int buf_can_put( buf_name )
	char  *buf_name ;
{
	/*-- Bad name for buffer --*/
	if( buf_name == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, "Invalid buffer name" ) ;
		return False ;
	}

	/*-- Can't write to history --*/
	if( *buf_name == '!' && isdigit( (int)*(buf_name+1) ) ) {
		sqsh_set_error( SQSH_E_BADPARAM, "Cannot write to history" ) ;
		return False ;
	}

	/*-- Dito --*/
	if( *buf_name == '!' && *(buf_name+1) == '!' ) {
		sqsh_set_error( SQSH_E_BADPARAM, "Cannot write to history" ) ;
		return False ;
	}

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

/*
 * buf_save():
 *
 * Write buffer buf_name to file save_name with write mode.
 */
int buf_save( buf_name, save_name, mode )
	char   *buf_name ;
	char   *save_name ;
	char   *mode ;
{
	FILE   *save_file ;
	char   *buf ;
	int     buf_len ;

	/*
	 * Attempt to retrieve the buffer.
	 */
	if( (buf = buf_get( buf_name )) == NULL )
		return False ;

	if( (save_file = fopen( save_name, mode )) == NULL ) {
		sqsh_set_error( errno, "%s: %s", save_name, strerror( errno ) ) ;
		return False ;
	}

	buf_len = strlen( buf ) ;
	if( buf_len > 0 ) {
		if( fwrite( buf, buf_len, 1, save_file ) != 1 ) {
			sqsh_set_error( errno, "%s: %s", save_name, strerror( errno ) ) ;
			fclose( save_file ) ;
			return False ;
		}
	}

	fclose( save_file ) ;
	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

/*
 * buf_load():
 *
 * Loads contents of file_name into buffer buf_name.
 */
int buf_load( buf_name, file_name, append )
	char   *buf_name;
	char   *file_name;
	int     append;
{
	FILE        *load_file;
	char         str[1024];
	varbuf_t    *load_buf;      /* Where we are building buffer */

	/*
	 * First, check to see if the buffer that we are to load into is
	 * writtable.
	 */
	if (buf_can_put( buf_name ) == False)
	{
		return False;
	}

	/*
	 * Now, attempt to open the file.
	 */
 	if ((load_file = fopen( file_name, "r" )) == NULL)
	{
 		sqsh_set_error( errno, "%s: %s", file_name, strerror( errno ) );
 		return False ;
	}

	/*
	 * Since we don't know how large the file is we need to create
	 * a temporary, growable, location in which to store it.
	 */
	if ((load_buf = varbuf_create( 1024 )) == NULL)
	{
		sqsh_set_error( sqsh_get_error(), "varbuf_create: %s",
		                sqsh_get_errstr() );
	 	fclose( load_file );
	 	return False;
	}

	/*
	 * Now, read the file, line-by-line, placing each line into
	 * load_buf.  If we fail, then clean-up and get out of here.
	 */
	while (fgets( str, sizeof(str), load_file ) != NULL)
	{
		if (varbuf_strcat( load_buf, str ) == -1)
		{
			varbuf_destroy( load_buf );
			fclose( load_file );
			sqsh_set_error( SQSH_E_NOMEM, NULL );
			return False;
		}
	}

	/*
	 * If fgets() returned NULL because of an error, then we should
	 * report it to the caller.
	 */
	if (ferror( load_file ))
	{
		varbuf_destroy( load_buf );
		sqsh_set_error( errno, NULL );
		fclose( load_file );
		return False;
	}

	/*-- Don't need this --*/
	fclose( load_file );

	/*
	 * Finally, place the contents of the buffer that we just built
	 * into the named buffer that the user requested.
	 */
	if (append != False)
	{
		if (buf_append( buf_name, varbuf_getstr(load_buf) ) == False)
		{
			varbuf_destroy( load_buf );
			return False;
		}
	}
	else
	{
		if (buf_put( buf_name, varbuf_getstr(load_buf) ) == False)
		{
			varbuf_destroy( load_buf );
			return False;
		}
	}
	varbuf_destroy( load_buf );

	sqsh_set_error( SQSH_E_NONE, NULL );
 	return True;
}

/*
 * buf_put():
 *
 * Saves buf into named buffer buf_name.  buf_put recognizes
 * the same special buffer names as buf_get().
 */
int buf_put( buf_name, buf )
	char   *buf_name;
	char   *buf;
{
#if defined(USE_READLINE)
	char     *str, *ch;
#endif

	/*-- Check parameters --*/
	if (buf_name == NULL)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	if (*buf_name == '!')
	{
		switch (*(buf_name + 1))
		{

			/*-- !. = Current working buffer --*/
			case '.' :
				if (varbuf_strcpy( g_sqlbuf, buf ) == -1)
				{
					return False;
				}

				/*
				 * If readline support has been compiled in, then we
				 * want to copy each line of the buffer that we copied
				 * into g_sqlbuf, into the readline history buffer.
				 */
#if defined(USE_READLINE)
				if (sqsh_stdin_isatty())
				{
					str = buf;
					while ((ch = strchr( str, '\n' )) != NULL)
					{
						if (str != ch)
						{
							*ch = '\0';
							add_history( str );
							*ch = '\n';
						}
						str = ch + 1;
					}

					if (str != ch && *str != '\0')
					{
						add_history( str );
					}
				}
#endif /* USE_READLINE */

				return True;

			/*-- !! = Most recent history --*/
			case '!' :
				sqsh_set_error( SQSH_E_INVAL, "Invalid to write to history" );
				return False;

			/*-- !+ = Next available history --*/
			case '+' :
				return history_append( g_history, buf );

			/*-- !n = History #n, !yyy = Named buffer yyy --*/
			default :
				if (isdigit( (int)*(buf_name + 1) ))
				{
					sqsh_set_error( SQSH_E_INVAL, "Invalid to write to history" );
					return False;
				}

				return env_set( g_buf, buf_name + 1, buf );
		}
	}

	return env_set( g_buf, buf_name, buf );
}

/*
 * buf_append():
 *
 * Appends buf into named buffer buf_name.  
 */
int buf_append( buf_name, buf )
	char   *buf_name;
	char   *buf;
{
	varbuf_t  *v;
	char      *old_buf;
	int        r;

	/*-- Check parameters --*/
	if (buf_name == NULL)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	if (*buf_name == '!')
	{
		switch (*(buf_name + 1))
		{

			/*-- !. = Current working buffer --*/
			case '.' :
				if (varbuf_strcat( g_sqlbuf, buf ) == -1)
				{
					return False;
				}
				return True;

			/*-- !! = Most recent history --*/
			case '!' :
				sqsh_set_error( SQSH_E_INVAL, "Invalid to write to history" );
				return False;

			/*-- !+ = Next available history --*/
			case '+' :
				return history_append( g_history, buf );

			/*-- !n = History #n, !yyy = Named buffer yyy --*/
			default :
				if (isdigit( (int)*(buf_name + 1) ))
				{
					sqsh_set_error( SQSH_E_INVAL, "Invalid to write to history" );
					return False;
				}

				buf_name = buf_name + 1;
		}
	}

	if (env_get( g_buf, buf_name, &old_buf ) == False)
	{
		return env_set( g_buf, buf_name, buf );
	}

	v = varbuf_create( 512 );

	if (v == NULL)
	{
		return False;
	}

	varbuf_strcpy( v, old_buf );
	varbuf_strcat( v, buf );

	r = env_set( g_buf, buf_name, varbuf_getstr(v) );

	varbuf_destroy( v );

	return r;
}

/*
 * buf_del()
 *
 * sqsh-2.1.6 - Deletes a buffer from history.
 *              If buffer doesn't exist, then False is returned.
 * sqsh-2.1.7 - Allow range deletes ( x-y ).
 */
int buf_del( parm )
    char *parm ;
{
    int   i ;
    int   j ;
    char  arg[12];
    char *p ;

    /*-- Check parameters --*/
    if ( parm == NULL || strlen (parm) > sizeof(arg) - 1)
    {
        sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
        return False ;
    }
    strcpy (arg, parm);


    if ( (p = strchr( arg, '-' )) != NULL) {
        *p = '\0';
	i  = atoi( arg );
	j  = atoi( p+1 );
	if ( i == 0 || j == 0 || i >= j ) {
            sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
            return False ;
        }
        history_range_del( g_history, i, j ) ;
        sqsh_set_error( SQSH_E_NONE, NULL ) ;
        return True ;
    }
    else if ( (i = atoi( arg )) > 0)
    {
        if ( history_del( g_history, i ) == True )
        {
            sqsh_set_error( SQSH_E_NONE, NULL ) ;
            return True ;
        }
        else
        {
            sqsh_set_error( SQSH_E_EXIST, NULL ) ;
            return False ;
        }
    }
    else
    {
        sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
        return False ;
    }
}

