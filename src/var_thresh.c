/*
 * var_thresh.c - Validation functions for threshold variables
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

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: var_thresh.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * var_set_severity()
 *
 * Validation function for setting minimum severity level.
 */
int var_set_severity( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	int  severity ;

	/*-- Make sure we have an int --*/
	if( var_set_int( env, var_name, var_value ) == False )
		return False ;

	/*-- Convert it to an integer --*/
	severity = atoi( *var_value ) ;

	/*-- Verify the range --*/
	if( severity < 0 || severity > 22 ) {
		sqsh_set_error( SQSH_E_INVAL,
							 "Severity level must be between 0 and 22" ) ;
		return False ;
	}

	return True ;
}
