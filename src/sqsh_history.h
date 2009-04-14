/*
 * sqsh_history.h - Rolling queue of command histories
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
#ifndef history_h_included
#define history_h_included

/*
 * The following data structure represents a single entry in a 
 * history buffer.
 */
typedef struct hisbuf_st {
	int               hb_nbr ;    /* History number */
	int               hb_len ;    /* Total length of buffer */
	unsigned long     hb_chksum;  /* sqsh-2.1.6 feature - buffer checksum */
	char             *hb_buf ;    /* The buffer itself */
	struct hisbuf_st *hb_nxt ;    /* Next buffer in chain */
	struct hisbuf_st *hb_prv ;    /* Previous buffer in chain */
} hisbuf_t ;

/*
 * The history buffer is simply a queue of strings (an array of char*'s),
 * where old entries are rolled off of the queue.
 */
typedef struct history_st {
	int       h_size ;             /* Total size of the history */
	int       h_nitems ;           /* Number of items in history */
	int       h_next_nbr ;         /* Next available history number */
	hisbuf_t *h_start ;            /* Youngest buffer */
	hisbuf_t *h_end ;              /* Oldest buffer */
} history_t ;

#define HISTORY_HEAD      -1
#define HISTORY_TAIL      -2

/*-- Prototypes --*/
history_t* history_create     _ANSI_ARGS(( int )) ;
int        history_set_size   _ANSI_ARGS(( history_t*, int )) ;
int        history_get_size   _ANSI_ARGS(( history_t* )) ;
int        history_get_nitems _ANSI_ARGS(( history_t* )) ;
int        history_get_nbr    _ANSI_ARGS(( history_t* )) ;
int        history_append     _ANSI_ARGS(( history_t*, char* )) ;
int        history_find       _ANSI_ARGS(( history_t*, int, char** )) ;
int        history_del        _ANSI_ARGS(( history_t*, int )) ;
int        history_clear      _ANSI_ARGS(( history_t* )) ;
int        history_save       _ANSI_ARGS(( history_t*, char* )) ;
int        history_load       _ANSI_ARGS(( history_t*, char* )) ;
int        history_destroy    _ANSI_ARGS(( history_t* )) ;

#endif /* history_h_included */
