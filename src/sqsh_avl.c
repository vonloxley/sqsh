/*
 * sqsh_avl.c - Routines for manipulating AVL trees
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
 *
 * This module implements an AVL tree (short for Adelson-Velskii and
 * Landis).  In short, an AVL tree remains as balanced as possible
 * following each insertion, guaranteeing a worst case shearch time
 * of O(log n).  Because of the overhead of balancing the tree upon
 * insertion, these trees are ideal only for frequent searches, not
 * frequent insertions or deletions.  This code was basically ripped
 * off from "Data Structures and Algorithm Analysis" by Mark Allen 
 * Weiss (pgs 107-119).  NOTE:  I realize that I could have written
 * a far more efficient non-recursive version of avl_insert(), but 
 * I was really looking for readability.
 *
 * Another nice side-effect of an AVL tree (any tree, really) is that
 * avl_first() and avl_next() always return data in sorted order.
 */
#include <stdio.h>
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_avl.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_avl.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $ " ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- internal prototypes --*/
static int      avl_push        _ANSI_ARGS(( avl_t*, tnode_t* )) ;
static tnode_t* avl_pop         _ANSI_ARGS(( avl_t* )) ;
static tnode_t* tn_create       _ANSI_ARGS(( void* )) ;
static void     tn_insert       _ANSI_ARGS(( avl_t*, tnode_t**, tnode_t* ));
static void     tn_single_left  _ANSI_ARGS(( tnode_t** )) ;
static void     tn_single_right _ANSI_ARGS(( tnode_t** )) ;
static void     tn_double_left  _ANSI_ARGS(( tnode_t** )) ;
static void     tn_double_right _ANSI_ARGS(( tnode_t** )) ;
static void     tn_destroy      _ANSI_ARGS(( tnode_t*, avl_destroy_t* )) ;

/* macro to compute the height of an avl sub-tree */
#define tn_hi(tn)   (((tn) == (tnode_t*)NULL)?(-1):(tn)->tn_height)

avl_t* avl_create( cmp_func, destroy_func )
	avl_cmp_t     *cmp_func ;
	avl_destroy_t *destroy_func ;
{
	avl_t *avl ;

	/*-- Check for bad parameters --*/
	if( cmp_func == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return (avl_t*)NULL ;
	}

	/*-- create the raw data structure --*/
	avl = (avl_t*)malloc( sizeof(avl_t) ) ;

	/* 
	 * Check to see if the malloc was succesfull.  The funny thing
	 * about this is that on most machines this will success regard-
	 * less of wether or not there is actually any free memory.
	 */
	if( avl == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return (avl_t*)NULL ;
	}

	/*-- now, just fill in fields --*/
	avl->avl_destroy     = destroy_func ;
	avl->avl_cmp         = cmp_func ;
	avl->avl_root        = (tnode_t*)NULL ;
	avl->avl_top         = 0 ;
	avl->avl_nitems      = 0 ;
	avl->avl_state       = AVL_ST_NONE ;

	/*-- Success! --*/
	sqsh_set_error( SQSH_E_NONE, NULL ) ;

	/*-- fini --*/
	return avl ;
}

