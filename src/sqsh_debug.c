/*
 * sqsh_debug.c - Routines for displaying debugging messages
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
#include "sqsh_debug.h"

#if defined(__ansi__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_debug.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif

static int   sg_debug_level = 0 ;    /* No debugging */

#if defined(DEBUG)
static int   sg_debug_fd    = -1 ;   /* File descriptor to print to */
static FILE *sg_debug_file  = NULL ; /* File to print to */
#endif

int sqsh_debug_level( debug_level )
	int debug_level ;
{
	sg_debug_level = debug_level ;
	return True ;
}

#if defined(__ansi__)
void sqsh_debug( int debug_mask, char *fmt, ... )
#else
void sqsh_debug( debug_mask, fmt, va_alist )
	int    debug_mask ;
	char  *fmt ;
	va_dcl
#endif
{
#if defined(DEBUG)
	va_list  ap ;

	if( debug_mask != DEBUG_ALL && !(debug_mask & sg_debug_level) )
		return ;

	if( sg_debug_file == NULL ) {
		if( (sg_debug_fd = dup(fileno(stderr))) == -1 )
			return ;
		if( (sg_debug_file = fdopen( sg_debug_fd, "w" )) == NULL ) {
			close( sg_debug_fd ) ;
			return ;
		}
		setbuf( sg_debug_file, NULL ) ;
	}

#if defined(__ansi__)
	va_start(ap, fmt) ;
#else
	va_start(ap) ;
#endif

	vfprintf( sg_debug_file, fmt, ap ) ;
	va_end( ap ) ;
	fflush( sg_debug_file ) ;

#endif /* DEBUG */

	return  ;
}

