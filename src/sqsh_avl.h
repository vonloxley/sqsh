/*
 * sqsh_avl.h - Structures and Prototypes for manipulating an AVL tree
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
#ifndef avl_h_included
#define avl_h_included

/*-- state in which the tree my reside --*/
#define AVL_ST_NONE             0    /* nothing interesting going on */
#define AVL_ST_TRAVERSE         1    /* currently being traversed */
#define AVL_ST_SEARCH           2    /* currently being searched */

/*
 * Maximum depth of AVL tree.
 */
#define AVL_MAX_DEPTH          32 

typedef struct tnode_st {
	unsigned char    tn_deleted ;   /* if True, then node has been deleted */
	int              tn_height ;    /* height of sub-tree */
	void            *tn_data ;      /* user data attached to tree node */
	struct tnode_st *tn_left ;      /* pointer to left child */
	struct tnode_st *tn_right ;     /* pointer to right child */
} tnode_t ;

typedef int  (avl_cmp_t)     _ANSI_ARGS((void*,void*)) ;
typedef void (avl_destroy_t) _ANSI_ARGS((void*)) ;

typedef struct avl_st {
	tnode_t       *avl_root ;        /* pointer to the root of the tree */
	avl_cmp_t     *avl_cmp ;         /* comparison function */
	avl_destroy_t *avl_destroy ;     /* destroy function */
	int            avl_nitems ;      /* number of items in the tree */
	tnode_t       *avl_stack[AVL_MAX_DEPTH] ; /* Traversal push stack */
	int            avl_top ;         /* Top of stack */
	int            avl_state ;       /* Search state */
	void          *avl_search ;      /* Last data searched for */
} avl_t ;

/*-- Prototypes --*/
avl_t*   avl_create     _ANSI_ARGS(( avl_cmp_t*, avl_destroy_t* )) ;
int      avl_insert     _ANSI_ARGS(( avl_t*, void* )) ;
int      avl_delete     _ANSI_ARGS(( avl_t*, void* )) ;
void*    avl_find       _ANSI_ARGS(( avl_t*, void* )) ;
void*    avl_first      _ANSI_ARGS(( avl_t* )) ;
void*    avl_next       _ANSI_ARGS(( avl_t* )) ;
int      avl_nitems     _ANSI_ARGS(( avl_t* )) ;
int      avl_destroy    _ANSI_ARGS(( avl_t* )) ;

#endif
