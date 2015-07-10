/*
 * sqsh_getopt.c - Reusable version of getopt(3c)
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
#ifndef sqsh_getopt_h_included
#define sqsh_getopt_h_included

typedef struct sqsh_longopt_st {
    char *name;
    int   reqarg;
    int   shortopt;
} sqsh_longopt_t;

/*-- Prototypes --*/
int sqsh_getopt          _ANSI_ARGS(( int, char**, char* )) ;
int sqsh_getopt_env      _ANSI_ARGS(( char*, char* )) ;
int sqsh_getopt_combined _ANSI_ARGS(( char*, int, char**, char* )) ;
int sqsh_getopt_reset    _ANSI_ARGS(( void )) ;
int sqsh_getopt_long     _ANSI_ARGS(( int, char**, char*, sqsh_longopt_t[] )) ;

#endif /* sqsh_getopt_h_included */
