/*
 * sqsh_varbuf.c - dynamic string buffer
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
#include "sqsh_varbuf.h"
#include "sqsh_error.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_varbuf.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- local prototypes --*/
static int  varbuf_grow      _ANSI_ARGS(( varbuf_t *varbuf, int add_size )) ;

/*
 * PUBLIC FUNCTIONS
 */
varbuf_t* varbuf_create( growsize )
	int growsize ;
{
	varbuf_t *newbuf ;

	if( growsize < 1 ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return (varbuf_t*)NULL ;
	}

	newbuf = (varbuf_t*) malloc(sizeof(varbuf_t)) ;

	if( newbuf == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return (varbuf_t*)NULL ;
	}

	newbuf->vb_growsize   = growsize ;
	newbuf->vb_tot_length = growsize ;
	newbuf->vb_cur_length = 0 ;
	newbuf->vb_buffer = (char*) malloc(sizeof(char)*growsize) ;

	if( newbuf->vb_buffer == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		free( newbuf ) ;
		return (varbuf_t*)NULL ;
	}

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return newbuf ;
}

int varbuf_destroy( varbuf )
	varbuf_t *varbuf ;
{
	if( varbuf == NULL ) {
		return -1 ;
	}
		
	free(varbuf->vb_buffer) ;
	free(varbuf) ;

	return 1 ;
}

