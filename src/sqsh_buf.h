/*
 * sqsh_buf.h - "Smart" named buffer functions
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
#ifndef sqsh_buf_h_included
#define sqsh_buf_h_included

/*-- Prototypes --*/
char*  buf_get     _ANSI_ARGS(( char* )) ;
int    buf_put     _ANSI_ARGS(( char*, char* )) ;
int    buf_append  _ANSI_ARGS(( char*, char* )) ;
int    buf_load    _ANSI_ARGS(( char*, char*, int )) ;
int    buf_save    _ANSI_ARGS(( char*, char*, char* )) ;
int    buf_can_put _ANSI_ARGS(( char* )) ;

#endif /* sqsh_buf_h_included */
