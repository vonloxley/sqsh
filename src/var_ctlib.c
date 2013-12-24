/*
 * var_dbproc.c - Variables that affect the global CS_CONNECTION
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
#include <ctype.h>
#include "sqsh_config.h"
#include "sqsh_global.h"
#include "sqsh_env.h"
#include "sqsh_error.h"
#include "sqsh_fd.h"
#include "var.h"
#include "sqsh_expand.h" /* sqsh-2.1.6 */

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: var_ctlib.c,v 1.2 2009/04/14 10:22:18 mwesdorp Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

int var_set_interfaces( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	/* sqsh-2.1.6 - New variables */
	varbuf_t  *exp_buf;
	char      *interfaces;


	if (var_value == NULL || *var_value == NULL)
	{
		sqsh_set_error( SQSH_E_INVAL, "Invalid path name" );
		return False;
	}

	/*
	 * sqsh-2.1.6 feature - Expand interfaces variable
	*/
	if ((exp_buf = varbuf_create( 512 )) == NULL)
	{
		sqsh_set_error( SQSH_E_INVAL, "Unable to allocate expand buffer" );
		return False;
	}
	if (sqsh_expand( *var_value, exp_buf, 0 ) == False)
	{
		sqsh_set_error( SQSH_E_INVAL, "Unable to expand interfaces variable" );
		return False;
	}
	interfaces = varbuf_getstr( exp_buf );


	if (access( interfaces, R_OK ) == -1)
	{
		sqsh_set_error( SQSH_E_INVAL, "Illegal path: %s", strerror(errno) );
		varbuf_destroy( exp_buf );
		return False;
	}

	if (g_context != NULL)
	{
		if (ct_config( g_context,             /* Context */
		               CS_SET,                /* Action */
		               CS_IFILE,              /* Property */
		               (CS_VOID*)interfaces,  /* Buffer */
		               CS_NULLTERM,           /* Buffer Length */
		               NULL ) != CS_SUCCEED )
		{
			sqsh_set_error( SQSH_E_INVAL, "Unable to configure CS_CONTEXT" );
			varbuf_destroy( exp_buf );
			return False;
		}
	}
	varbuf_destroy( exp_buf ); /* sqsh-2.1.6 feature */
	return True ;
}

int var_set_packet( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	int packet_size;

	if (var_set_nullint( env, var_name, var_value ) == False)
	{
		return False;
	}

	if (*var_value == NULL)
		return True;

	packet_size = atoi(*var_value);

	if (packet_size <= 0 || (packet_size % 512) != 0)
	{
		sqsh_set_error( SQSH_E_INVAL,
			"Invalid packet size. Must be a multiple of 512 bytes" );
		return False;
	}

	return True;
}
