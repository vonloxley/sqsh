/*
 * var_misc.c - Generic variable validation functions
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
#include "sqsh_stdin.h"
#include "var.h"
#include "sqsh_global.h"
#include "sqsh_fd.h"
#include "sqsh_expand.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: var_misc.c,v 1.8 2014/03/12 14:40:43 mwesdorp Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

int var_get_interactive( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	if (sqsh_stdin_isatty())
	{
		*var_value = "1";
	}
	else
	{
		*var_value = "0";
	}

	return True;
}

int var_set_env( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	char   str[512];

	if (var_value == NULL || *var_value == NULL)
	{
		sprintf(str, "%s=", var_name );
		putenv( str );
		return True;
	}

	sprintf(str, "%s=%s", var_name, *var_value );
	putenv( str );

	DBG(sqsh_debug(DEBUG_ENV,"var_set_env: putenv(%s)\n", str);)

	return True;
}

int var_get_env( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	char   *c;

	c = getenv(var_name);

	if (c == NULL)
	{
		*var_value = "";
	}
	else
	{
		*var_value = c;
	}

	DBG(sqsh_debug(DEBUG_ENV,"var_set_env: getenv(%s) = %s\n", var_name, c);)

	return True;
}

/*
 * var_set_esc():
 *
 * Allow a variable to be set using escape sequences.
 */
int var_set_esc( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	char   *src, *dst ;

	src = *var_value ;
	dst = *var_value ;
	while( *src != '\0' ) {
		if( *src == '\\' ) {
			++src ;
			switch( *src ) {
				case 'n' :
					*dst++ = '\n' ;
					break ;
				case 'f' :
					*dst++ = '\f' ;
					break ;
				case 'r' :
					*dst++ = '\r' ;
					break ;
				case 't' :
					*dst++ = '\t' ;
					break ;
				case 'v' :
					*dst++ = '\v' ;
					break ;
				default :
					*dst++ = *src ;
			}
		} else {
			*dst++ = *src ;
		}

		++src ;
	}
	*dst = '\0' ;

	return True ;
}

/*
 * var_set_nullesc():
 *
 * Allow a variable to be set using escape sequences.
 */
int var_set_nullesc( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	char   *src, *dst ;

	/*
	 * Allow them to set the string to a null value.
	 */
	if( var_value == NULL || *var_value == NULL || strcasecmp(*var_value,"NULL") == 0 ) {
		*var_value = NULL ;
		return True ;
	}

	src = *var_value ;
	dst = *var_value ;
	while( *src != '\0' ) {
		if( *src == '\\' ) {
			++src ;
			switch( *src ) {
				case 'n' :
					*dst++ = '\n' ;
					break ;
				case 'f' :
					*dst++ = '\f' ;
					break ;
				case 'r' :
					*dst++ = '\r' ;
					break ;
				case 't' :
					*dst++ = '\t' ;
					break ;
				case 'v' :
					*dst++ = '\v' ;
					break ;
				default :
					*dst++ = *src ;
			}
		} else {
			*dst++ = *src ;
		}

		++src ;
	}
	*dst = '\0' ;

	return True ;
}

int var_set_readonly( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	sqsh_set_error( SQSH_E_INVAL, "readonly variable" ) ;
	return False ;
}

/*
 * var_set_notempty()
 *
 * Validation function for setting a variable with a non-NULL string.
 */
int var_set_notempty( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	char *cp ;

	/*
	 * Perform simple validations.
	 */
	if( var_value == NULL || *var_value == NULL || **var_value == '\0' ) {
		sqsh_set_error( SQSH_E_INVAL, "Invalid value" ) ;
		return False ;
	}

	/*-- Skip any whitespace --*/
	for( cp = *var_value; *cp != '\0' && isspace((int)*cp); ++cp ) ;

	if( *cp == '\0' ) {
		sqsh_set_error( SQSH_E_INVAL, "Invalid value" ) ;
		return False ;
	}

	return True ;
}

/*
 * var_set_bool()
 *
 * Validation function for setting boolean (1/0, True/False, Yes/No)
 * variables.  Returns True if validation succeed, False otherwise.
 */
