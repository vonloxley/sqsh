/*
 * sqsh_args.c - Routines & structures for manipulating argc/argv pairs
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
#include "sqsh_args.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_args.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

args_t* args_create( grow_size )
	int grow_size ;
{
	args_t   *a ;
	int       i ;

	/*-- Always check parameters --*/
	if( grow_size < 1 ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return NULL ;
	}

	/*-- Allocate data structure --*/
	if( (a = (args_t*)malloc(sizeof(args_t))) == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return NULL ;
	}

	/*-- Allocate array of arguments --*/
	if( (a->a_argv = (char**)malloc(grow_size*sizeof(char*))) == NULL ) {
		free( a ) ;
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
	}

	/*-- Clear out array --*/
	for( i = 0; i < grow_size; i++ )
		a->a_argv[i] = NULL ;

	a->a_argc     = 0 ;
	a->a_len      = grow_size ;
	a->a_growsize = grow_size ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return a ;
}

int args_add( a, arg )
	args_t   *a ;
	char     *arg ;
{
	char **new_argv ;

	if( a == NULL || arg == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*
	 * Make sure that there is room for the current argument, plus
	 * the trailing NULL terminator.
	 */
	if( a->a_argc == (a->a_len-1) ) {
		new_argv = (char**)realloc(a->a_argv, (a->a_len + a->a_growsize) *
		                                      sizeof(char*)) ;
		if( new_argv == NULL ) {
			sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
			return False ;
		}
		a->a_len += a->a_growsize ;
		a->a_argv = new_argv ;
	}

	if( (a->a_argv[a->a_argc] = sqsh_strdup( arg )) == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return False ;
	}

	++a->a_argc ;

	/*
	 * Always keep a NULL entry on the end.
	 */
	a->a_argv[a->a_argc] = NULL;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

int args_argc( a )
	args_t   *a ;
{
	if( a == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return a->a_argc ;
}

char** args_argv( a )
	args_t   *a ;
{
	if( a == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return NULL ;
	}

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return a->a_argv ;
}

void args_destroy( a )
	args_t  *a ;
{
	int i ;
	if( a != NULL ) {
		if( a->a_argv != NULL ) {
			for( i = 0; i < a->a_argc; i++ ) {
				if( a->a_argv[i] != NULL )
					free( a->a_argv[i] ) ;
			}
			free( a->a_argv ) ;
		}
		free( a ) ;
	}
}
