/*
 * var_date.c - Date/Time variable set/retrieval functions
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
#include "sqsh_error.h"
#include "sqsh_global.h"
#include "dsp.h"
#include "var.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: var_date.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * var_get_date()
 */
int var_get_date( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	static char date_str[32] ;  /* Hopefully large enough */
	char       *date_fmt ;
	time_t      cur_time ;

	/*-- Retrieve the current time --*/
	time( &cur_time ) ;

	date_fmt = *var_value ;
	if( date_fmt == NULL || *date_fmt == '\0' )
		date_fmt = "%d-%b-%y" ;

	cftime( date_str, date_fmt, &cur_time ) ;

	*var_value = date_str ;
	return True ;
}

/*
 * var_get_time()
 */
int var_get_time( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	static char time_str[32] ;  /* Hopefully large enough */
	char       *time_fmt ;
	time_t      cur_time ;

	/*-- Retrieve the current time --*/
	time( &cur_time ) ;

	time_fmt = *var_value ;
	if( time_fmt == NULL || *time_fmt == '\0' )
		time_fmt = "%H:%M:%S" ;

	cftime( time_str, time_fmt, &cur_time ) ;

	*var_value = time_str ;
	return True ;
}
