/*
 * cmd_lock.c - Lock the current session
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
#include <pwd.h>
#include "sqsh_config.h"
#include "sqsh_cmd.h"
#include "sqsh_global.h"
#include "sqsh_error.h"
#include "cmd.h"

#if defined(HAVE_SHADOW_H)
#include <shadow.h>
#endif

#if defined(HAVE_CRYPT_H)
#include <crypt.h>
#endif

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_lock.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

int cmd_lock( argc, argv )
	int    argc ;
	char  *argv[] ;
{
#if defined(HAVE_SHADOW_H)
	struct spwd   *spwd;
#endif
	struct passwd *passwd;
	char          salt[2];
	char          pass[25];
	char         *crypt_pass;
	char         *real_pass = NULL;
	int           len;

	/*
	 * Retrieve the password information about the current user.
	 */
	passwd = getpwuid( getuid() );

	if (passwd == NULL) {
		fprintf( stderr, "\\lock: Unable to get login entry from /etc/passwd\n" );
		return CMD_FAIL;
	}

	/*
	 * If the locking password has been set, then use it instead of the
	 * password contained in the password file.
	 */
	if (g_lock == NULL) {

#if !defined(HAVE_CRYPT)
		fprintf( stderr, "\\lock: Crypt(3C) support not compiled into sqsh\n" );
		fprintf( stderr, "\\lock: You must set $lock to assign your password\n" );
		return CMD_FAIL;
#else

		real_pass = passwd->pw_passwd;

#if defined(HAVE_SHADOW_H)
		/*
		 * If the passwd->pw_passwd entry is undefined or contains an invalid
		 * password (e.g. "x"), the we are probably using shadow passwords,
		 * so lets give that a try.
		 */
		if (real_pass == NULL || strlen(real_pass) < 2) {
			if ((spwd = getspnam( passwd->pw_name )) == NULL) {
				fprintf( stderr, "\\lock: Unable to get shadow password\n" );
				return CMD_FAIL;
			}
			real_pass = spwd->sp_pwdp;
		}
#endif /* HAVE_SHADOW_H */

#endif /* !HAVE_CRYPT */

	}

	printf( "sqsh: This session has been locked by user '%s'\n",
	         passwd->pw_name );

	for (;;) {
		len = sqsh_getinput( "Password: ", pass, sizeof(pass), 0 );

		if (len < 0)
		{
			/*
			 * If len == -2 then we received some sort of interrupt
			 * while waiting on input from the user, so we want to
			 * start all over.
			 */
			if (len == -2)
			{
				continue;
			}

			if (len == -1)
			{
				fprintf( stderr, "\n\\lock Unable to read input: %s\n",
					sqsh_get_errstr() );
			}
			return CMD_FAIL;
		}

		if (pass[len-1] == '\n')
			pass[len-1] = '\0';

		if (g_lock != NULL) {
			if (strcmp(pass, g_lock) == 0)
				break;
			else 
				fprintf( stderr, "sqsh: Invalid password.\n" );
		} else {

			/*
			 * If there is no password defined at all for this user, then
			 * simply return.
			 */
			if (real_pass == NULL || *real_pass == '\0')
				return CMD_LEAVEBUF;

#if defined(HAVE_CRYPT)
			salt[0] = real_pass[0];
			salt[1] = real_pass[1];

			crypt_pass = (char*)crypt( pass, salt );

			if (strcmp( real_pass, crypt_pass ) != 0) {
				fprintf( stderr, "sqsh: Invalid password.\n" );
			} else
				break;
#endif /* HAVE_CRYPT */

		}
	} 

	return CMD_LEAVEBUF;
}
