/*
 * cmd_return.c - Return out of command.
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
#include "sqsh_cmd.h"
#include "sqsh_env.h"
#include "sqsh_global.h"
#include "cmd.h"

/*
 * cmd_return():
 *
 * Tells current cmd_input() to return up to the parent caller.
 */
int cmd_return( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	int r;
	char nbr[10];

	if (argc < 2)
	{
		env_set( g_internal_env, "?", "0" );
	}
	else
	{
		r = atoi(argv[1]);
		sprintf( nbr, "%d", r );
		env_set( g_internal_env, "?", nbr );
	}
	return CMD_RETURN;
}

/*
 * cmd_break():
 *
 * Break out of loop.
 */
int cmd_break( argc, argv )
	int    argc ;
	char  *argv[] ;
{
	return CMD_BREAK;
}
