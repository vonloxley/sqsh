/*
 * sqsh_func.c - Routines for manipulating user defined sqsh functions
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
#include "sqsh_avl.h"
#include "sqsh_func.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_func.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes --*/
static func_t*  func_create    _ANSI_ARGS(( char*, char* ));
static void     func_destroy   _ANSI_ARGS(( func_t* ));
static int      func_cmp       _ANSI_ARGS(( func_t*, func_t* ));

funcset_t* funcset_create()
{
	funcset_t  *fs;

	/*-- Allocate a new command set --*/
	if ((fs = (funcset_t*)malloc( sizeof(funcset_t) )) == NULL)
	{
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return NULL ; 
	}

	/*-- Allocate a tree for our commands --*/
	if ((fs->fs_funcs = 
		avl_create((avl_cmp_t*)func_cmp, (avl_destroy_t*)func_destroy)) == NULL)
	{
		free( fs );
		return NULL;
	}

	sqsh_set_error( SQSH_E_NONE, NULL );
	return fs;
}

int funcset_add( fs, func_name, func_body )
	funcset_t *fs;
	char      *func_name;
	char      *func_body;
{
	func_t    *f;

	/*-- Validate arguments --*/
	if (fs == NULL || func_name == NULL || func_body == NULL )
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	/*-- Create a new command structure --*/
	if ((f = func_create( func_name, func_body )) == NULL)
		return False;

	/*-- And stick it in the tree --*/
	if (avl_insert( fs->fs_funcs, (void*)f ) == False)
		return False;

	sqsh_set_error( SQSH_E_NONE, NULL );
	return True;
}

func_t* funcset_get( fs, func_name )
	funcset_t  *fs;
	char       *func_name;
{
	func_t    *f, func_search;

	if (fs == NULL || func_name == NULL)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return NULL;
	}

	func_search.func_name = func_name;
	f = (func_t*)avl_find( fs->fs_funcs, (void*)&func_search );

	if (f == NULL)
	{
		sqsh_set_error( SQSH_E_EXIST, NULL );
		return NULL;
	}

	sqsh_set_error( SQSH_E_NONE, NULL );
	return f;
}

int funcset_destroy( fs )
	funcset_t *fs;
{
	if (fs == NULL)
		return False;

	if (fs->fs_funcs != NULL)
		avl_destroy( fs->fs_funcs );
	free( fs );

	return True;
}

static func_t* func_create( func_name, func_body )
	char    *func_name;
	char    *func_body;
{
	func_t  *f;

	if ((f = (func_t*)malloc(sizeof(func_t))) == NULL)
	{
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return NULL;
	}

	if ((f->func_name = sqsh_strdup(func_name)) == NULL)
	{
		free(f);
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return NULL;
	}

	if ((f->func_body = sqsh_strdup(func_body)) == NULL)
	{
		free(f->func_name);
		free(f);
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return NULL;
	}

	return f;
}

static int func_cmp( f1, f2 )
	func_t  *f1;
	func_t  *f2;
{
	return strcmp( f1->func_name, f2->func_name );
}

static void func_destroy( f )
	func_t  *f;
{
	if (f != NULL)
	{
		if (f->func_name != NULL)
			free( f->func_name );
		free( f );
	}
}
