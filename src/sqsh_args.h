/*
 * sqsh_args.h - Routines & structures for manipulating argc/argv pairs.
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
#ifndef sqsh_args_h_included
#define sqsh_args_h_included

/*
 * The following data structure represents an array of strings (an
 * arugments list).
 */
typedef struct args_st {
	int        a_argc ;     /* Number of arguments stored */
	char     **a_argv ;     /* Array of pointer to arguments */
	int        a_len ;      /* Current length of the a_argv array */
	int        a_growsize ; /* Number of units by which to grow a_len */
} args_t ;

/*-- Prototypes --*/
args_t* args_create   _ANSI_ARGS(( int )) ;
int     args_add      _ANSI_ARGS(( args_t*, char* )) ;
int     args_argc     _ANSI_ARGS(( args_t* )) ;
char**  args_argv     _ANSI_ARGS(( args_t* )) ;
void    args_destroy  _ANSI_ARGS(( args_t* )) ;

#endif /* sqsh_args_h_included */
