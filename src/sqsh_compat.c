/*
 * sqsh_compat.c - Libc routines that may not exist on all machines
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
#include <termios.h>
#include <setjmp.h>
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_sig.h"
#include "sqsh_compat.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_compat.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Local Globals --*/
static JMP_BUF sg_getinput_jmp;

/*-- Prototypes --*/
static void sqsh_getinput_handler  _ANSI_ARGS(( int, void * ));
static int  sqsh_setenv_cmp        _ANSI_ARGS(( char*, char* ));

#if !(defined(HAVE_STRCASECMP))
#if defined(__ansi__)
int strcasecmp( const char *s1, const char *s2 )
#else
int strcasecmp( s1, s2 )
	char *s1 ;
	char *s2 ;
#endif /* !__ansi__ */
{
	int r ;

	for( ; s1 != s2 && (r = (tolower(s1) - tolower(s2))) == 0; ++s1, ++s2 ) ;
	return r ;
}
#endif /* !HAVE_STRCASECMP */

/*
 * sqsh_setenv():
 *
 * Handy wrapper around the setenv() function.  This function
 * will automatically use putenv() if setenv() is not available.
 */
int sqsh_setenv( var_name, var_value )
	char  *var_name;
	char  *var_value;
{
	static int    setenv_called = False;
	extern char **environ;
	char        **new_environ;
	char         *buf;
	int           buf_size;
	int           i;

	/*
	 * The first time that sqsh_setenv is called, we must make 
	 * a copy of the current environment and replace it with
	 * our new copy.
	 */
	if (setenv_called == False)
	{
		setenv_called = True;

		for (i = 0; environ[i] != NULL; i++);
		new_environ = (char**)malloc(sizeof(char*) * (i+1));

		if (new_environ == NULL)
		{
			return -1;
		}

		for (i = 0; environ[i] != NULL; i++)
		{
			new_environ[i] = sqsh_strdup( environ[i] );

			if (new_environ == NULL)
			{
				for (--i; i >= 0; i++)
				{
					free( new_environ[i] );
				}
				free( new_environ );
				setenv_called = False;
				return -1;
			}
		}
		new_environ[i] = NULL;
		environ = new_environ;
	}

	/*
	 * Create our environment string of the format 'var_name=var_value'
	 * just like it says in the book.
	 */
	buf_size = strlen(var_name) + strlen(var_value) + 2;
	buf = (char*)malloc(buf_size * sizeof(char));

	if (buf == NULL)
	{
		return -1;
	}
	sprintf( buf, "%s=%s", var_name, var_value );

	/*
	 * Attempt to track down an existing entry in the environtment
	 * for this variable. 
	 */
	for (i = 0; environ[i] != NULL && 
			sqsh_setenv_cmp( environ[i], var_name ) != 0; i++);
	
	/*
	 * If we have an existing entry in the environment, then we
	 * can simply replace it with our new one.
	 */
	if (environ[i] != NULL)
	{
		free( environ[i] );
		environ[i] = buf;
		return 0;
	}

	/*
	 * Otherwise we must grow the environment from where it is
	 * right now.
	 */
	new_environ = (char**)realloc(environ, (i + 2) * sizeof(char*));

	if (new_environ == NULL)
	{
		return -1;
	}
	environ = new_environ;

	environ[i]   = buf;
	environ[i+1] = NULL;

	return 0;
}

static int sqsh_setenv_cmp( env, var_name )
	char *env;
	char *var_name;
{
	while (*env == *var_name &&
	       *env != '\0' && *env != '=' &&
	       *var_name != '\0')
	{
		++env;
		++var_name;
	}

	if (*env == '=' && *var_name == '\0')
	{
		return 0;
	}

	return -1;
}
			
/*
 * sqsh_getc():
 *
 * A handy wrapper around the getc() function that deals with
 * signals.
 */
int sqsh_getc( file )
	FILE *file;
{
	int    ch;

	do {
		errno = 0;
		ch    = getc( file );
	} while (ch == EOF && errno == EINTR);

	return ch;
}

/*
 * sqsh_strdup():
 *
 * I got so sick of having to deal with how many systems either don't
 * define strdup() or don't have a proper prototype that I threw it
 * out all together and rolled my own.
 */
char* sqsh_strdup( s )
	char *s ;
{
	char *new_s ;
	if( (new_s = (char*)malloc(sizeof(char) * (strlen(s)+1))) == NULL )
		return NULL ;
	strcpy( new_s, s ) ;
	return new_s;
}

char* sqsh_strndup( s, n )
	char *s ;
	int   n;
{
	char *new_s ;
	int   i;

	if ((new_s = (char*)malloc(sizeof(char) * n+1)) == NULL)
	{
		return NULL ;
	}

	for (i = 0; s[i] != '\0' && i < n; i++)
	{
		new_s[i] = s[i];
	}
	new_s[i] = '\0';

	return new_s;
}

