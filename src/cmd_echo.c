/*
 * cmd_echo.c - User command to echo arguments to screen
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
static char RCS_Id[] = "$Id: cmd_echo.c,v 1.2 2001/10/24 17:47:36 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_echo:
 */
int cmd_echo( argc, argv )
	int    argc;
	char  *argv[];
{
	int i;
	int lf = True;
	int count = 0;

	if (argc == 1)
	{
		printf( "\n" );
		return CMD_LEAVEBUF;
	}

	if (strcmp(argv[1], "-n") == 0)
	{
		lf = False;
		i = 2;
	}
	else
	{
		i = 1;
	}

	for (; i < argc; i++)
	{
		if (count++ == 0)
			printf( "%s", argv[i] );
		else
			printf( " %s", argv[i] );
	}

	if (lf)
	{
		printf( "\n" );
	}

	return CMD_LEAVEBUF;
}
