/*
 * sqsh_cmd.c - Routines for manipulating named function pointers
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
#include "sqsh_cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_cmd.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes --*/
static cmd_t*   cmd_create    _ANSI_ARGS(( char*, cmd_f* )) ;
static void     cmd_destroy   _ANSI_ARGS(( cmd_t* )) ;
static int      cmd_cmp       _ANSI_ARGS(( cmd_t*, cmd_t* )) ;

cmdset_t* cmdset_create()
{
	cmdset_t  *cs ;

	/*-- Allocate a new command set --*/
	if( (cs = (cmdset_t*)malloc( sizeof(cmdset_t) )) == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return NULL ; 
	}

	/*-- Allocate a tree for our commands --*/
	if( (cs->cs_cmd = avl_create( (avl_cmp_t*)cmd_cmp,
	                              (avl_destroy_t*)cmd_destroy )) == NULL ) {
		free( cs ) ;
		return NULL ;
	}

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return cs ;
}

int cmdset_add( cs, cmd_name, cmd_ptr )
	cmdset_t *cs ;
	char     *cmd_name ;
	cmd_f    *cmd_ptr ;
{
	cmd_t    *c ;

	/*-- Validate arguments --*/
	if( cs == NULL || cmd_name == NULL || cmd_ptr == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*-- Create a new command structure --*/
	if( (c = cmd_create( cmd_name, cmd_ptr )) == NULL )
		return False ;

	/*-- And stick it in the tree --*/
	if( avl_insert( cs->cs_cmd, (void*)c ) == False )
		return False ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

cmd_t* cmdset_get( cs, cmd_name )
	cmdset_t  *cs ;
	char      *cmd_name ;
{
	cmd_t    *c, cmd_search ;

	if( cs == NULL || cmd_name == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return NULL ;
	}

	cmd_search.cmd_name = cmd_name ;
	c = (cmd_t*)avl_find( cs->cs_cmd, (void*)&cmd_search ) ;

	if( c == NULL ) {
		sqsh_set_error( SQSH_E_EXIST, NULL ) ;
		return NULL ;
	}

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return c ;
}

int cmdset_destroy( cs )
	cmdset_t *cs ;
{
	if( cs == NULL )
		return False ;

	if( cs->cs_cmd != NULL )
		avl_destroy( cs->cs_cmd ) ;
	free( cs ) ;

	return True ;
}

static cmd_t* cmd_create( cmd_name, cmd_ptr )
	char   *cmd_name ;
	cmd_f  *cmd_ptr ;
{
	cmd_t  *c ;

	if( (c = (cmd_t*)malloc(sizeof(cmd_t))) == NULL )  {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return NULL ;
	}
	if( (c->cmd_name = sqsh_strdup( cmd_name )) == NULL ) {
		free(c) ;
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return NULL ;
	}

	c->cmd_ptr   = cmd_ptr ;
	return c;
}

static int cmd_cmp( c1, c2 )
	cmd_t  *c1 ;
	cmd_t  *c2 ;
{
	return strcmp( c1->cmd_name, c2->cmd_name ) ;
}

static void cmd_destroy( c )
	cmd_t  *c ;
{
	if( c != NULL ) {
		if( c->cmd_name != NULL )
			free( c->cmd_name ) ;
		free( c ) ;
	}
}
