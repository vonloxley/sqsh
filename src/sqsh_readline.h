/*
 * sqsh_readline.h - Generic wrapper around the GNU readline function
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
#ifndef sqsh_readline_h_included
#define sqsh_readline_h_included

#if defined(USE_READLINE)
#include <readline/readline.h>

/*
 * Readline history functions - for some reason not all
 * readline installs have history.h available, so we do
 * this.
 */
#if !defined (PARAMS)
#  if defined (__STDC__) || defined (__GNUC__) || defined (__cplusplus)
#    define PARAMS(protos) protos
#  else
#    define PARAMS(protos) ()
#  endif
#endif /* PARAMS */

extern void _rl_clear_screen PARAMS((void));

#if !defined (_HISTORY_H_)
extern void stifle_history   PARAMS((int));
extern int  unstifle_history PARAMS((void));
extern int  read_history     PARAMS((const char *));
extern int  write_history    PARAMS((const char *));
extern void add_history      PARAMS((const char *));
#endif /*_HISTORY_H_ */

#endif /* USE_READLINE */

/*-- Prototypes --*/
int   sqsh_readline_init  _ANSI_ARGS(( void ));
int   sqsh_readline_exit  _ANSI_ARGS(( void ));
char* sqsh_readline       _ANSI_ARGS(( char* )) ;
int   sqsh_readline_load  _ANSI_ARGS(( void  )) ;
int   sqsh_readline_read  _ANSI_ARGS(( char* )) ;
int   sqsh_readline_add   _ANSI_ARGS(( char* )) ;
int   sqsh_readline_clear _ANSI_ARGS(( void )) ;
#endif /* sqsh_readline_h_included */
