/*
 * sqsh_expand.h - Routinues for expanding environment variables
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
#ifndef sqsh_expand_h_included
#define sqsh_expand_h_included
#include "sqsh_varbuf.h"

/*
 * The following flags are used to modify the way in which sqsh_expand()
 * behaves for certain characters.
 */
#define EXP_STRIPQUOTE      (1<<0)  /* Throw away quotes during expansion */
#define EXP_STRIPESC        (1<<1)  /* Strip out escape sequences */
#define EXP_STRIPNL         (1<<2)  /* Strip out newlines */
#define EXP_IGNORECMD       (1<<3)  /* Ignore command substitution */
#define EXP_COMMENT         (1<<4)  /* Pay attention to comments */
#define EXP_COLUMNS         (1<<5)  /* Pay attention to comments */

/*
 * Passing a positive value into sqsh_nexpand() causes it to expand until
 * we hit n characters.
 */
#define EXP_EOF             -1       /* Stop at EOF */
#define EXP_WORD            -2       /* Expand to the first word */

/*-- Prototype --*/
int sqsh_expand  _ANSI_ARGS(( char*, varbuf_t*, int )) ;
int sqsh_nexpand _ANSI_ARGS(( char*, varbuf_t*, int, int )) ;
/* sqsh-2.1.6 feature - Expand color prompt function */
char *expand_color_prompt _ANSI_ARGS(( char*, int )) ;

#endif  /* sqsh_expand_h_included */