#if !(defined(HAVE_STRERROR))
#if defined(__ansi__)
const char* strerror( int error )
#else
const char* strerror( error )
	int error ;
#endif /* !__ansi__ */
{
	extern const char *sys_errlist[] ;
	extern const int   sys_nerr ;

	if( error >= 0 && (unsigned int) error < sys_nerr )
		return sys_errlist[error] ;

	return "Unknown error code" ;
}
#endif /* !HAVE_STRERROR */

/*
 * sqsh_getinput():
 *
 * Read input from controlling terminal (not just stdin).  If the
 * echo flag is set to 0, then this function behaves like getpass().
 * -1 is returned upon error, -2 indicates that it was interrupted,
 * otherwise the length of input is returned.
 */
int sqsh_getinput( prompt, buf, len, echo )
	char *prompt;
	char *buf;
	int   len;
	int   echo;
{
	struct  termios    term,  termsave ;
	FILE   *read_fp;
	FILE   *write_fp;
	int     interrupted;
	char   *tty_name;
	int     ch;
	int     i;

	/*
	 * Grab a handle on the controlling terminal (tty), so we
	 * can be assured that we aren't reading from stdin which
	 * may be redirected from a file or something.
	 */
	tty_name = ctermid(NULL) ;

	if (tty_name == NULL)
	{
		sqsh_set_error( SQSH_E_EXIST, 
			"sqsh_getinput: Unable to determine controlling tty" );
		return -1;
	}

	/*
	 * This is stupid.  On some systems (Linux, in particular)
	 * there seems to be a bug with turning off echoing on file
	 * descriptors open as read/write.  To get around this bug
	 * need to open a seperate descriptor for both reading and
	 * writing.
	 */
	if ((read_fp = fopen( tty_name, "r" )) == NULL)
	{
		sqsh_set_error( errno, "sqsh_getinput: Error opening %s for read: %s\n",
		                tty_name, strerror(errno) );
		return -1;
	}

	if ((write_fp = fopen( tty_name, "w" )) == NULL)
	{
		sqsh_set_error( errno, "sqsh_getinput: Error opening %s for write: %s\n",
		                tty_name, strerror(errno) );
		fclose( read_fp );
		return -1;
	}

	/*
	 * Turn off buffering for both file handles.
	 */
	setbuf( read_fp, NULL ) ;
	setbuf( write_fp, NULL ) ;

	fputs( prompt, write_fp );
	fflush( write_fp );

	if (SETJMP( sg_getinput_jmp ) != 0)
	{
		interrupted = True;
		i = 0;
		goto cleanup;
	}
	else
	{
		/*
		 * Since we are going to install signal handlers, we don't want
		 * want to affect any handlers our parent has installed, so
		 * set up our own context.
		 */
		sig_save();

		interrupted = False;
		i = 0;
	}

	sig_install( SIGINT, sqsh_getinput_handler, (void*)NULL, 0 );
	sig_install( SIGPIPE, sqsh_getinput_handler, (void*)NULL, 0 );
	sig_install( SIGTSTP, sqsh_getinput_handler, (void*)NULL, 0 );

	/*
	 * Grab the current terminal settings, save them away in termsave,
	 * and turn off all echoing for the read handle only.
	 */
	tcgetattr( fileno(read_fp), &termsave );
	term = termsave;
	term.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	tcsetattr( fileno(read_fp), TCSAFLUSH, &term );

	/*
	 * Now, read in a string from the user, placing the output in
	 * input, and reading at most lencharacters.
	 */
	while ((ch = fgetc( read_fp )) != EOF && ch != '\n')
	{
		if (i < (len - 1))
		{
			if (echo != 0)
			{
				fputc( ch, write_fp );
			}
			buf[i++] = ch;
		}
	}
	if (buf != NULL && len > 1)
	{
		buf[i] = '\0';
	}

cleanup:

	/*
	 * Restore the terminal settings back to where they were
	 * when we began.
	 */
	tcsetattr( fileno(read_fp), TCSAFLUSH, &termsave );
	fclose(read_fp);

	/*
	 * The only thing we needed this file handle for was to print
	 * out the friggin' new-line character.  We *should* be able
	 * to do this with only one file handle, but some systems can't
	 * deal with it.
	 */
	fputc( '\n', write_fp );
	fclose( write_fp );

	/*
	 * Restore the original signal handlers back, so that the
	 * rest of the program will behave as expected.
	 */
	sig_restore();

	if (interrupted)
	{
		return -2;
	}

	return i;
}

/*
 * sqsh_getinput_handler():
 * 
 * Signal handler for sqsh_getinput().
 */
static void sqsh_getinput_handler( sig, user_data )
	int    sig;
	void  *user_data;
{
	LONGJMP( sg_getinput_jmp, 1 );
}
