/*
 * var_passwd.c - Password setting variable functions
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
#include "sqsh_env.h"
#include "sqsh_global.h"
#include "sqsh_error.h"
#include "var.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: var_passwd.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

int var_set_lock( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	char *new_lock;

	/*
	 * Allow them to set the string to a null value.
	 */
	if ( *var_value == NULL || strcmp(*var_value,"NULL") == 0 ) {

		/*-- Free up the old value --*/
		if (g_lock != NULL)
			free( g_lock );

		g_lock = NULL ;
#if !defined(INSECURE)
		*var_value = "*lock*";
#endif
		return True ;
	}

	if ((new_lock = sqsh_strdup(*var_value)) == NULL) {
		sqsh_set_error(SQSH_E_NOMEM, NULL);
		return False;
	}

	if (g_lock != NULL)
		free( g_lock );
	g_lock = new_lock;

	/*
	 * Regardless what they set the lock password to, it will show
	 * a value of *lock*.
	 */
#if !defined(INSECURE)
	*var_value = "*lock*";
#endif

	return True ;
}

int var_set_password( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	char *new_pass;

	/*-- Get rid of the old password --*/
	if (g_password != NULL)
	{
		g_password_set = False;
		free( g_password );
		g_password     = NULL;
	}

	/*
	 * If the variable's value is "", then we want to leave the
	 * password marked at unset.
	 */
	if (*var_value != NULL && **var_value == '\0')
	{
		return True;
	}

	/*
	 * Allow them to set the string to a null value.
	 */
	if ( *var_value == NULL || strcmp(*var_value,"NULL") == 0 ) {

		if (*var_value != NULL)
		{
			g_password_set = False;
		}
		else
		{
			g_password_set = True;
		}

		g_password     = NULL ;

#if !defined(INSECURE)
		*var_value     = "*password*";
#endif
		return True ;
	}

	if ((new_pass = sqsh_strdup(*var_value)) == NULL) {
		sqsh_set_error(SQSH_E_NOMEM, NULL);
		return False;
	}

	/*
	 * Regardless what they set the lock password to, it will show
	 * a value of *lock*.
	 */
	g_password = new_pass;
#if !defined(INSECURE)
	*var_value = "*password*";
#endif
	g_password_set = True;

	return True;
}
