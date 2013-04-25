/*
 * var_debug.c - Parser for setting debugging level
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
#include "sqsh_global.h"
#include "var.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: var_debug.c,v 1.3 2013/04/18 11:54:43 mwesdorp Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

#if defined(DEBUG)

/*
 * The following table maps a string supplied for the $debug
 * variable into a bitflag.
 */
struct debug_st {
	char  *d_name ;        /* Name of debug flag */
	int    d_flag ;        /* Which flag name indicates */
} debug_list[] = {
	{ "ALIAS",                DEBUG_ALIAS   },
	{ "ALL",                  DEBUG_ALL     },
	{ "AVL",                  DEBUG_AVL     },
	{ "BCP",                  DEBUG_BCP     },
	{ "DISPLAY",              DEBUG_DISPLAY },
	{ "ENV",                  DEBUG_ENV     },
	{ "ERROR",                DEBUG_ERROR   },
	{ "EXPAND",               DEBUG_EXPAND  },
	{ "FD",                   DEBUG_FD      },
	{ "HISTORY",              DEBUG_HISTORY },
	{ "HIST",                 DEBUG_HISTORY },
	{ "JOB",                  DEBUG_JOB     },
	{ "READLINE",             DEBUG_READLINE},
	{ "RL",                   DEBUG_READLINE},
	{ "RPC",                  DEBUG_RPC     },
	{ "SCREEN",               DEBUG_SCREEN  },
	{ "SIG",                  DEBUG_SIG     },
	{ "SIGCHLD",              DEBUG_SIGCLD  },
	{ "SIGCLD",               DEBUG_SIGCLD  },
	{ "TDS",                  DEBUG_TDS     },
} ;

#endif

/*
 * var_set_debug()
 *
 * Allows you to turn on debugging flags by name rather than
 * by value, such as DEBUG_ENV|DEBUG_FD|DEBUG_EXPAND.
 */
int var_set_debug( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{

#if !defined(DEBUG)
	sqsh_set_error( SQSH_E_INVAL, "sqsh not compiled with -DDEBUG" ) ;
	return False ;
#else
	static  char debug_number[8] ;
	int     debug_flags = 0 ;
	char    flag_name[40] ; 
	char   *str ;
	int     i ;
	int     nitems ;
	char    error_msg[256];

	if( *var_value == NULL || strcmp(*var_value, "NULL") == 0 ) {
		sqsh_debug_level( 0 ) ;
		*var_value="0" ;
		return True ;
	}

	nitems = sizeof( debug_list ) / sizeof( struct debug_st ) ;

	str = *var_value ;
	while( *str != '\0' ) {

		/*-- Skip leading white space --*/
		for( ; *str != '\0' && isspace( (int) *str); ++str ) ;

		for( i = 0; *str != '\0' && !(isspace( (int) *str)) && *str != '|'; ++str ) {
			if( i < sizeof(flag_name) )
				flag_name[i++] = *str ;
		}
		flag_name[i] = '\0' ;

		if( isspace( (int) *str) ) {
			/*-- Skip leading white space --*/
			for( ++str; *str != '\0' && isspace( (int) *str); ++str ) ;
			
			if( !(*str == '\0' || *str == '|') ) {
				sqsh_set_error( SQSH_E_INVAL,
					"Expected a '|' separating debugging levels" ) ;
				return False ;
			}

		}
		if( *str != '\0' )
			++str ;
		
		if( isdigit( (int) *flag_name) )
			debug_flags |= atoi(flag_name) ;
		else {
			for( i = 0; i < nitems; i++ ) {
				if( strcmp( flag_name, debug_list[i].d_name ) == 0 )
					break ;
			}
			if( i == nitems ) {
				sprintf( error_msg, "Valid values are: %s", debug_list[0].d_name );
				for (i = 1; i < nitems; i++) {
					strcat( error_msg, ", " );
					strcat( error_msg, debug_list[i].d_name );
				}
				sqsh_set_error( SQSH_E_INVAL, error_msg );
				return False ;
			}
			debug_flags |= debug_list[i].d_flag ;
		}
	}

	if( sqsh_debug_level( debug_flags ) == False ) {
		sqsh_set_error( SQSH_E_INVAL, sqsh_get_errstr() ) ;
		return False ;
	}

	sprintf( debug_number, "%3d", debug_flags ) ;
	*var_value = debug_number ;
	return True ;
#endif
}
