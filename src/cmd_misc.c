/*
 * cmd_misc.c - Miscellaneous functions share between user commands
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
#include "sqsh_expand.h"
#include "sqsh_error.h"
#include "sqsh_varbuf.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "cmd.h"
#include "cmd_misc.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_misc.c,v 1.2 2009/04/14 10:02:54 mwesdorp Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * cmd_display():
 *
 * This function is used by various user-level commands to force the
 * contents of the sql buffer to be displayed to stdout, including
 * the prompt.
 */
int cmd_display( sqlbuf )
	varbuf_t  *sqlbuf ;
{
	char      *buf, *end ;
	varbuf_t  *expand_buf = NULL ;
	char      *prompt ;
	char      *s ;

	env_get( g_env, "prompt", &prompt ) ;

	/*
	 * Create a buffer into which we can expand the contents
	 * of the prompt.
	 */
	if( prompt != NULL && *prompt != '\0' ) {
		if( (expand_buf = varbuf_create( 512 )) == NULL ) {
			fprintf( stderr, "cmd_display: %s\n", sqsh_get_errstr() ) ;
			return False ;
		}
	}

	buf = varbuf_getstr( sqlbuf ) ;
	env_set( g_env, "lineno", NULL ) ;    /* reset */
	while( (end = strchr( buf, '\n' )) != NULL ) {

		if( expand_buf != NULL ) {
			env_set( g_env, "lineno", "1" ) ;

			/*-- Attempt to expand the prompt --*/
			if( (sqsh_expand( prompt, expand_buf, 0 )) == False ) {
				fprintf( stderr, "prompt: %s\n", sqsh_get_errstr() ) ;
				varbuf_strcpy( expand_buf, "Bad Prompt> " ) ;
			}
			s = varbuf_getstr( expand_buf ) ;
		} else
			s = "" ;

                /*-- sqsh-2.1.6 feature - Expand color prompt --*/
		s = expand_color_prompt (s, False);

		/*-- Print the line of text --*/
		fprintf( stdout, "%s%*.*s\n", 
					s,                             /* Prompt */
					(int) (end - buf),             /* Length of line */
					(int) (end - buf),             /* Length of line */
					buf ) ;                        /* Line itself */
		fflush( stdout ) ;

		buf = end + 1 ;
	}

	/*-- We are working on the next line --*/
	env_set( g_env, "lineno", "1" ) ;

	if( expand_buf != NULL )
		varbuf_destroy( expand_buf ) ;

	return True ;
}
