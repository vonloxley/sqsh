/*
 * sqsh_error.c - Functions for setting/retrieving error state
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
#include "sqsh_config.h"
#include "sqsh_error.h"

#ifdef __ansi__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_error.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */


/*
 * The following is the maxiumum length of a given error.
 */
#define MAX_ERRLEN     512

/*-- Static globals --*/
static int  sg_sqsh_errno = SQSH_E_NONE ;  /* Last error */
static char sg_sqsh_errstr[MAX_ERRLEN+1] ;

/*
 * The following are the default messages for a given error condition.
 * These messages may be overridded via sqsh_set_error() by supplying
 * a non-NULL error message.
 */
static char *sg_sqsh_errtable[] = {
	"No error",                                   /* SQSH_E_NONE */
	"Memory allocation failure",                  /* SQSH_E_NOMEM */
	"Invalid parameter to function",              /* SQSH_E_BADPARAM */
	"Object does not exist",                      /* SQSH_E_EXIST */
	"Validation failure",                         /* SQSH_E_INVAL */
	"Bad variable syntax",                        /* SQSH_E_BADVAR */
	"Unbound quote",                              /* SQSH_E_BADQUOTE */
	"Syntax error",                               /* SQSH_E_SYNTAX */
	"Out of range access",                        /* SQSH_E_RANGE */
	"Invalid state for operation",                /* SQSH_E_BADSTATE */
} ;


int sqsh_get_error()
{
	return sg_sqsh_errno ;
}

char* sqsh_get_errstr()
{
	if( sg_sqsh_errno == SQSH_E_NONE || sg_sqsh_errstr[0] == '\0' ) {
		if( sg_sqsh_errno > 0 )
			return strerror( sg_sqsh_errno ) ;
		return sg_sqsh_errtable[ -(sg_sqsh_errno) ] ;
	}

	return sg_sqsh_errstr ;
}

#if defined(__ansi__)
void sqsh_set_error( int err, char *fmt, ... )
#else
void sqsh_set_error( err, fmt, va_alist )
	int     err ;
	char   *fmt ;
	va_dcl
#endif
{
	va_list ap ;
	char    tmp_errstr[MAX_ERRLEN+1] ;

	sg_sqsh_errno = err ;

	if( sg_sqsh_errno != SQSH_E_NONE ) {
		if( fmt != NULL ) {
			va_start( ap, fmt ) ;
			vsprintf( tmp_errstr, fmt, ap ) ;
			va_end( ap ) ;

			strcpy( sg_sqsh_errstr, tmp_errstr ) ;
		} else {
			sg_sqsh_errstr[0] = '\0' ;
		}
	}
}
