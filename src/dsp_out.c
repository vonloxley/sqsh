/*
 * dsp_out.c - Signal-safe buffered output display functions.
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
#include "sqsh_error.h"
#include "sqsh_global.h"
#include "sqsh_debug.h"
#include "dsp.h"

extern int errno;

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: dsp_out.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

dsp_out_t* dsp_fopen( f )
	FILE  *f;
{
	dsp_out_t   *o;

	o = (dsp_out_t*)malloc(sizeof(dsp_out_t));

	if (o == NULL)
	{
		fprintf( stderr, "dsp_fopen: Memory allocation failure\n" );
		return NULL;
	}

	/*
	 * Make sure that the FILE* is flushed before we steal its
	 * file descriptor.
	 */
	fflush( f );

	o->o_fd   = fileno( f );
	o->o_file = f;
	o->o_nbuf = 0;

	return o;
}

int dsp_fclose( o )
	dsp_out_t  *o;
{
	dsp_fflush( o );
	clearerr( o->o_file );
	free( o );

	return 0;
}

int dsp_fputc( c, o )
	int         c;
	dsp_out_t  *o;
{
	o->o_buf[o->o_nbuf++] = c;

	if (c == '\n' || o->o_nbuf == DSP_BUFSIZE)
	{
		return dsp_fflush( o );
	}

	return 0;
}

int dsp_fputs( s, o )
	char       *s;
	dsp_out_t  *o;
{
	while (*s != '\0')
	{
		o->o_buf[o->o_nbuf++] = *s;

		if (*s == '\n' || o->o_nbuf == DSP_BUFSIZE)
		{
			if (dsp_fflush( o ) == -1)
			{
				return -1;
			}
		}
		++s;
	}

	return 0;
}

#if defined(__ansi__)
int dsp_fprintf( dsp_out_t *o, char *fmt, ...)
#else
int dsp_fprintf( o, fmt, va_alist )
	dsp_out_t  *o;
	char       *fmt;
	va_dcl
#endif
{
	va_list     ap;
	int         nbytes;

	dsp_fflush( o );

#if defined(__ansi__)
	va_start( ap, fmt );
#else
	va_start( ap );
#endif

	o->o_nbuf = vsprintf( o->o_buf, fmt, ap );
	va_end( ap );

	nbytes = o->o_nbuf;
	dsp_fflush( o );

	return nbytes;
}

int dsp_fflush( o )
	dsp_out_t   *o;
{
	char   *cp;
	int     nbytes;
	int     r;

	cp     = o->o_buf;
	nbytes = o->o_nbuf;

	/*
	 * Just in case someone has been using the *real* FILE
	 * structure, lets make sure that we flush it as well.
	 */
	fflush( o->o_file );

	while (nbytes > 0)
	{
		r = write(o->o_fd, (void*)cp, nbytes);

		if (r == -1)
		{
			if (errno != EINTR)
			{
				return -1;
			}
		}
		else
		{
			nbytes -= r;
			cp     += r;
		}
	}

	o->o_nbuf = 0;
	return 0;
}
