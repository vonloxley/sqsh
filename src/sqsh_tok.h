/*
 * sqsh_tok.h - Lexical analysis (tokenizing) of a command line
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
#ifndef sqsh_tok_h_included
#define sqsh_tok_h_included
#include "sqsh_varbuf.h"

/*-- Possible Tokens --*/
#define SQSH_T_EOF         0    /* End-of-line */
#define SQSH_T_REDIR_OUT   1    /* A '[n]>', or a '[n]>>' */
#define SQSH_T_REDIR_IN    2    /* A '<' */
#define SQSH_T_REDIR_DUP   3    /* A '[m]>&[n]' */
#define SQSH_T_WORD        4    /* Something other than above */

/*
 * The following is the longest token that we are capable of dealing
 * with.  I don't really like hard-coding this, but it is better than
 * constantly dynamically allocating a new buffer each time we need to
 * tokenize.
 */
#define SQSH_TOKLEN      512

typedef struct tok_st {

	int   tok_type ;           /* One of the above defines */

	/*
	 * For SQSH_T_REDIR_OUT, this will be a True or False if we are
	 * appending to a file or overwriting it.
	 */
	int   tok_append ;         /* True/False, Are we to append to file? */

	/*
	 * The following are use by the various redirection tokens.  In the case
	 * of SQSH_T_REDIR_OUT only tok_fd_left will be valid, for SQSH_T_REDIR_DUP,
	 * both will be valid (tok_fd_left will always be m, from above), for
	 * all other tokens these don't apply.
	 */
	int   tok_fd_left ;        /* File descriptor to left of token */
	int   tok_fd_right ;       /* File descriptor to right of token */

	/*
	 * All tokens will fill in the text for which they represent
	 */
 	varbuf_t  *tok_value ;

} tok_t ;

/*
 * Flags tot the tokenizer.
 */
#define TOK_F_LEAVEQUOTE   (1<<0)   /* Leave quotes in-tact */
#define TOK_F_TEST         (1<<1)   /* Expand '[' ']' test operators */

/*-- Local Prototypes --*/
int   sqsh_tok         _ANSI_ARGS(( char*, tok_t**, int )) ;

#define sqsh_tok_value(t)  varbuf_getstr((t)->tok_value)

#endif
