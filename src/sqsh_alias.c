/*
 * sqsh_alias.c - Routines for manipulating command aliases
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
#include "sqsh_error.h"
#include "sqsh_debug.h"
#include "sqsh_alias.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_alias.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes --*/
static aliasnode_t* aliasnode_create  _ANSI_ARGS(( char*, char* )) ;
static int          aliasnode_replace _ANSI_ARGS(( aliasnode_t*, char* )) ;
static int          aliasnode_cmp     _ANSI_ARGS(( void*, void* )) ;
static void         aliasnode_destroy _ANSI_ARGS(( void* )) ;

/*
 * Maximum length of an alias name.
 */
#define MAX_ALIAS    30

/*
 * alias_create():
 *
 * Creates a new alias structure, empty of any actual alias entries.
 */
alias_t* alias_create()
{
	alias_t   *new_alias ;

	new_alias = (alias_t*)malloc(sizeof(alias_t)) ;
	if( new_alias == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return NULL ;
	}

	new_alias->alias_entries = avl_create( (avl_cmp_t*)aliasnode_cmp, 
		(avl_destroy_t*)aliasnode_destroy ) ;

	if( new_alias->alias_entries == NULL ) {
		free( new_alias ) ;
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return NULL ;
	}

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return new_alias ;
}

/*
 * alias_add():
 *
 * Adds a new alias for alias_name with a body of alias_body to the
 * alias a, returning True upon success and False upon failure.
 */
