/*
 * sqsh_cmd.h - Routines for manipulating named function pointers
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
#ifndef sqsh_cmd_h_included
#define sqsh_cmd_h_included
#include "sqsh_avl.h"

/*
 * The following represents a command function, which is expected
 * to take two parameters (argc, and argv) and return an integer
 * return value.
 */
typedef int (cmd_f) _ANSI_ARGS(( int, char** )) ;

/*
 * The following data structure represents a single command that may
 * be run by a user.  It is simply a named pointer to a command function.
 */
typedef struct cmd_st {
	char          *cmd_name ;  /* Name of the command */
	cmd_f         *cmd_ptr ;   /* Pointer to the actual command function */
} cmd_t ;

/*
 * The following structure is used to encapsulate a gaggle of command
 * structures from above.
 */
typedef struct cmdset_st {
	avl_t    *cs_cmd ;         /* An AVL tree of cmd_t's */
} cmdset_t ;

/*-- Prototypes --*/
cmdset_t* cmdset_create  _ANSI_ARGS(( void )) ;
int       cmdset_add     _ANSI_ARGS(( cmdset_t*, char*, cmd_f* )) ;
cmd_t*    cmdset_get     _ANSI_ARGS(( cmdset_t*, char* )) ;
int       cmdset_print   _ANSI_ARGS(( cmdset_t* )) ;
int       cmdset_destroy _ANSI_ARGS(( cmdset_t* )) ;

#endif /* sqsh_cmd_h_included */
