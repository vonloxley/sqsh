/*
 * sqsh_global.c - Where globals are defined
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
#include <stdio.h>
#include "sqsh_config.h"
#include "sqsh_global.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_global.c,v 1.1.1.1 2004/04/07 12:35:05 chunkm0nkey Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Global Initialization --*/
CS_CONTEXT         *g_context    = NULL;
CS_CONNECTION      *g_connection = NULL;

CS_INT      g_cs_ver       = CS_VERSION_100;

env_t      *g_env          = NULL;
env_t      *g_buf          = NULL;
env_t      *g_internal_env = NULL;
varbuf_t   *g_sqlbuf       = NULL;
cmdset_t   *g_cmdset       = NULL;
funcset_t  *g_funcset      = NULL;
jobset_t   *g_jobset       = NULL;
history_t  *g_history      = NULL;
alias_t    *g_alias        = NULL;
char       *g_password     = NULL;
int         g_password_set = False;
char       *g_lock         = NULL;
char       *g_copyright    = "Copyright (C) 1995-2001 Scott C. Gray";
char       *g_version      = SQSH_VERSION;
dsp_desc_t *g_do_cols[64];
int         g_do_ncols     = 0;
funcarg_t   g_func_args[64];
int         g_func_nargs   = 0;  