int var_set_bool( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	/*
	 * Perform simple validations.
	 */
	if( var_value == NULL || *var_value == NULL || **var_value == '\0' ) {
		sqsh_set_error( SQSH_E_INVAL, "Invalid boolean value" ) ;
		return False ;
	}

	/*-- Base case --*/
	if( strcmp( *var_value, "1" ) == 0 ||
		 strcmp( *var_value, "0" ) == 0 )
		return True ;

	/*-- Map True or Yes to 1 --*/
	if( strcasecmp( *var_value, "True" ) == 0 ||
		 strcasecmp( *var_value, "Yes" ) == 0 ||
		 strcasecmp( *var_value, "On" ) == 0 ) {
		*var_value = "1" ;
		return True ;
	}

	/*-- Map False or No to 0 --*/
	if( strcasecmp( *var_value, "False" ) == 0 ||
		 strcasecmp( *var_value, "No" ) == 0 ||
		 strcasecmp( *var_value, "Off" ) == 0 ) {
		*var_value = "0" ;
		return True ;
	}

	sqsh_set_error( SQSH_E_INVAL, "Invalid boolean value" ) ;
	return False ;
}

int var_set_nullint( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	char *cptr ;

	if( var_value == NULL || *var_value == NULL || strcasecmp(*var_value, "NULL") == 0 ) {
		*var_value = NULL ;
		return True;
	}

	/*-- Skip whitespace --*/
	for( cptr = *var_value; *cptr != '\0' && isspace( (int)*cptr ); ++cptr ) ;

	/*-- Check the digits --*/
	for( ; *cptr != '\0' && isdigit( (int)*cptr );  ++cptr ) ;

	/*-- Skip whitespace --*/
	for( ; *cptr != '\0' && isspace( (int)*cptr ); ++cptr ) ;

	if( *cptr != '\0' ) {
		sqsh_set_error( SQSH_E_INVAL, "Invalid integer expression" ) ;
		return False ;
	}

	return True ;
}

int var_set_nullstr( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	if( var_value == NULL || *var_value == NULL || strcasecmp( *var_value, "NULL" ) == 0 )
		*var_value = NULL ;

	return True ;
}

/*
 * var_set_int():
 *
 * Validates that var_value contains an integer expression.
 */
int var_set_int( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	/*-- Can't set it a NULL value --*/
	if( var_value == NULL || *var_value == NULL || strcasecmp(*var_value, "NULL") == 0) {
		sqsh_set_error( SQSH_E_INVAL, "Invalid integer expression" ) ;
		return False ;
	}

	return var_set_nullint( env, var_name, var_value ) ;
}

/*
 * var_set_add():
 *
 * If var_value is NULL, var_name is set to 0, otherwise var_value
 * is added to the currently value of var_name.
 */
int var_set_add( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	static char number[20] ;
	char   *value ;
	char   *cur_value ;
	char   *n ;

	/*-- Can't set it a NULL value --*/
	if( var_value == NULL || *var_value == NULL || **var_value == '\0' ) {
		*var_value = "0" ;
		return True ;
	}

	/*-- Save what was passed in --*/
	value = *var_value ;

	/*-- Skip whitespace --*/
	for( value = *var_value; *value != '\0' && isspace( (int)*value ); ++value );

	if( !(isdigit((int)*value) || *value == '=' || *value == '+' ||
		*value == '-') ){
		sqsh_set_error( SQSH_E_INVAL, "Invalid value" ) ;
		return False ;
	}

	if( !isdigit((int)*value) )
		n = value + 1 ;
	else
		n = value ;

	/*-- Check the digits --*/
	for( ; *n != '\0' && isdigit( (int)*n );  ++n ) ;

	/*-- Skip whitespace --*/
	for( ; *n != '\0' && isspace( (int)*n ); ++n ) ;

	if( *n != '\0' ) {
		sqsh_set_error( SQSH_E_INVAL, "Invalid value" ) ;
		return False ;
	}

	if( *value == '=' ) {
		strcpy( number, value+1 ) ;
		*var_value = number ;
		return True ;
	}

	/*-- Get the current value --*/
	env_get( env, var_name, &cur_value ) ;

	if( *value == '-' )
		sprintf( number, "%d", atoi(cur_value) - atoi(value+1) ) ;
	else if( *value == '+' )
		sprintf( number, "%d", atoi(cur_value) + atoi(value+1) ) ;
	else
		sprintf( number, "%d", atoi(cur_value) + atoi(value) ) ;

	*var_value = number ;
	return True ;
}

/*
 * var_set_path_rw():
 *
 * Validates that var_value contains a valid path.
 */
int var_set_path_rw( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	/*-- Can't set it a NULL value --*/
	if( var_value == NULL || *var_value == NULL ) {
		sqsh_set_error( SQSH_E_INVAL, "Invalid path name" ) ;
		return False ;
	}

	/*-- Check to see if we can read and write to the path --*/
	if( access( *var_value, W_OK ) == -1 ) {
		sqsh_set_error( SQSH_E_INVAL, "Illegal path: %s", strerror( errno ) ) ;
		return False ;
	}

	return True ;
}

