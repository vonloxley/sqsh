/*
 * sqsh_global.h - Where globals are defined
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
#ifndef sqsh_global_h_included
#define sqsh_global_h_included
#include <ctpublic.h>

#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "sqsh_job.h"
#include "sqsh_varbuf.h"
#include "sqsh_history.h"
#include "sqsh_alias.h"
#include "dsp.h"
#include "sqsh_func.h"

#include "config.h"

#if defined(HAVE_LOCALE_H)
    #include <locale.h>
#endif

/* g_cs_ver: This is the value of CS_VERSION_xxx. Stored in a global
 *          because it is needed to figure out the correct BLK_VERSION_xxx
 *          value to use in cmd_blk.c
 */
extern CS_INT         g_cs_ver;

/*
 * g_context: This represents this process's connection to the database,
 *           it is possible for this to be a NULL pointer, for example
 *           when a job is run in the background, g_dbproc is initialized
 *           to NULL within the context of the child process.
 */
extern CS_CONTEXT     *g_context;
extern CS_CONNECTION  *g_connection;

/*
 * g_env:    This is the current set of environment variables in use by
 *           the current process. This structure is inherited between
 *           children.
 */
extern env_t       *g_env ;

/*
 * g_internal_env:   This environment is considered the "internal" env-
 *           ironment used to store variables that are available for
 *           expansion to the user but do not show up during a \set command.
 *           And, are not settable by the user.
 */
extern env_t       *g_internal_env ;

/*
 * g_do_cols: This is a stack of column descriptions that is
 *           used for column value expansion during a \do loop.
 */
extern dsp_desc_t  *g_do_cols[];
extern int          g_do_ncols;

/*
 * g_func_args: This is a stack of function arguments for each function
 *            currently being called.
 */
extern funcarg_t    g_func_args[];
extern int          g_func_nargs;

/*
 * g_buf :    This structure is simply another instance of an
 *            environment (like g_env), except that it is used to
 *            hold named buffers.
 */
extern env_t       *g_buf ;

/*
 * g_sqlbuf: This is the buffer containing the current SQL query to
 *           be executed by the database. Note, this buffer does *not*
 *           contain the expanded version (where all $variables are
 *           expanded), it is the responsibility of those commands
 *           that need this buffer expanded to maintain thier own
 *           copies.
 */
extern varbuf_t   *g_sqlbuf ;

/*
 * g_cmdset: This is the set of commands available for execution within
 *           the sqsh shell (such as \go, \exit, \reset, \set, etc.). These
 *           commands may be run within a jobset.
 */
extern cmdset_t   *g_cmdset ;

/*
 * g_funcset: Set of user defined functions. These are an
 *           AVL tree of func_t's.
 */
extern funcset_t  *g_funcset;

/*
 * g_alias:  The set of aliases available for particular commands. These
 *           are used by jobset_run() to expand the command line of
 *           alias's prior to execution.
 */
extern alias_t    *g_alias ;

/*
 * g_jobset: This is the current list of background jobs running within
 *           the current process.  This structure is re-initialized within
 *           the context of a child process by sqsh_fork().
 */
extern jobset_t   *g_jobset ;

/*
 * g_history: The complete history of commands that have been executed
 *            within sqsh.  Typically this structure is only altered
 *            within cmd_loop.c, however other commands are allowed to
 *            manipulate it.
 */
extern history_t  *g_history ;

/*
 * g_version:   Current version.
 * g_copyright: This current copyright banner.  This is useful for having
 *              a central location in which to change the version number.
 */
extern char       *g_copyright ;
extern char       *g_version ;

/*
 * g_password & g_lock: Contains the current value of the regular database
 *              password and the session lock password.
 */
extern int         g_password_set;
extern char       *g_password;
extern char       *g_lock;

/*
 * g_interactive: Global indicator if sqsh was started in interactive mode.
 */
extern int         g_interactive;

/*
 * g_lconv: sqsh-2.3 - Do locale conversion of numeric/decimal/money datatypes.
 */
#if defined(HAVE_LOCALE_H)
    extern struct lconv *g_lconv;
#else
    extern void *g_lconv;
#endif

/*
 * sqsh-2.5 - New feature: Print to file from message handler after $p2faxm
 * number of printed messages to screen.
 */
extern FILE       *g_p2f_fp; /* Print to file filepointer */
extern int         g_p2fc;   /* Print to file count */

#if !defined(CS_NULLDATA)
#error cspublic.h not included!
#endif

/* newer FreeTDS versions defines CS_TDS_72, old ones use enumeration but not defines */
#undef SQSH_FREETDS
#if defined(CS_TDS_72) || !defined(CS_TDS_50)
#define SQSH_FREETDS 1
#endif

#endif
