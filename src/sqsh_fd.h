/*
 * sqsh_fd.h - Routines for safely manipulating file descriptors
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
#ifndef sqsh_fd_h_included
#define sqsh_fd_h_included

#if defined(HAVE_POLL) && defined(HAVE_STROPTS_H)
#include <stropts.h>
#include <poll.h>
#else

typedef struct pollfd {
	int        fd;                  /* File descriptor */
	short      events;              /* Event to wait for */
	short     revents;              /* Event returned */
} pollfd_t;

/*-- Pollable events --*/
#define    POLLIN    ((short)1<<0)
#define    POLLOUT   ((short)1<<1)
#define    POLLERR   ((short)1<<2)

#endif /* !HAVE_POLL */

/*-- Prototypes --*/
int   sqsh_maxfd       _ANSI_ARGS(( void )) ;
int   sqsh_open        _ANSI_ARGS(( char*, int, mode_t )) ;
void  sqsh_fdprint     _ANSI_ARGS(( void )) ;
int   sqsh_dup         _ANSI_ARGS(( int )) ;
int   sqsh_dup2        _ANSI_ARGS(( int, int )) ;
int   sqsh_popen       _ANSI_ARGS(( char*, char*, int*, int* )) ;
pid_t sqsh_ptest       _ANSI_ARGS(( int )) ;
int   sqsh_close       _ANSI_ARGS(( int )) ;
int   sqsh_fsave       _ANSI_ARGS(( void )) ;
int   sqsh_frestore    _ANSI_ARGS(( void )) ;
int   sqsh_poll        _ANSI_ARGS(( struct pollfd *, unsigned long, int ));

#endif /* sqsh_fd_h_included */
