/*
 * var_readline.c - Variables specific to controlling readline
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
#include "sqsh_env.h"
#include "sqsh_error.h"
#include "var.h"

#if defined(USE_READLINE)
#include <readline/readline.h>
extern void stifle_history();
extern void unstifle_history();
#endif

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: var_readline.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

int var_set_rl_histsize( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	int  stifle_value;

	if (var_set_nullint( env, var_name, var_value ) == False)
	{
		return False;
	}

	stifle_value = atoi(*var_value);

#if defined(USE_READLINE)
	if (stifle_value <= 0)
	{
		unstifle_history();
	}
	else
	{
		stifle_history( stifle_value ); 
	}
#endif
	return True;
}

/*
 * var_set_completion()
 *
 * Sets the value of tab expansion completion.  Where valid values are
 * none=0, lower=1, upper=2, and smart=3.
 */
int var_set_completion( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	static char *errstr = 
		"Values must be 0=none, 1=lower, 2=upper, 3=smart, or 4=exact";
	int   d ;

	/*
	 * Perform simple validations.
	 */
	if (var_value == NULL || *var_value == NULL)
	{
		sqsh_set_error( SQSH_E_INVAL, errstr );
		return False;
	}

	/*
	 * Allow this variable to be set using the numerical representation
	 * of the setting.
	 */
	if (isdigit((int)**var_value))
	{
		if (var_set_int( env, var_name, var_value ) == False)
			return False;
		
		d = atoi(*var_value);

		if (d < 0 || d > 4)
		{
			sqsh_set_error( SQSH_E_INVAL, errstr );
			return False;
		}
	}
	else
	{
		if (strcasecmp( *var_value, "none" ) == 0)
			d = 0;
		else if (strcasecmp( *var_value, "lower" ) == 0)
			d = 1;
		else if (strcasecmp( *var_value, "upper" ) == 0)
			d = 2;
		else if (strcasecmp( *var_value, "smart" ) == 0)
			d = 3;
		else if (strcasecmp( *var_value, "exact" ) == 0)
			d = 4;
		else
		{
			sqsh_set_error( SQSH_E_INVAL, errstr );
			return False;
		}
	}

	/*
	 * Set the contents of the variable value to be the number
	 * representation of whatever the user has chosen.
	 */
	sprintf( *var_value, "%d", d );
	return True;
}
