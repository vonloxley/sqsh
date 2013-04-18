/*
 * sqsh_debug.h - Prototypes for debugging routines.
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
#ifndef sqsh_debug_h_included
#define sqsh_debug_h_included

#define DEBUG_FD       (1<<0)   /* Debug file descriptor code */
#define DEBUG_SIGCLD   (1<<1)   /* Debug code for SIGCHLD handling */
#define DEBUG_ENV      (1<<2)   /* Debug environment variables */
#define DEBUG_JOB      (1<<3)   /* Debug launching of jobs */
#define DEBUG_EXPAND   (1<<4)   /* Debug expansion of variables */
#define DEBUG_AVL      (1<<5)   /* Debug avl tree code */
#define DEBUG_READLINE (1<<6)   /* Debug readline code */
#define DEBUG_SCREEN   (1<<7)   /* Debug anything related to the display */
#define DEBUG_ALIAS    (1<<8)   /* Debug command aliasing */
#define DEBUG_DISPLAY  (1<<9)   /* Debug result set display routines */
#define DEBUG_BCP      (1<<10)  /* Debug the bcp command */
#define DEBUG_RPC      (1<<11)  /* Debug the rpc command */
#define DEBUG_ERROR    (1<<12)  /* Debug error handlers */
#define DEBUG_SIG      (1<<13)  /* Debug signal handlers */
#define DEBUG_HISTORY  (1<<14)  /* Debug history processing */
#define DEBUG_ALL     ~(0)      /* Turn on all debugging */

#if defined(DEBUG)
#  define  DBG(x)    x
#else
#  define  DBG(x)
#endif

int  sqsh_debug_level _ANSI_ARGS(( int )) ;
int  sqsh_debug_show  _ANSI_ARGS(( int )) ;
void sqsh_debug       _ANSI_ARGS(( int, char*, ... )) ;

#endif /* sqsh_debug_h_included */
