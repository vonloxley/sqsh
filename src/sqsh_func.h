/*
 * sqsh_func.h - Routines for manipulating user defined
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
#ifndef sqsh_func_h_included
#define sqsh_func_h_included
#include "sqsh_avl.h"

/*
 * The following data structure represents a single command that may
 * be run by a user.
 */
typedef struct func_st
{
	char          *func_name;  /* Name of the command */
	char          *func_body;  /* Body of the function */
}
func_t;

/*
 * The following structure is used to encapsulate a gaggle of functions
 * structures from above.
 */
typedef struct funcset_st
{
	avl_t    *fs_funcs ;       /* An AVL tree of func_t's */
}
funcset_t ;

typedef struct funcarg_st
{
	int       argc;
	char    **argv;
}
funcarg_t;

/*-- Prototypes --*/
funcset_t* funcset_create  _ANSI_ARGS(( void )) ;
int        funcset_add     _ANSI_ARGS(( funcset_t*, char*, char* )) ;
func_t*    funcset_get     _ANSI_ARGS(( funcset_t*, char* )) ;
int        funcset_print   _ANSI_ARGS(( funcset_t* )) ;
int        funcset_destroy _ANSI_ARGS(( funcset_t* )) ;

#endif /* sqsh_func_h_included */
