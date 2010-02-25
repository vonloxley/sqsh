/*
 * sqsh_stdin.c - Steaming data source.
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
#include <sys/stat.h>
#include "sqsh_config.h"
#include "sqsh_sigcld.h"
#include "sqsh_env.h"
#include "sqsh_global.h"
#include "sqsh_error.h"
#include "sqsh_stdin.h"

/*
** MAX_STDIN_STACK: The number of data sources we can have
** pushed away at any given time.
*/
#define MAX_STDIN_STACK  32

typedef enum
{
    STDIN_FILE,
    STDIN_BUFFER
}
stdin_type_e;

typedef struct _stdin_st
{
    stdin_type_e  stdin_type;     /* Type of input */
    int           stdin_isatty;   /* Is this a tty? */
    FILE         *stdin_file;     /* File input */
    char         *stdin_buf;      /* Buffer input */
    char         *stdin_buf_cur;  /* CUrrent pointer int buffer */
    char         *stdin_buf_end;  /* Pointer to last byte of input */
    int           stdin_buf_read; /* Amount processed */
}
stdin_t;


/*
** Static globals.
*/
static stdin_t sg_stdin_stack[MAX_STDIN_STACK];
static int      sg_stdin_cur = 0;

/*
** sqsh_stdin_buffer():
**
** Set the current input source as a buffer (string).
*/
int sqsh_stdin_buffer( buf, len )
    char   *buf;
    int     len;
{
    if (sg_stdin_cur == MAX_STDIN_STACK-1)
    {
        sqsh_set_error(SQSH_E_RANGE, 
            "sqsh_stdin_buffer: STDIN buffer stack full" );
        return(-1);
    }
    sg_stdin_stack[sg_stdin_cur].stdin_type     = STDIN_BUFFER;
    sg_stdin_stack[sg_stdin_cur].stdin_isatty   = 0;
    sg_stdin_stack[sg_stdin_cur].stdin_buf      = buf;
    sg_stdin_stack[sg_stdin_cur].stdin_buf_cur  = buf;

    if (len >= 0)
    {
        sg_stdin_stack[sg_stdin_cur].stdin_buf_end = (buf + len);
    }
    else
    {
        sg_stdin_stack[sg_stdin_cur].stdin_buf_end = NULL;
    }

    ++sg_stdin_cur;

    return(0);
}

/*
** sqsh_stdin_file():
**
** Set the current input source as a file (string).
*/
int sqsh_stdin_file( file )
    FILE  *file;
{
    if (sg_stdin_cur == MAX_STDIN_STACK-1)
    {
        sqsh_set_error(SQSH_E_RANGE, 
            "sqsh_stdin_file: STDIN buffer stack full" );
        return(-1);
    }
    sg_stdin_stack[sg_stdin_cur].stdin_type     = STDIN_FILE;
    sg_stdin_stack[sg_stdin_cur].stdin_isatty   = isatty(fileno(file));
    sg_stdin_stack[sg_stdin_cur].stdin_file     = file;
    ++sg_stdin_cur;

    return(0);
}

/*
** sqsh_stdin_pop()
**
** Restore the previous stdin source.
*/
int sqsh_stdin_pop()
{
    if (sg_stdin_cur > 0)
        --sg_stdin_cur;
    return(0);
}

int sqsh_stdin_isatty()
{
    stdin_t   *sin;

    if (sg_stdin_cur == 0)
    {
        return(False);
    }

    sin = &(sg_stdin_stack[sg_stdin_cur-1]);

    if (sin->stdin_isatty)
    {
        return(True);
    }
    return(False);
}

/*
** sqsh_stdin_fgets():
**
** Read a string from stdin.
*/
char* sqsh_stdin_fgets( buf, len )
    char    *buf;
    int      len;
{
    stdin_t   *sin;
    char      *str;
    char      *dptr;
    char      *eptr;

    if (sg_stdin_cur == 0)
    {
        sqsh_set_error(SQSH_E_RANGE, 
            "sqsh_stdin_file: STDIN has been closed" );
        return((char*)NULL);
    }

    sin = &(sg_stdin_stack[sg_stdin_cur-1]);

    if (sin->stdin_type == STDIN_FILE)
    {
        if (sin->stdin_isatty) {
            while (1) {
                str = fgets( buf, len, sin->stdin_file );
                if (str != NULL || errno != EINTR) break;
            }
        }
        else
            str = fgets( buf, len, sin->stdin_file );

        if (str == NULL)
        {
            if (feof( sin->stdin_file ))
            {
                sqsh_set_error( SQSH_E_NONE, NULL );
            }
            else
            {
                sqsh_set_error( SQSH_E_RANGE, "%s", strerror(errno) );
            }
        }

        return(str);
    }


    dptr = buf;
    eptr = buf + len;

    if (sin->stdin_buf_end == NULL)
    {
        if (*sin->stdin_buf_cur == '\0')
        {
            sqsh_set_error( SQSH_E_NONE, NULL );
            return((char*)NULL);
        }

        while (*(sin->stdin_buf_cur) != '\0' && 
            dptr < eptr)
        {
            *dptr++ = *sin->stdin_buf_cur;

            if (*sin->stdin_buf_cur == '\n')
            {
                ++sin->stdin_buf_cur;
                break;
            }

            ++sin->stdin_buf_cur;
        }
    }
    else
    {
        if (sin->stdin_buf_cur == sin->stdin_buf_end)
        {
            sqsh_set_error( SQSH_E_NONE, NULL );
            return((char*)NULL);
        }

        while (sin->stdin_buf_cur < sin->stdin_buf_end && 
            dptr < eptr)
        {
            if (*sin->stdin_buf_cur == '\n')
            {
                ++sin->stdin_buf_cur;
                break;
            }

            ++sin->stdin_buf_cur;
        }
    }

    if (dptr < (eptr-1))
    {
        *dptr = '\0';
    }

    return(buf);
}