int varbuf_ncat( varbuf, buf, nbytes )
	varbuf_t *varbuf ;
	char     *buf ;
	int      nbytes ;
{
	if( varbuf == NULL || buf == NULL || nbytes < 0 ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	if ( varbuf_grow(varbuf, nbytes) == False ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return -1 ;
	}
	
	memcpy(varbuf->vb_buffer + varbuf->vb_cur_length, buf, nbytes) ;
	varbuf->vb_cur_length += nbytes ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return nbytes ;
}

int varbuf_clear( varbuf )
	varbuf_t *varbuf ;
{
	if( varbuf == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	varbuf->vb_cur_length = 0 ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return 0 ;
}

int varbuf_strcat( varbuf, str )
	varbuf_t *varbuf; 
	char *str ;
{
	int len ;
	char *cp;

	if( varbuf == NULL || str == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	len = strlen(str) ;
	if( varbuf_grow( varbuf, len ) == False ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return -1 ;
	}

	cp = varbuf->vb_buffer + varbuf->vb_cur_length;

	while (*str != '\0')
	{
		*cp++ = *str++;
	}

	varbuf->vb_cur_length += len ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return len ;
}

int varbuf_strncat( varbuf, str, n )
	varbuf_t *varbuf; 
	char *str ;
	int   n ;
{
	if( varbuf == NULL || str == NULL || n < 0 ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	if( n == 0 )
		return varbuf->vb_cur_length ;

	if( varbuf_grow( varbuf, n ) == False ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return -1 ;
	}

	strncpy(varbuf->vb_buffer + varbuf->vb_cur_length, str, n) ;
	varbuf->vb_cur_length += n ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return varbuf->vb_cur_length ;
}

int varbuf_subst( varbuf, start_idx, nchars, subst_str )
	varbuf_t   *varbuf ;
	int         start_idx ;
	int         nchars ;
	char       *subst_str ;
{
	int   subst_len ;
	int   nshift ;


	if( subst_str == NULL )
		subst_len = 0 ;
	else
		subst_len = strlen( subst_str ) ;

	/*
	 * If the number of characters to replace is 0 or negative, then
	 * we are replacing that many characters from the end of the
	 * buffer.
	 */
	if( nchars <= 0 )
		nchars = (varbuf->vb_cur_length - start_idx) + nchars ;

	if( varbuf == NULL || start_idx < 0 || start_idx > varbuf->vb_cur_length ||
		 nchars < 1 || (nchars + start_idx) > varbuf->vb_cur_length ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	/*
	 * If the length of the string to be inserted into the varbuf is
	 * longer than the whole to be replaced, then we need to grow
	 * the varbuf.
	 */
	if( subst_len > nchars ) {

		/*
		 * Calculate the number of characters that we need to shift
		 * to the right in order to make room for the substitute
		 * string.
		 */
		nshift = subst_len - nchars ;

		if( varbuf_grow( varbuf, nshift ) == False ) {
			sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
			return -1 ;
		}
		if( (start_idx + nchars) < varbuf->vb_cur_length ) {
			memmove( varbuf->vb_buffer + start_idx + subst_len,
						varbuf->vb_buffer + start_idx + nchars,
						varbuf->vb_cur_length - (start_idx + nchars) ) ;
		}
		varbuf->vb_cur_length += nshift ;
	} else {
		/*
		 * If the string to be inserted is smaller than the whole
		 * it is replacing, then we need to shift the remainder
		 * of the buffer left a little bit.
		 */
		if( subst_len < nchars ) {
			nshift = nchars - subst_len ;
			if( (start_idx + nchars) < varbuf->vb_cur_length ) {
				memmove( varbuf->vb_buffer + start_idx + subst_len, 
							varbuf->vb_buffer + start_idx + nchars,
							varbuf->vb_cur_length - (start_idx + nchars) ) ;
			}
			varbuf->vb_cur_length -= (nchars - subst_len) ;
		}
	}

	if( subst_len > 0 )
		memcpy( varbuf->vb_buffer + start_idx, subst_str, subst_len ) ;
	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return varbuf->vb_cur_length ;
}

/*
 * For some reason, the Cray C compiler chokes on the following
 * function delcaration unless I do it this way.
 */
#ifdef __ansi__
int varbuf_charcat( varbuf_t *varbuf, char ch )
#else
int varbuf_charcat( varbuf, ch )
	varbuf_t *varbuf ;
	char ch ;
#endif
{
	if( varbuf == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	if( varbuf_grow( varbuf, 1 ) == False ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return -1 ;
	}

	varbuf->vb_buffer[varbuf->vb_cur_length++] = ch ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return 1 ;
}

int varbuf_strcpy( varbuf, str )
	varbuf_t *varbuf ;
	char *str ;
{
	if( varbuf == NULL || str == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	varbuf->vb_cur_length = 0 ;
	return (varbuf_strcat( varbuf, str )) ;
}

#ifdef __ansi__
int varbuf_charcpy( varbuf_t *varbuf, char ch )
#else
int varbuf_charcpy( varbuf, ch )
	varbuf_t *varbuf ;
	char ch ;
#endif
{
	if( varbuf == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}
	varbuf->vb_cur_length = 0 ;
	varbuf->vb_buffer[varbuf->vb_cur_length++] = ch ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return 1 ;
}

int varbuf_size( varbuf, size )
	varbuf_t *varbuf ;
	long size ;
{
	int    new_length ;
	char*  new_buffer ;

	if( varbuf == NULL || size < 1 ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	if (varbuf->vb_tot_length < size ) {
		new_length = (((int) size/varbuf->vb_growsize)+1) * varbuf->vb_growsize ;
		new_buffer = (char*)realloc( varbuf->vb_buffer, new_length ) ;

		if( new_buffer == NULL ) {
			sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
			return -1 ;
		}

		varbuf->vb_tot_length = new_length ;
		varbuf->vb_buffer     = new_buffer ;
	} else
		new_length = size ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return new_length ;
}

int varbuf_setlen( varbuf, size )
	varbuf_t *varbuf ;
	long size ;
{
	int    new_length ;
	char*  new_buffer ;

	if( varbuf == NULL || size < 1 ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	if (varbuf->vb_tot_length < size ) {
		new_length = (((int) size/varbuf->vb_growsize)+1) * varbuf->vb_growsize ;
		new_buffer = (char*)realloc( varbuf->vb_buffer, new_length ) ;

		if( new_buffer == NULL ) {
			sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
			return -1 ;
		}

		varbuf->vb_tot_length = new_length ;
		varbuf->vb_buffer     = new_buffer ;
	}

	varbuf->vb_cur_length = size ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return size ;
}

#ifdef __ansi__
int varbuf_printf( varbuf_t *varbuf, char *str, ... )
#else
int varbuf_printf( varbuf, str, va_alist )
	varbuf_t *varbuf ;
	char     *str ;
	va_dcl 
#endif
{
	va_list ap ;
	int     retval ;

#ifdef __ansi__
	va_start(ap, str) ;
#else
	va_start(ap) ;
#endif

	retval = varbuf_vprintf( varbuf, str, ap ) ;
	va_end(ap) ;
	return retval ;
}

char* varbuf_getstr( varbuf )
	varbuf_t *varbuf ;
{
	if( varbuf == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return (char*)NULL ;
	}

	if ( varbuf_grow( varbuf, 1 ) == False ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return (char*)NULL ;
	}

	varbuf->vb_buffer[varbuf->vb_cur_length] = '\0' ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return varbuf->vb_buffer ;
}

int varbuf_vprintf( varbuf, str, ap )
	varbuf_t *varbuf ;
	char     *str ;
	va_list  ap ;
{
	char buffer[1024] ;
	int charcount ;

	if( varbuf == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	vsprintf( buffer, str, ap ) ;
	charcount = varbuf_getlen( varbuf ) ;
	if( varbuf_strcat( varbuf, buffer ) == -1 )
		return -1 ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return (varbuf_getlen(varbuf) - charcount) ;
}

/*
 * PRIVATE FUNCTIONS
 */

/*
 * varbuf_grow:  internal function to handle increasing the size of the
 *               string buffer.  Given the current varbuf, it will attempt
 *               to grow the length of the string by add_size bytes.  Read
 *               the comments for more detail.
 */
static int varbuf_grow( varbuf, add_size )
	varbuf_t *varbuf ;
	int add_size ;
{
	int   new_length ;
	char *new_buffer ;
	int sizedif ;

	/*
	 * Get the difference in total new length of the string and the
	 * current malloc'ed size of our buffer.
	 */
	sizedif = (varbuf->vb_cur_length + add_size) - (varbuf->vb_tot_length - 2) ;

	/*
	 * If there is a positive difference (i.e. total string size > current
	 * buffer size),
	 */
	if (sizedif >= 0) {
		/*
		 * Malloc the least number of BUF_GROWSIZE bytes that can be added
		 * in order to fit the string in the buffer.
		 */
		new_length = varbuf->vb_tot_length + (varbuf->vb_growsize * 
			(((int) sizedif / varbuf->vb_growsize)+1) ) ;
		new_buffer = (char*)realloc( varbuf->vb_buffer, new_length ) ;

		if( new_buffer == NULL )
			return False ;


		varbuf->vb_tot_length = new_length ;
		varbuf->vb_buffer     = new_buffer ;
	}
	return True ;
}