int var_set_path_r( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	/*-- Can't set it a NULL value --*/
	if (var_value == NULL || *var_value == NULL)
	{
		sqsh_set_error( SQSH_E_INVAL, "Invalid path name" );
		return False;
	}

	/*-- Check for read access --*/
	if (access( *var_value, R_OK ) == -1)
	{
		sqsh_set_error( SQSH_E_INVAL,
			"Illegal path: %s", strerror( errno ) );
		return False;
	}

	return True;
}

/*
 * var_set_lconv() - sqsh-2.3
 *
 * Validation function for setting variable localeconv (1/0, True/False, Yes/No)
 * When set to true, initialize variable g_lconv to localeconv(), NULL otherwise.
 * Returns True if validation succeed, False otherwise.
 */
int var_set_lconv( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	/*
	 * Perform simple validations.
	 */
	if( var_value == NULL || *var_value == NULL || **var_value == '\0' ) {
		sqsh_set_error( SQSH_E_INVAL, "Invalid boolean value" ) ;
		return False ;
	}

	/*-- Map True or Yes to 1 --*/
	if( strcasecmp( *var_value, "True" ) == 0 ||
	    strcasecmp( *var_value, "Yes" )  == 0 ||
	    strcasecmp( *var_value, "On" )   == 0 ) {
		*var_value = "1" ;
	}
	/*-- Map False or No to 0 --*/
	else if( strcasecmp( *var_value, "False" ) == 0 ||
		 strcasecmp( *var_value, "No" )    == 0 ||
		 strcasecmp( *var_value, "Off" )   == 0 ) {
		*var_value = "0" ;
	}

	if( strcmp( *var_value, "1" ) == 0 )
	{
#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE) && defined(HAVE_LOCALECONV)
		setlocale ( LC_ALL, "" );
		g_lconv = localeconv();
		return True;
#else
		g_lconv = NULL;
		*var_value = "0" ;
		sqsh_set_error( SQSH_E_INVAL, "setlocale() and/or localeconv() not available" ) ;
		return False;
#endif
	}
	else if( strcmp( *var_value, "0" ) == 0 )
	{
#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)
		setlocale ( LC_ALL, "C" );
#endif
		g_lconv = NULL;
		return True;
	}

	sqsh_set_error( SQSH_E_INVAL, "Invalid boolean value" ) ;
	return False ;
}

/*
 * sqsh-2.5 - New feature p2f (Print to File)
 *            Assume var_name == "p2fname"
 */
int var_set_p2fname( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	varbuf_t  *exp_buf = NULL;
	FILE      *nfp     = NULL;
	char      *exp_fn;


	if ( var_name == NULL || strcmp(var_name, "p2fname") != 0 )
       	{
		sqsh_set_error( SQSH_E_INVAL, "var_set_p2fname: Unexpected variable name %s", var_name == NULL ? "NULL" : var_name ) ;
		return False;
	}

	if ( var_value == NULL || *var_value == NULL || strcasecmp( *var_value, "NULL" ) == 0 )
       	{
		*var_value = NULL ;
	}
	else {
		if ((exp_buf = varbuf_create( 512 )) == NULL)
		{
			fprintf( stderr, "sqsh: %s\n", sqsh_get_errstr() );
			sqsh_set_error( SQSH_E_INVAL, "Unable to expand filename %s", *var_value ) ;
			*var_value = NULL ;
			return False;
		}

		if (sqsh_expand( *var_value, exp_buf, 0 ) != False)
		{
			exp_fn = varbuf_getstr( exp_buf );
		}
		else
		{
			sqsh_set_error( SQSH_E_INVAL, "Unable to expand filename %s", *var_value ) ;
			*var_value = NULL ;
			varbuf_destroy( exp_buf );
			return False;
		}

		if( (nfp = fopen( exp_fn, "a" )) == NULL)
	       	{
			sqsh_set_error( SQSH_E_INVAL, "Unable to open file %s", exp_fn ) ;
			*var_value = NULL ;
			varbuf_destroy( exp_buf );
			return False;
		}
		varbuf_destroy( exp_buf );
	}

	/*
	 * If we come here, the fopen of the new file was successfull, or the p2fname variable is
	 * set to NULL. Either way, we can close the original file pointer if it is open
	 * and assign g_p2f_fp the new file pointer value (that still might be NULL).
	 */
	if (g_p2f_fp != NULL)
       	{
		fclose (g_p2f_fp);
	}
	g_p2f_fp = nfp;

	return True ;
}

