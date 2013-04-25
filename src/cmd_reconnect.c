/*
 * cmd_reconnect.c - User command to force reconnect to database
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
#include "sqsh_cmd.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_reconnect.c,v 1.2 2013/04/04 10:52:35 mwesdorp Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_reconnect():
 *
 * Re-establishes a connection the database, closing the prior
 * connection.  This allows, essentially, an 'su' for databases.
 *
 * sqsh-2.2.0 - Also save the current context and let ct_connect setup
 * a new context. In case of success we can drop the original context,
 * otherwise restore the original context.
 */
int cmd_reconnect( argc, argv )
	int     argc ;
	char   *argv[] ;
{
	CS_CONNECTION *old_connection;
	CS_CONTEXT    *old_context;

	old_connection = g_connection;
	old_context    = g_context;
	g_connection   = NULL;
	g_context      = NULL;

	if( cmd_connect( argc, argv ) == CMD_FAIL ) 
	{
		g_connection = old_connection ;
		g_context    = old_context ;
		return CMD_FAIL ;
	}

	if (ct_close( old_connection, CS_UNUSED ) != CS_SUCCEED)
  	    ct_close( old_connection, CS_FORCE_CLOSE );
	ct_con_drop( old_connection );

	if (ct_exit( old_context, CS_UNUSED ) != CS_SUCCEED)
		ct_exit( old_context, CS_FORCE_EXIT );
	cs_ctx_drop( old_context );

	return CMD_LEAVEBUF ;
}
