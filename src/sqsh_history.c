/*
 * sqsh_history.c - Rolling queue of command histories
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
#include "sqsh_error.h"
#include "sqsh_history.h"
#include "sqsh_varbuf.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_history.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Local Prototypes --*/
static hisbuf_t* hisbuf_create  _ANSI_ARGS(( history_t*, char*, int )) ;
static hisbuf_t* hisbuf_get     _ANSI_ARGS(( history_t*, int )) ;
static int       hisbuf_destroy _ANSI_ARGS(( hisbuf_t* )) ;

/*
 * history_create():
 *
 * Allocates a history structure large enough to hold up to size history
 * entries.
 */
history_t* history_create( size )
	int   size ;
{
	history_t  *h ;

	/*-- Always check your parameters --*/
	if( size < 0 ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return NULL ;
	}

	/*-- Allocate a new history structure --*/
	if( (h = (history_t*)malloc(sizeof(history_t))) == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return NULL ;
	}

	/*-- Initialize elements of structure --*/
	h->h_size     = size ;
	h->h_nitems   = 0 ;
	h->h_start    = NULL ;
	h->h_next_nbr = 1 ;
	h->h_end      = NULL ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return h ;
}

/*
 * history_get_size():
 *
 * Returns the total size of the current history.
 */
int history_get_size( h )
	history_t  *h ;
{
	/*-- Check parameters --*/
	if( h == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	return h->h_size ;
}

/*
 * history_get_nitems():
 *
 * Returns the total number of entries in history.
 */
int history_get_nitems( h )
	history_t  *h ;
{
	/*-- Check parameters --*/
	if( h == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return -1 ;
	}

	return h->h_nitems ;
}

/*
 * history_get_nbr():
 *
 * Returns the next available history entry number.
 */
int history_get_nbr( h )
	history_t  *h ;
{
	return h->h_next_nbr ;
}

int history_set_size( h, size )
	history_t *h ;
	int        size ;
{
	hisbuf_t  *hb ;

	/*-- Check parameters --*/
	if( h == NULL || size < 0 ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*
	 * If the size was smaller than the current number of items
	 * in the history then we need to roll older items off of
	 * the history until we can fit the new size.
	 */
	while( size < h->h_nitems ) {

		/*
		 * Unlink the oldest entry from the list and destroy
		 * it.
		 */
 		hb = h->h_end ;
		if( hb->hb_prv != NULL ) {
			h->h_end         = hb->hb_prv ;
			h->h_end->hb_nxt = NULL ;
		} else {
			h->h_end = h->h_start = NULL ;
		}
		hisbuf_destroy( hb ) ;

		--h->h_nitems ;
	}

	h->h_size = size ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

/*
 * history_append():
 *
 * Adds buffer buf to history h.
 */
int history_append( h, buf )
	history_t   *h ;
	char        *buf ;
{
	hisbuf_t  *hisbuf, *hb_tmp ;
	int        len ;

	/*-- Check parameters --*/
	if( h == NULL || buf == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*-- Get length of buffer --*/
	len = strlen( buf ) ;

	/*
	 * If the buffer is set up to hold no entries, or the buffer is
	 * empty, then we don't need to go any further.
	 */
	if( h->h_size == 0 || len == 0 ) {
		sqsh_set_error( SQSH_E_NONE, NULL ) ;
		return True ;
	}

	/*
	 * Go ahead an try to allocate a buffer before we try to
	 * stick it into the history.
	 */
	if( (hisbuf = hisbuf_create( h, buf, len )) == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		return False ;
	}

	/*
	 * If the buffer is full, then we need to roll and entry off
	 * of the beginning of the list.
	 */
	if( h->h_nitems == h->h_size ) {

		hb_tmp = h->h_end->hb_prv ;    /* Temp pointer to next-to-last */
		hisbuf_destroy( h->h_end ) ;   /* Destroy last entry */
		h->h_end = hb_tmp ;            /* Make end be the next-to-last */

		if( h->h_end != NULL )         /* If there is an end */
			h->h_end->hb_nxt = NULL ;   /* Then it doesn't point anywhere */
		else
			h->h_start = NULL ;         /* Otherwise list is empty */

		--h->h_nitems ;                /* One fewer... */
	}

	/*
	 * Now that we have the space, stick the new hisbuf_t into
	 * our history list.
	 */
	if( h->h_start == NULL ) {
		h->h_start = hisbuf ;
		h->h_end   = hisbuf ;
	} else {
		h->h_start->hb_prv = hisbuf ;
		hisbuf->hb_nxt     = h->h_start ;
		h->h_start         = hisbuf ;
	}
	++h->h_nitems ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

int history_find( h, idx, buf )
	history_t  *h ;
	int         idx ;
	char      **buf ;
{
	hisbuf_t  *hb ;

	/*-- Check parameters not checked by hisbuf_get() --*/
	if( buf == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*-- Retrieve the appropriate history entry */
	if( (hb = hisbuf_get( h, idx )) == NULL )
		return False ;

	*buf = hb->hb_buf ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

/*
 * history_del():
 *
 * Deletes entry idx from history h.
 */
int history_del( h, idx )
	history_t   *h ;
	int          idx ;
{
	hisbuf_t   *hb ;

	/*-- Retrieve the appropriate history entry */
	if( (hb = hisbuf_get( h, idx )) == NULL )
		return False ;

	/*-- Unlink the node --*/
	if( hb->hb_prv != NULL )
		hb->hb_prv->hb_nxt = hb->hb_nxt ;
	else
		h->h_start = hb->hb_nxt ;

	if( hb->hb_nxt != NULL )
		hb->hb_nxt->hb_prv = hb->hb_prv ;
	else
		h->h_end = hb->hb_prv ;

	hisbuf_destroy( hb ) ;

	--h->h_nitems ;
	return True ;
}

int history_clear( h )
	history_t  *h ;
{
	hisbuf_t  *hb, *hb_nxt ;

	/*-- Check parameters --*/
	if( h == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	hb = h->h_start ;
	while( hb != NULL ) {
		hb_nxt = hb->hb_nxt ;
		hisbuf_destroy( hb ) ;
		hb = hb_nxt ;
	}

	h->h_start  = NULL ;
	h->h_end    = NULL ;
	h->h_nitems = 0 ;

	return True ;
}

/*
 * history_save():
 *
 * Saves the current command history to file named save_file,
 * returns True upon success, False upon failure.
 */
int history_save( h, save_file )
	history_t   *h ;
	char        *save_file ;
{
	hisbuf_t  *hb ;
	FILE      *fptr ;

	/*-- Check the arguments --*/
	if( h == NULL || save_file == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*-- Open the file to be save to --*/
	if( (fptr = fopen( save_file, "w" )) == NULL ) {
		sqsh_set_error( errno, "%s: %s", save_file, strerror( errno ) ) ;
		return False ;
	}

	/*
	 * Traverse the list of history buffers from oldest to newest, this
	 * way we can perform a regular history_append() on each buffer while
	 * loading them in and they will be in the correct order.
	 */
	for( hb = h->h_end; hb != NULL; hb = hb->hb_prv ) {
		/*
		 * Since buffer can contain just about anything we need to 
		 * provide some sort of separator.
		 */
		fprintf( fptr, "--> History Entry <--\n" ) ;
		fputs( hb->hb_buf, fptr ) ;

	}

	fclose( fptr ) ;

	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

/*
 * history_load():
 *
 * Load history contained in load file into h.  If h is too small to
 * hold all of the entries in load_file, then the oldest entries are
 * rolled off as necessary.
 */
int history_load( h, load_file )
	history_t    *h ;
	char         *load_file ;
{
	FILE      *fptr ;
	char       str[1024] ;
	varbuf_t  *history_buf ;

	/*-- Check the arguments --*/
	if( h == NULL || load_file == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return False ;
	}

	/*-- Attempt to open the load file --*/
	if( (fptr = fopen( load_file, "r" )) == NULL ) {
		sqsh_set_error( errno, "%s: %s", load_file, strerror( errno ) ) ;
		return False ;
	}

	/*
	 * Create a varbuf structure to hold the current buffer being
	 * loaded, this buffer will be destroyed before we return
	 * to the caller.
	 */
 	if( (history_buf = varbuf_create(1024)) == NULL ) {
		sqsh_set_error( sqsh_get_error(), "varbuf_create: %s",
		                sqsh_get_errstr() ) ;
	 	return False ;
	}

	while( fgets( str, sizeof(str), fptr ) != NULL ) {
		/*
		 * If we hit the separator for history entries then we need
		 * to save the current history buffer that we are building.
		 */
		if( strcmp( str, "--> History Entry <--\n" ) == 0 ) {

			/*
			 * However, since the first line of the load file contains
			 * the separator we don't wan't to build and empty buffer.
			 */
			if( varbuf_getlen( history_buf ) > 0 ) {
				/*
				 * Create a new entry.
				 */
			 	if( history_append( h, varbuf_getstr( history_buf ) ) == False ) {
			 		fclose( fptr ) ;
				 	varbuf_destroy( history_buf ) ;
			 		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
			 		return False ;
				}
			}

			varbuf_clear( history_buf ) ;
		} else {
			/*
			 * Since this isn't an entry separator, it is a line that
			 * needs to be added to the current buffer.
			 */
		 	if( varbuf_strcat( history_buf, str ) == -1 ) {
			 	fclose( fptr ) ;
		 		varbuf_destroy( history_buf ) ;
		 		sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
		 		return False ;
			}
		}
	}

	fclose( fptr ) ;

	/*
	 * If there is anything left in the history buffer then we need
	 * to add it to the history as well.
	 */
	if( varbuf_getlen( history_buf ) > 0 ) {

		if( history_append( h, varbuf_getstr( history_buf ) ) == False ) {
			varbuf_destroy( history_buf ) ;
			sqsh_set_error( SQSH_E_NOMEM, NULL ) ;
			return False ;
		}

	}

	varbuf_destroy( history_buf ) ;
	sqsh_set_error( SQSH_E_NONE, NULL ) ;
	return True ;
}

int history_destroy( h )
	history_t *h ;
{
	if( history_clear( h ) == False )
		return False ;

	free( h ) ;
	return True ;
}

/*
 * hisbuf_get():
 *
 * Retrieves history entry idx from h If idx is HISTORY_HEAD then the
 * first (most recent) entry into the history is returned and if idx
 * is HISTORY_TAIL the last (oldest) entry is returned.  Upon success
 * True is returned, otherwise False.
 */
static hisbuf_t* hisbuf_get( h, idx )
	history_t  *h ;
	int         idx ;
{
	hisbuf_t  *hb ;

	/*-- Check parameters --*/
	if( h == NULL || (idx < 0 && idx != HISTORY_HEAD && idx != HISTORY_TAIL) ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL ) ;
		return NULL ;
	}

	/*-- The most recent entry? --*/
	if( idx == HISTORY_HEAD ) {
		if( h->h_start == NULL ) {
			sqsh_set_error( SQSH_E_EXIST, "History is empty" ) ;
			return NULL ;
		}

		return h->h_start ;
	}

	/*-- The most oldest entry? --*/
	if( idx == HISTORY_TAIL ) {
		if( h->h_end == NULL ) {
			sqsh_set_error( SQSH_E_EXIST, "History is empty" ) ;
			return NULL ;
		}

		return h->h_end ;
	}

	/*
	 * Perform a rather inefficient, linear, search through
	 * our history list looking for idx.
	 */
	for( hb = h->h_start; hb != NULL && hb->hb_nbr != idx;
		  hb = hb->hb_nxt ) ;

	if( hb == NULL ) {
		sqsh_set_error( SQSH_E_EXIST, "Invalid history number %d", idx ) ;
		return NULL ;
	}

	return hb ;
}


static hisbuf_t* hisbuf_create( h, buf, len )
	history_t *h ;
	char      *buf ;
	int        len ;
{
	hisbuf_t  *hisbuf ;

	/*-- Allocate --*/
	if( (hisbuf = (hisbuf_t*)malloc(sizeof(hisbuf_t))) == NULL )
		return NULL ;

	/*-- Initialize --*/
	hisbuf->hb_len = len ;
	hisbuf->hb_nbr = h->h_next_nbr++ ;
	hisbuf->hb_nxt = NULL ;
	hisbuf->hb_prv = NULL ;

	/*-- Copy the buffer --*/
	if( (hisbuf->hb_buf = sqsh_strdup( buf )) == NULL ) {
		free( hisbuf ) ;
		return NULL ;
	}

	return hisbuf ;
}

static int hisbuf_destroy( hb )
	hisbuf_t  *hb ;
{
	if( hb != NULL ) {
		if( hb->hb_buf != NULL )
			free( hb->hb_buf ) ;
		free( hb ) ;
	}
	return True ;
}