int avl_insert( avl, data )
	avl_t *avl ;
	void  *data ;
{
	tnode_t  *tn ;        /* newly created node */

	/*
	 * Ok, go ahead and be completely anal about things.  I would rather
	 * do this and make bugs easier to track down for the programmer in
	 * the future.
	 */
	if( avl == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*
	 * Update the state of the tree.  If someone was in the process of
	 * searching the tree and they inserted a new node, then we take them
	 * out of the TRAVERSE state, same goes for s search.
	 */
	avl->avl_state = AVL_ST_NONE ;

	/*
	 * Attempt to create a tree-node to insert into the avl tree.  If
	 * this fails we raise and error condition.
	 */
	if( (tn = tn_create( data )) == NULL )
		return False ;

	/*-- This can't return an error condition --*/
	tn_insert( avl, &avl->avl_root, tn ) ;
	++avl->avl_nitems ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

int avl_delete( avl, data )
	avl_t *avl ;
	void  *data ;
{
	tnode_t *tn ;
	int      x ;

	/*-- Check arguments --*/
	if( avl == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	tn = avl->avl_root ;
	while( tn != NULL ) {
		x = avl->avl_cmp(data, tn->tn_data) ;
		if( x == 0 && tn->tn_deleted == False )
			break ;
		if( x <= 0 ) 
			tn = tn->tn_left ;
		else
			tn = tn->tn_right ;
	}

	if( tn != NULL ) {
		tn->tn_deleted = True ;
		sqsh_set_error( SQSH_E_NONE, NULL ) ;
		return True ;
	}

	sqsh_set_error( SQSH_E_EXIST, NULL ) ;
	return False ;
}

void* avl_find( avl, data )
	avl_t *avl ;
	void  *data ;
{
	tnode_t *tn ;
	int      x ;

	/*
	 * Once again, I am being relatively anal about things, but
	 * I think this is good in the long run.
	 */
	if( avl == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return (void*)NULL ;
	}

	/*-- No error condition past here --*/
	sqsh_set_error( SQSH_E_NONE, NULL ) ;

	tn = avl->avl_root ;
	while( tn != NULL ) {
		x = avl->avl_cmp(data, tn->tn_data) ;
		if( x == 0 && tn->tn_deleted == False )
			break ;
		if( x <= 0 ) 
			tn = tn->tn_left ;
		else
			tn = tn->tn_right ;
	}

	if( tn != NULL ) {
		/*
		 * Now, since there may be more nodes that match the search
		 * request, and we know that if there are more nodes then
		 * they are going to be on the left of this node, then push
		 * whatever is on the left of this node so that avl_next()
		 * can return that node.
		 */
		if( tn->tn_left != NULL ) {
			/*
			 * Make sure we let avl_next() know what is going on,
			 * so it can continue the search.
			 */
			avl->avl_state = AVL_ST_SEARCH ;

			/*
			 * Keep around a pointer to the expression that we are searching
			 * for.  Logically, what I should do is keep the pointer that
			 * the caller passed in (void *data), but I don't trust them,
			 * so instead I am keeping a pointer to the search expression
			 * that matched.
			 */
			avl->avl_search = tn->tn_data ;
			avl->avl_top    = 0 ;

			/*
			 * Push the node we found onto the stack in case someone wants
			 * to call avl_next() later.
			 */
			if( avl_push( avl, tn->tn_left ) == False )
				return False ;

		} else
			avl->avl_state = AVL_ST_NONE ;

		return tn->tn_data ;
	}

	/*
	 * There was no match, so the avl tree is currently no longer
	 * doing anything of interest.
	 */
	avl->avl_state = AVL_ST_NONE ;

	return (void*)NULL ;
}

void* avl_first( avl )
	avl_t  *avl ;
{
	tnode_t *cur, *left ;

	/*-- Check ye-olde paramters --*/
	if( avl == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return (void*)NULL ;
	}

	avl->avl_top = 0 ;
	cur = avl->avl_root ;
	while( cur != NULL && cur->tn_left != NULL ) {
		if( avl_push( avl, cur ) == False )
			return NULL ;
		cur = cur->tn_left ;
	}

	if( cur != NULL && cur->tn_right != NULL ) {
		if( avl_push( avl, cur->tn_right ) == False )
			return NULL ;
	}

	/*
	 * If there current node has been deleted, then perform the
	 * same logic as avl_next().
	 */
	while( cur != NULL && cur->tn_deleted ) {
		cur = avl_pop( avl ) ;

		while( cur != NULL ) {
			if( cur->tn_right != NULL ) {
				left = cur->tn_right ;
				while( left ) {
					if( avl_push( avl, left ) == False )
						return NULL ;
					left = left->tn_left ;
				}
			}

			if( cur->tn_deleted == False )
				break ;
			else
				cur = avl_pop(avl) ;
		}
	}

	/*-- No errors past here --*/
	sqsh_set_error( SQSH_E_NONE, NULL ) ;

	/*
	 * If we have found a match, then put the avl structure in the
	 * traversal state, this will let avl_next() know what to do 
	 * when it is called.
	 */
	if( cur != NULL ) {
		avl->avl_state = AVL_ST_TRAVERSE ;
		return( cur->tn_data ) ;
	}
	return (void*)NULL ;
}

void* avl_next( avl )
	avl_t *avl ;
{
	tnode_t *cur, *left ;

	/*-- Check parameters --*/
	if( avl == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return (void*)NULL ;
	}

	/*-- Check to see what state we are in --*/
	if( avl->avl_state == AVL_ST_NONE ) {
		sqsh_set_error( SQSH_E_BADSTATE, NULL ) ;
		return NULL ;
	}

	/*-- Shouldn't have many error past here --*/
	sqsh_set_error( SQSH_E_NONE, NULL ) ;

	if( (cur = avl_pop(avl)) == NULL )
		return NULL ;

	if( avl->avl_state == AVL_ST_TRAVERSE ) {

		while( cur != NULL ) {
			if( cur->tn_right != NULL ) {
				left = cur->tn_right ;
				while( left ) {
					if( avl_push( avl, left ) == False )
						return NULL ;
					left = left->tn_left ;
				}
			}

			if( cur->tn_deleted == False )
				break ;
			else
				cur = avl_pop(avl) ;
		}

		if( cur != NULL )
			return( cur->tn_data ) ;

	} else if (avl->avl_state == AVL_ST_SEARCH ) {

		/*
		 * If the tree is in the search state, then take the
		 * next node that was to be compared off the stack
		 * and compare it to our search criteria (avl->avl_state->s_search).
		 * If there is a match, then we need to stick the next one
		 * to be searched back onto the stack, and return the
		 * match.
		 */
		while( cur != NULL && 
		       avl->avl_cmp( avl->avl_search, cur->tn_data ) == 0 ) {

			if( cur->tn_deleted == False ) {
				/*
				 * If we are capable of searching further through the
				 * tree, then push the next search location on the stack,
				 * otherwise mark the tree as being done with its
				 * search.
				 */
				if( cur->tn_left != NULL ) {
					if( avl_push( avl, cur->tn_left ) == False )
						return NULL ;
				} else {
					avl->avl_state = AVL_ST_NONE ;
				}

				return cur->tn_data ;

			} else
				cur = cur->tn_left ;
		}
	}

	/*
	 * If there is no next node, or the tree wasn't in a valid state
	 * then return a NULL for "no match".
	 */
	avl->avl_state = AVL_ST_NONE ;
	return (void*)NULL ;
}

int avl_nitems( avl )
	avl_t *avl ;
{
	if( avl == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}
	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return avl->avl_nitems ;
}

int avl_destroy( avl )
	avl_t         *avl ;
{
	if( avl == NULL )
		return False ;

	if( avl->avl_root != NULL )
		tn_destroy( avl->avl_root, avl->avl_destroy ) ;
	free( avl ) ;
	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

static tnode_t* tn_create( data )
	void *data ;
{
	tnode_t *tn ;

	tn = (tnode_t*)malloc( sizeof(tnode_t) ) ;

	if( tn == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return NULL ;
	}

	tn->tn_deleted = False ;
	tn->tn_height  = 0 ;
	tn->tn_data    = data ;
	tn->tn_left    = (tnode_t*)NULL ;
	tn->tn_right   = (tnode_t*)NULL ;

	return tn ;
}

static void tn_insert( avl, tree, node )
	avl_t    *avl ;
	tnode_t  **tree ;
	tnode_t  *node ;
{
	tnode_t *root ;
	int      x ;
	void    *t ;

	/*
	 * First, the exit condition for our recursive function.
	 */
	if( *tree == (tnode_t*)NULL ) {
		*tree = node ;
		return ;
	}

	root = *tree ;
	if( (x = avl->avl_cmp( node->tn_data, root->tn_data )) <= 0 ) {

		/*
		 * If the current root node is equal to the node to be inserted
		 * and the root node has been marked for deletion, then we pull
		 * a little trick.  We swap the data contained in the root
		 * node with the data in the insert node and delete the insert
		 * node. No rotation or anything needs to happen.
		 */
		if( x == 0 && root->tn_deleted ) {
			t = root->tn_data ;
			root->tn_data = node->tn_data ;
			node->tn_data = t ;
			tn_destroy( node, avl->avl_destroy ) ;

			root->tn_deleted = False ;
			return ;
		}

		tn_insert( avl, &root->tn_left, node ) ;
		if( (tn_hi(root->tn_left) - tn_hi(root->tn_right)) == 2 ) {

			if( avl->avl_cmp( node->tn_data, root->tn_left->tn_data ) <= 0 ) {
				tn_single_left( tree ) ;
			} else {
				tn_double_left( tree ) ;
			}
		} else {
			root->tn_height = max(tn_hi(root->tn_left),
			                      tn_hi(root->tn_right)) + 1 ;
		}
	} else {

		tn_insert( avl, &root->tn_right, node ) ;
		if( (tn_hi(root->tn_right) - tn_hi(root->tn_left)) == 2 ) {

			if( avl->avl_cmp( node->tn_data, root->tn_right->tn_data ) > 0 ) {
				tn_single_right( tree ) ;
			} else {
				tn_double_right( tree ) ;
			}
		} else {
			root->tn_height = max(tn_hi(root->tn_left),
			                      tn_hi(root->tn_right)) + 1 ;
		}
	}
}

static void tn_single_left( tree )
	tnode_t **tree ;
{
	tnode_t *root ;
	tnode_t *tmp ;

	root          = *tree ;
	tmp           = root->tn_left ;
	root->tn_left = tmp->tn_right ;
	tmp->tn_right = root ;

	root->tn_height = max(tn_hi(root->tn_left),tn_hi(root->tn_right))+1;
	tmp->tn_height = max(tn_hi(tmp->tn_left),tn_hi(root))+1;

	*tree = tmp ;
}

static void tn_double_left( tree )
	tnode_t **tree ;
{
	tn_single_right( &(*tree)->tn_left ) ;
	tn_single_left( tree ) ;
}

static void tn_single_right( tree )
	tnode_t **tree ;
{
	tnode_t *root ;
	tnode_t *tmp ;

	root           = *tree ;
	tmp            = root->tn_right ;
	root->tn_right = tmp->tn_left ;
	tmp->tn_left   = root ;

	root->tn_height = max(tn_hi(root->tn_left),tn_hi(root->tn_right))+1;
	tmp->tn_height = max(tn_hi(tmp->tn_right),tn_hi(root))+1;
	*tree = tmp ;
}

static void tn_double_right( tree )
	tnode_t **tree ;
{
	tn_single_left( &(*tree)->tn_right ) ;
	tn_single_right( tree ) ;
}

/* icky..another recursive function */
static void tn_destroy( tn, destroy_func )
	tnode_t       *tn ;
	avl_destroy_t *destroy_func ;
{
	if( tn == NULL )
		return ;

	DBG(sqsh_debug(DEBUG_AVL,"tn_destroy: Destroying node %p\n", tn->tn_data );)

	if( destroy_func != (avl_destroy_t*)NULL && tn->tn_data != (void*)NULL )
		destroy_func( tn->tn_data ) ;

	tn_destroy( tn->tn_left, destroy_func ) ;
	tn_destroy( tn->tn_right, destroy_func ) ;
	free( tn ) ;
}

static int avl_push( avl, tn )
	avl_t   *avl ;
	tnode_t *tn ;
{
	if( avl->avl_top == AVL_MAX_DEPTH ) {
		sqsh_set_error( SQSH_E_NOMEM, "Stack full" ) ;
		return False ;
	}

	avl->avl_stack[avl->avl_top++] = tn ;
	return True ;
}

static tnode_t* avl_pop( avl )
	avl_t   *avl ;
{
	if( avl->avl_top == 0 )
		return NULL ;

	--avl->avl_top ;
	return avl->avl_stack[avl->avl_top] ;
}
