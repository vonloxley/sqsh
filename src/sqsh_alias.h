/*
 * sqsh_alias.h - Routines for manipulating command aliases
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
#ifndef sqsh_alias_h_included
#define sqsh_alias_h_included
#include "sqsh_varbuf.h"
#include "sqsh_avl.h"

/*
 * The following structure defines an alias to a command in sqsh. In
 * reality, an alias is not directly tied to a command. It simply
 * describes a method for replacing the first word in a line of text
 * with another word.
 */
typedef struct aliasnode_st {
	char   *an_name ;        /* Name of the alias */
	char   *an_body ;        /* The body of the alias to be expanded */
} aliasnode_t ;

typedef struct alias_st {
	avl_t  *alias_entries ;  /* AVL Tree of alias_t's */
} alias_t ;

/*-- Prototypes --*/
alias_t* alias_create    _ANSI_ARGS(( void )) ;
int      alias_add       _ANSI_ARGS(( alias_t*, char*, char* )) ;
int      alias_remove    _ANSI_ARGS(( alias_t*, char* )) ;
int      alias_expand    _ANSI_ARGS(( alias_t*, char*, varbuf_t* )) ;
int      alias_test      _ANSI_ARGS(( alias_t*, char* )) ;
int      alias_destroy   _ANSI_ARGS(( alias_t* )) ;

#endif /* sqsh_alias_h_included */
