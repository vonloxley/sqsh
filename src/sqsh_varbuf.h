/*
 * sqsh_varbuf.h - dynamic string buffer
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
#ifndef sqsh_varbuf_h_included
#define sqsh_varbuf_h_included
#include "sqsh_config.h"
#if defined(__ansi__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/*
 * BUF_GROWSIZE defines the increments in which a string buffer will
 * malloc memory.
 */
typedef struct {
	int vb_tot_length ;   /* Total length of malloced memory */
	int vb_cur_length ;   /* Length of actual string in buffer w/o null char */
	int vb_growsize ;     /* Increments by which buffer will grow */
	int vb_errno ;        /* Most recent error condition */
	char *vb_buffer ;     /* String buffer */
} varbuf_t ;

#define varbuf_getlen(x) ((x)->vb_cur_length)
#define varbuf_getbuf(x) ((x)->vb_buffer)

/*-- Prototypes --*/
varbuf_t* varbuf_create   _ANSI_ARGS(( int )) ;
int       varbuf_destroy  _ANSI_ARGS(( varbuf_t*)) ;
int       varbuf_ncpy     _ANSI_ARGS(( varbuf_t*,char*,int )) ;
int       varbuf_strcat   _ANSI_ARGS(( varbuf_t*,char* )) ;
int       varbuf_strncat  _ANSI_ARGS(( varbuf_t*,char*,int )) ;
int       varbuf_charcat  _ANSI_ARGS(( varbuf_t*,char )) ;
int       varbuf_strcpy   _ANSI_ARGS(( varbuf_t*,char* )) ;
int       varbuf_charcpy  _ANSI_ARGS(( varbuf_t*,char )) ;
int       varbuf_subst    _ANSI_ARGS(( varbuf_t*, int, int, char* )) ;
int       varbuf_clear    _ANSI_ARGS(( varbuf_t* )) ;
int       varbuf_size     _ANSI_ARGS(( varbuf_t*,long )) ;
int       varbuf_printf   _ANSI_ARGS(( varbuf_t*,char*,... )) ;
int       varbuf_vprintf  _ANSI_ARGS(( varbuf_t*,char*,va_list )) ;
int       varbuf_setlen   _ANSI_ARGS(( varbuf_t*,long )) ;
char*     varbuf_getstr   _ANSI_ARGS(( varbuf_t* )) ;

#endif /* sqsh_varbuf_h_included */
