/*
 * sqsh_strchr.c - A "smart" version of strchr
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
#include "sqsh_strchr.h"
#include "sqsh_error.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_strchr.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Different quotes --*/
#define QUOTE_NONE           1
#define QUOTE_DOUBLE         2
#define QUOTE_SINGLE         3

/*
 * sqsh_strchr():
 *
 * Simple wrapper around strnchr().
 */
char* sqsh_strchr( str, ch )
	char   *str ;
	int     ch ;
{
	return sqsh_strnchr( str, ch, -1 ) ;
}

/*
 * sqsh_strnchr():
 *
 * Acts as a smart version of strnchr.  sqsh_strchr() returns the first
 * ocurrance of the character ch from str, that is not contained within
 * quotes, either single or double.  If n is zero or a positive then 
 * sqsh_strnchr() stops its search at the nth character of str, otherwise
 * it continues until ch is found or an end-of-string is encountered.
 */
char* sqsh_strnchr( str, ch, n )
	char    *str ;
	int      ch ;
	int      n ;
{
	int     cmd_quote = QUOTE_NONE ;
	char   *str_end ;

	/*
	 * If the user passed a positive n, then set str_end to be the
	 * address of the end of the string.  Otherwise it is NULL.
	 */
	if( n >= 0 )
		str_end = str + n ;
	else
		str_end = NULL ;

	/*
	 * Traverse through str until we either hit the end of the string
	 * or we find ch and it is not in any quotes.
	 */
	while( str != str_end && *str != '\0' &&
	       !(*str == ch && cmd_quote == QUOTE_NONE) ) {
		
		/*-- Detect single quotes --*/
		if( *str == '\'' && cmd_quote != QUOTE_DOUBLE )
			cmd_quote = (cmd_quote == QUOTE_NONE) ? QUOTE_SINGLE : QUOTE_NONE ;

		/*-- Detect double quotes --*/
		else if( *str == '\"' && cmd_quote != QUOTE_SINGLE )
			cmd_quote = (cmd_quote == QUOTE_NONE) ? QUOTE_DOUBLE : QUOTE_NONE ;

		/*-- Look for escape --*/
		else if( *str == '\\' && cmd_quote != QUOTE_SINGLE ) {
			if( (str+1) != str_end && *(str + 1) == '\\' )
				str += 2 ;
		}
		
		++str ;
	}

	if( cmd_quote != QUOTE_NONE ) {
		sqsh_set_error( SQSH_E_BADQUOTE, "Unbounded %s quote",
		                cmd_quote == QUOTE_DOUBLE ? "double" : "single" ) ;
		return NULL ;
	}

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	if( str_end == str || *str != ch ) 
		return NULL ;

	return str ;
}
