/*
 * sqsh_config.h - User configuration header file
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
#ifndef sqsh_config_h_included
#define sqsh_config_h_included
#include "config.h"

/*
 * Begin user configuration section
 */

/*
 * The maximum length of a password.  I just pulled this one out of
 * the air.
 * sqsh-2.1.6 - Increased value from 12 to 30
 * sqsh-2.4   - Increased value from 30 to 64
 */
#ifndef SQSH_PASSLEN
#define SQSH_PASSLEN    64
#endif

/*
 * This is the location of the history save file.  Be careful about where
 * this is placed and the ownership of it, otherwise you could end up
 * with others seeing your "sp_addlogin" commands.
 */
#ifndef SQSH_HISTORY
#define SQSH_HISTORY    "${HOME}/.sqsh_history"
#endif

/*
 * This is the location of the readline history file.  This file is
 * only processed when sqsh is compiled with readline support.
 */
#ifndef SQSH_RLHISTORY
#define SQSH_RLHISTORY  "${HOME}/.sqsh_readline"
#endif

/*
 * This is the location of the user's configuration file. The
 * contents of this file are executed immediatly prior to
 * parsing the command line arguments to sqsh.  Note that this
 * value is actually set in config.h because it relies on the
 * --prefix parameter passed in during compile.
 */
#ifndef SQSH_RC
#define SQSH_RC        "${HOME}/.sqshrc"
#endif

/*
 * The location of the users session configuration file. The
 * contents of this file are executed immediately prior to a
 * new connection being established to the database.
 */
#ifndef SQSH_SESSION
#define SQSH_SESSION    "${HOME}/.sqsh_session"
#endif

/*
 * The default location for a users keyword completion list.
 * This list has no effect if this file cannot be found or if
 * sqsh doesn't have readline support compiled in.
 */
#ifndef SQSH_WORDS
#define SQSH_WORDS      "${HOME}/.sqsh_words"
#endif

/*
 * The following defines where deferred output of a background task
 * is placed.  There will be a filed called sqsh-dfr.<pid>-<jobid>
 * placed in this directory to which the output will be written.
 */
#ifndef SQSH_TMP
#define SQSH_TMP     "/tmp"
#endif

/*
 * The following defines where help files are placed.  These files
 * are expected to be in roff format.
 */
#ifndef SQSH_HELP
#define SQSH_HELP    "/usr/local/lib/sqsh/help"
#endif

/*
 * All commands run in a pipe-line are executed within the following
 * shell.  Typically this should be the location of some form of
 * bourne shell.
 */
#ifndef SQSH_SHELL
#define SQSH_SHELL         "/bin/sh"
#endif

/*
 * The default editor to be used if $EDITOR or $VISUAL is not
 * set.
 */
#ifndef SQSH_EDITOR
#define SQSH_EDITOR        "vi"
#endif

/*
 * Upon start-up sqsh attempts to determine the maximum number of
 * file descriptors by requesting them from the OS. If it can't find
 * them then the following number is used.
 */
#ifndef SQSH_MAXFD
#define SQSH_MAXFD         256
#endif

#if defined(USE_READLINE)
/*
 * sqsh-2.1.8 - Define the default query that will be executed in
 * case keyword_dynamic is enabled and an interactive connection is setup
 * to a Sybase ASE or Microsoft MSSQL database server, or the database
 * context is changed due to a "use" command.
 *
 */
#  ifndef SQSH_KEYQUERY
#  define SQSH_KEYQUERY    "select name from sysobjects order by name"
#  endif
#endif

/*
 * End user configuration section
 */

/*
 * Current version number.
 */
#define SQSH_VERSION     "sqsh-2.5"

#if !defined(__ansi__)
#  if defined(__STDC__) || defined(STDC_HEADERS) || defined(PROTOTYPES)
#    define __ansi__
#  endif
#endif

#if !defined(_ANSI_ARGS)
#  if defined(__ansi__)
#    define _ANSI_ARGS(x)      x
#  else
#    define _ANSI_ARGS(x)     ()
#  endif
#endif

#ifndef min
#define min(x,y)  ((x)<(y)?(x):(y))
#define max(x,y)  ((x)<(y)?(y):(x))
#endif

#ifndef True
#define True   1
#define False  0
#endif

/*
 * The following is used when dealing with GCC and -Wall.  The USE()
 * macro will suppress warnings about unused variables and RCS_Id's.
 */
#if __GNUC__ == 2
#define USE(var) \
   static const char sizeof##var = sizeof(sizeof##var) + sizeof(var);
#else
#  define USE(var)
#endif

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

extern int errno;

#if !defined(HAVE_VOLATILE)
#  define volatile
#endif

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(HAVE_SYS_WAIT_H)
#include <sys/wait.h>
#endif

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif

#if defined(HAVE_MEMORY_H)
#include <memory.h>
#endif

#if defined(HAVE_TIME_H)
#include <time.h>
#endif

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif

#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

/*
 * SQSH_MAXPATH is to contain the maximum length of a file that we
 * can deal with.  First, we try to define it in terms of MAXPATHLEN,
 * then we try the POSIX, PATH_MAX, failing that we try MAXPATHLEN,
 * failing that we hardcode it to 1024.
 */
#if defined(PATH_MAX)
#   define SQSH_MAXPATH    PATH_MAX
#else
#   if defined(MAXPATHLEN)
#      define SQSH_MAXPATH   MAXPATHLEN
#   else
#      define SQSH_MAXPATH   1024
#   endif
#endif

/*
 * Since sqsh uses setjmp/longjmp to return from signal handlers,
 * the following will cause the "signal-safe" versions of setjmp
 * and longjmp to be used, if they are available.
 */
#if defined(HAVE_SIGSETJMP)
#define  SETJMP(b)      sigsetjmp(b,1)
#define  LONGJMP(b,v)   siglongjmp(b,v)
#define  JMP_BUF        sigjmp_buf
#else
#define  SETJMP(b)      setjmp(b)
#define  LONGJMP(b,v)   longjmp(b,v)
#define  JMP_BUF        jmp_buf
#endif

/*
 * This file cannot be included until this point, because it requires
 * _ANSI_ARGS() for its work.
 */
#include "sqsh_compat.h"
#include "sqsh_debug.h"

#if defined(NO_DB)
#define   DBPROCESS     void
#endif

#endif /* sqsh_config_h_included */
