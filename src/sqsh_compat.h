/*
 * sqsh_compat.h - Libc routines that may not exist on all machines
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
#ifndef sqsh_compat_h_included
#define sqsh_compat_h_included

/*
 * The following functions are defined in sqsh_compat.c if they are
 * not available on the current system.
 */
#if !(defined(HAVE_STRCASECMP))
int strcasecmp   _ANSI_ARGS(( const char*, const char* )) ;
#endif /* !HAVE_STRCASECMP */

/*
 * The following came from Helmut Ruholl (helmut@isv-gmbh.de), as
 * a kludge to get sqsh to compile on Sequents.  If GNU autoconf
 * couldn't find gettimeofday(), then it tries to use
 * get_process_stats() instead.  If that is not availale, it pretty
 * much gives up and sticks with gettimeofday().
 */
#if !(defined(HAVE_GETTIMEOFDAY))
#   if defined(HAVE_GET_PROCESS_STATS)
#      define gettimeofday(a,b)  get_process_stats(a,getpid(),0,0)
#   endif /* HAVE_GET_PROCESS_STATS */
#endif  /* HAVE_GETTIMEOFDAY */

#if !(defined(HAVE_STRERROR))
const char* strerror   _ANSI_ARGS(( int )) ;
#endif /* !HAVE_STRERROR */

#if !(defined(HAVE_STRFTIME))
#  if defined(HAVE_TIMELOCAL)
#     define strftime(s,max,format,tm)  cftime(s,format,timelocal(tm))
#  else
#     define strftime(s,max,format,tm)  cftime(s,format,mktime(tm))
#  endif
#endif

/*
 * The following functions are replaced by macros that achieve the same
 * affect, if they are not available on this system.
 */
#if !(defined(HAVE_CFTIME))
#   if defined(HAVE_LOCALTIME)
#      define cftime(s,format,clock)  strftime(s,64,format,localtime(clock))
#   else  /* HAVE_LOCALTIME */
#      define cftime(s,format,clock)  strftime(s,64,format,timelocal(clock))
#   endif /* !HAVE_LOCALTIME */
#endif /* !HAVE_CFTIME */

#if !(defined(HAVE_STRCHR))
#   define strchr(s,c)            index(s,c)
#endif

#if !(defined(HAVE_MEMCPY))
#   define memcpy(b1,b2,length)   bcopy(b1,b2,length)
#endif

#if !(defined(HAVE_MEMMOVE))
#   define memmove(b1,b2,length)   bcopy(b1,b2,length)
#endif

/*-- Prototypes --*/
char* sqsh_strdup     _ANSI_ARGS(( char* ));
char* sqsh_strndup    _ANSI_ARGS(( char*, int ));
int   sqsh_getinput   _ANSI_ARGS(( char*, char*, int, int ));
int   sqsh_getc       _ANSI_ARGS(( FILE* ));
int   sqsh_setenv     _ANSI_ARGS(( char*, char* ));

#endif /* sqsh_compat_h_included */