int alias_add( a, alias_name, alias_body )
	alias_t   *a ;
	char      *alias_name ;
	char      *alias_body ;
{
	aliasnode_t  *new_node ;
	aliasnode_t  search_node ;

	/*-- Check parameters --*/
	if( a == NULL || alias_name == NULL || alias_body == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*-- Check to see if we already have an entry --*/
	search_node.an_name = alias_name ;
	new_node = (aliasnode_t*)avl_find( a->alias_entries, (void*)&search_node ) ;

	/*
	 * If this alias has already been seen in the past, then we simply
	 * replace the body of the alias with the new body.
	 */
	if( new_node != NULL ) {
		if( aliasnode_replace( new_node, alias_body ) == False )
			return False ;

		sqsh_set_error( SQSH_E_NONE, NULL ) ;
		return True ;
	}

	/*-- Create a new node --*/
	new_node = aliasnode_create( alias_name, alias_body ) ;
	if( new_node == NULL )
		return False ;

	/*-- Add it to the AVL Tree --*/
	if( avl_insert( a->alias_entries, (void*)new_node ) == False )
		return False ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

/*
 * alias_remove():
 *
 * Removes alias for alias name from a, returning True if the delete
 * succeeded or there is no alias known by alias_name, False if an
 * error was encountered.
 */
int alias_remove( a, alias_name )
	alias_t    *a ;
	char       *alias_name ;
{
	aliasnode_t  search_node ;

	/*-- Check parameters --*/
	if( a == NULL || alias_name == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	search_node.an_name = alias_name ;
	avl_delete( a->alias_entries, (void*)&search_node ) ;

	return True ;
}

/* 
 * alias_expand():
 *
 * If cmd_line begins with a word to be aliased, then the alias
 * substitution is performed, leaving the results in exp_buf and
 * 1 is returned.  If cmd_line does not contain and alias 0 is
 * returned, and -1 is returned if an error condition is encountered.
 */
int alias_expand( a, cmd_line, exp_buf )
	alias_t    *a ;
	char       *cmd_line ;
	varbuf_t   *exp_buf ;
{
	aliasnode_t  *node ;
	aliasnode_t   search_node ;
	char         *word_start ;
	char         *word_end ;
	char         *chunk_start ;
	char         *chunk_end ;
	char         *args_start ;
	char         *args_end ;
	char         *cptr ;
	char          alias_name[MAX_ALIAS+1] ;
	int           len ;
	int           did_subst = False ;
	int           r ;

	/*-- Check parameters --*/
	if( a == NULL || cmd_line == NULL || exp_buf == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	/*-- Skip leading white-space --*/
	for( word_start = cmd_line; *word_start != '\0' && 
		isspace((int)*word_start); ++word_start ) ;

	/*-- Find the end of the word --*/
	for( word_end = word_start; *word_end != '\0' && 
		!(isspace((int)*word_end)); ++word_end ) ;

	if( word_end == word_start ) {
		sqsh_set_error( SQSH_E_EXIST, NULL ) ;
		return 0 ;
	}

	/*-- Back up to the real end of the word --*/
	--word_end ;

	/*
	 * This kind of sucks. Right now we have to limit aliases to
	 * the length of alias_name.  Eventually I would like to allow
	 * arbitrary length aliases.
	 */
	len = min((word_end - word_start) + 1, MAX_ALIAS);
	strncpy( alias_name, word_start, len );
	alias_name[len] = '\0';

	search_node.an_name = alias_name ;

	DBG(sqsh_debug( DEBUG_ALIAS, 
	      "alias_expand: Looking for '%s', length %d\n",
	      alias_name, len );)

	node = avl_find( a->alias_entries, (void*)&search_node ) ;

	/*
	 * Short-circuit if we don't need to perform any sort of alias
	 * expansion, this will save us a buffer copy in the long run.
	 */
	if( node == NULL ) {
		DBG(sqsh_debug(DEBUG_ALIAS, "alias_expand: No match\n");)
		sqsh_set_error( SQSH_E_EXIST, NULL ) ;
		return 0 ;
	}

	/*-- Emtpy our expansion buffer --*/
	varbuf_clear( exp_buf ) ;

	/*
	 * Skip to the first word following the word that was aliased
	 */
	for( args_start = word_end+1; *args_start != '\0' && 
		isspace((int)*args_start); ++args_start ) ;

	/*
	 * Keep a pointer to the end of the argument list.
	 */
	args_end = args_start ;
	for( cptr = args_start; *cptr != '\0'; ++cptr ) {
		if( !isspace((int)*cptr) )
			args_end = cptr + 1;
	}

	/*
	 * The algorithm now is to copy the chunk from the beginning of
	 * the line, up to the end of the line or the first !* that we
	 * encounter.  If we hit a !* then we copy the remainder of cmd_line
	 * (sans the aliased word) in place of it, then continue.
	 */
	chunk_start = node->an_body ;
	chunk_end   = strstr( chunk_start, "!*" ) ;
	while( chunk_end != NULL ) {

		/*-- Copy the chunk into the string --*/
		r = varbuf_strncat( exp_buf, chunk_start, chunk_end - chunk_start ) ;

		/*-- Append the remainder of the command line --*/
		if( r != -1 )
			r = varbuf_strncat( exp_buf, args_start, args_end - args_start ) ;

		if( r == -1 )
			return -1 ;

		did_subst   = True ;
		chunk_start = chunk_end + 2 ;
		chunk_end   = strstr( chunk_start, "!*" ) ;
	}

	/*-- Append remainder of the line --*/
	if( varbuf_strcat( exp_buf, chunk_start ) == -1 )
		return -1 ;

	/*
	 * If the above loop didn't yield a *!, then we go ahead
	 * and just append the arguments that we have thus far.
	 */
	if( did_subst == False ) {
		if( varbuf_charcat( exp_buf, ' ' ) == -1 )
			return -1 ;
		if( varbuf_strncat( exp_buf, args_start, args_end-args_start ) == -1 )
			return -1 ;
	}

	DBG(sqsh_debug( DEBUG_ALIAS, "alias_expand: Expanded '%s'\n", 
	                varbuf_getstr( exp_buf ) );)

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return 1 ;
}

/*
 * alias_test():
 *
 * Similar to alias_expand(), except that alias_test() never actually
 * expands cmd_line, rather it returns a 1 if cmd_line contains an
 * aliased word, or 0 if it doesn't.
 */
int alias_test( a, cmd_line )
	alias_t    *a ;
	char       *cmd_line ;
{
	aliasnode_t  *node ;
	aliasnode_t   search_node ;
	char         *word_start ;
	char         *word_end ;
	int           len;
	char          alias_name[MAX_ALIAS+1];

	/*-- Check parameters --*/
	if( a == NULL || cmd_line == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	/*-- Skip leading white-space --*/
	for( word_start = cmd_line; *word_start != '\0' && 
		isspace((int)*word_start); ++word_start ) ;

	/*-- Find the end of the word --*/
	for( word_end = word_start; *word_end != '\0' && 
		!(isspace((int)*word_end)); ++word_end ) ;

	if( word_end == word_start ) {
		sqsh_set_error( SQSH_E_EXIST, NULL ) ;
		return 0 ;
	}

	--word_end ;

	len = min((word_end - word_start) + 1, MAX_ALIAS);
	strncpy( alias_name, word_start, len );
	alias_name[len] = '\0';

	DBG(sqsh_debug( DEBUG_ALIAS, 
	      "alias_test: Looking for '%s', length %d\n", alias_name, len );)

	search_node.an_name = alias_name ;
	node = avl_find( a->alias_entries, (void*)&search_node ) ;

	if( node == NULL ) {
		DBG(sqsh_debug( DEBUG_ALIAS, "alias_test: No match.\n" );)
		sqsh_set_error( SQSH_E_EXIST, NULL ) ;
		return 0 ;
	}

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return 1 ;
}

/*
 * alias_destroy():
 *
 * Destroys all aliases in a.
 */
int alias_destroy( a )
	alias_t  *a ;
{
	if( a == NULL )
		return False ;

	avl_destroy( a->alias_entries ) ;
	return True ;
}

static aliasnode_t* aliasnode_create( alias_name, alias_body )
	char *alias_name ;
	char *alias_body ;
{
	aliasnode_t  *new_node ;

	new_node = (aliasnode_t*)malloc(sizeof(aliasnode_t)) ;
	if( new_node == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return False ;
	}

	new_node->an_name = sqsh_strdup( alias_name ) ;
	new_node->an_body = sqsh_strdup( alias_body ) ;

	if( new_node->an_name == NULL || new_node->an_body == NULL ) {
		aliasnode_destroy( (void*)new_node ) ;
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return False ;
	}

	return new_node ;
}

static int aliasnode_replace( a, new_body )
	aliasnode_t  *a ;
	char         *new_body ;
{
	char *old_body  ;

	old_body = a->an_body ;
	a->an_body = sqsh_strdup(new_body) ;

	if( a->an_body == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		a->an_body = old_body ;
		return False ;
	}

	free( old_body ) ;
	return True ;
}

static int aliasnode_cmp( a1, a2 )
	void *a1, *a2 ;
{
	return strcmp( ((aliasnode_t*)a1)->an_name, ((aliasnode_t*)a2)->an_name ) ;
}

static void aliasnode_destroy( a )
	void  *a ;
{
	aliasnode_t  *an ;

	an = (aliasnode_t*)a ;

	if( an == NULL )
		return ;

	if( an->an_name )
		free( an->an_name ) ;
	if( an->an_body )
		free( an->an_body ) ;
	free( an ) ;
}


