/*
 * sqsh_tok.c - Lexical analysis (tokenizing) of a command line
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
#include "sqsh_tok.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_tok.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes --*/
static tok_t* tok_create _ANSI_ARGS(( void ));

/*
 * sg_cptr:  This variable keeps track of the current location within
 *           a buffer being parsed.  It must be a static global because
 *           it is shared between both parse routines.
 */
static char *sg_cptr = NULL;

/*
 * sg_tok:   This is the buffer used to store the token returned by 
 *           sqsh_tok().
 */
static tok_t *sg_tok = NULL;

/*-- Types of quotes we may encounter --*/
#define QUOTE_NONE          0
#define QUOTE_SINGLE        1
#define QUOTE_DOUBLE        2

/*
 * sqsh_tok():
 *
 * Tokenizes a command line, returning either words or some special
 * expression to the caller.  If the end of the str is reached 
 * SQSH_T_EOF is returned.  The end of the line is considered wither
 * when '\0' is reached or if a | or a & is encountered.
 */
int sqsh_tok( str, token, tok_flag )
	char      *str;
	tok_t     **token;
	int         tok_flag;
{
	int      idx;
	int      ret;
	char     number[64];
	int      quote_type = QUOTE_NONE;

	/*
	 * Although it isn't usually necessary, check the parameters,
	 * you can never trust the caller.
	 */
	if( token == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	/*
	 * Now, initialize the global token buffer, if it hasn't been
	 * initialized already, then point the user's pointer to the
	 * global token.
	 */
 	if (sg_tok == NULL)
	{
 		if( (sg_tok = tok_create()) == NULL )
 			return False;
 	}
 	*token = sg_tok;

	/*
	 * If the user passed in a non-null str pointer then this is our
	 * first time through the string.
	 */
	if (str != NULL)
	{
		sg_cptr = str;
	}

restart_parse:

	/*-- Innocent until proven guilty --*/
	sqsh_set_error( SQSH_E_NONE, NULL );
	varbuf_clear( sg_tok->tok_value );


	/*-- Skip any whitespace --*/
	for (; sg_cptr != NULL && *sg_cptr != '\0' && isspace((int)*sg_cptr);
		++sg_cptr);

	/*-- Check to see if we hit the end --*/
	if (sg_cptr == NULL || *sg_cptr == '\0')
	{
		sg_tok->tok_type     = SQSH_T_EOF;
		return True;
	}

	/*
	 * While reading the following code, keep a couple of things in mind.
	 * I am attempting to parse the command line in the same manner that
	 * bourne shell does, so white-space is important.  For example:
	 *
	 *     echo hello2>test
	 *
	 * Will be treated as:
	 *
	 *     echo hello2 > test
	 *
	 * (i.e the file "test" will contain the word hello2).  So, the first
	 * thing I want to do is look for some form of re-direction that could
	 * be semi-ambiguous, as shown above.
	 */

	/*
	 * SQSH_T_REDIR_DUP : [m]>&[n]
	 * SQSH_T_REDIR_OUT : [n]>
	 *                    [n]>>
	 */
	if (*sg_cptr == '>' || 
		(isdigit((int)*sg_cptr) && *(sg_cptr+1) == '>'))
	{

		/*
		 * If the first character is a '>', then we assume that the left
		 * file descriptor (the one being redirected) is stdout.
		 */
		if (*sg_cptr == '>')
		{
			sg_tok->tok_fd_left = 1;
			++sg_cptr;
		}
		else
		{
			/*
			 * Otherwise is we are looking at 'n>', then we assume that
			 * the left file descriptor is n.
			 */
			sg_tok->tok_fd_left = *sg_cptr - '0';
			sg_cptr += 2;
		}

		if (*sg_cptr == '>')
		{
			/*
			 * If we are dealing with a [n]>>, then we simply need to
			 * consume the last '>', and return the appropriate token
			 * type.
			 */
			sg_tok->tok_type   = SQSH_T_REDIR_OUT;
			sg_tok->tok_append = True;
			++sg_cptr;
			varbuf_printf( sg_tok->tok_value, "%d>>", sg_tok->tok_fd_left );

		}
		else if (*sg_cptr == '&')
		{
			/*
			 * If we are dealing with [m]>&[n], then we have a little
			 * bit of work to do.  First, we know which token we have.
			 */
			sg_tok->tok_type    = SQSH_T_REDIR_DUP;

			/*-- Skip any whitespace --*/
			for (++sg_cptr; *sg_cptr != '\0' && isspace((int)*sg_cptr);
				++sg_cptr);

			/*
			 * Copy second fd into sg_tok->tok_value (temporary), note that
			 * this is a little different from bourne shell.  In Bourne
			 * the string 'echo hello 1>&2test' acts as though you had typed
			 * 'echo hello 1>&2' while this will be 'echo hello 1>&2 test'.
			 */
			idx = 0;
			while (*sg_cptr != '\0' && isdigit((int)*sg_cptr))
			{
				number[idx++] = *sg_cptr++;
			}
			number[idx] = '\0';

			/*
			 * If idx is still 0 then we didn't find a file descriptor
			 * to redirect too. Which is an error.
			 */
			if (idx == 0)
			{
				sqsh_set_error( SQSH_E_SYNTAX, "Expected fd following >&" );
				return False;
			}
			else
			{
				sg_tok->tok_fd_right = atoi(number);
			}

			varbuf_printf( sg_tok->tok_value, "%d>&%d", sg_tok->tok_fd_left, 
			               sg_tok->tok_fd_right );
		}
		else
		{
			/*
			 * In this case we have encountered simply [n]>, without
			 * anything following it, so we are pretty much done.
			 */
			sg_tok->tok_type   = SQSH_T_REDIR_OUT;
			sg_tok->tok_append = False;
			varbuf_printf( sg_tok->tok_value, "%d>", sg_tok->tok_fd_left );
		}

		return True;
	}

	/*
	 * SQSH_T_EOF:  ("&", "|")
	 */
	if (*sg_cptr == '|' || *sg_cptr == '&')
	{
		sg_tok->tok_type = SQSH_T_EOF;
		return True;
	}

	/*
	 * SQSH_T_REDIR_IN: ("<")
	 */
	if (*sg_cptr == '<')
	{
		sg_tok->tok_type = SQSH_T_REDIR_IN;
		varbuf_strcpy( sg_tok->tok_value, "<" );
		++sg_cptr;
		return True;
	}

	/*
	 * If we hit a '[' and the next character is a space, and
	 * the caller wants us to do test conversion (i.e.
	 * '[ expr ]' -> 'test expr', then do so.
	 */
	if (*sg_cptr == '[' && 
		(tok_flag & TOK_F_TEST) != 0 && isspace((int)*(sg_cptr+1)) )
	{
		sg_tok->tok_type = SQSH_T_WORD;
		varbuf_strcpy( sg_tok->tok_value, "test" );
		++sg_cptr;
		return True;
	}

	/*
	 * If we hit a ']' and the caller wants us to do test 
	 * conversion, then throw it away and go back to look
	 * for more special characters.
	 */
	if (*sg_cptr == ']' &&
		(tok_flag & TOK_F_TEST) != 0)
	{
		++sg_cptr;
		goto restart_parse;
	}

	/*
	 * SQSH_T_WORD:
	 *
	 * That's it! We couldn't figure out what the token was, so
	 * we just call it a word.  This is the point in which we strip
	 * out quotes.
	 */
	sg_tok->tok_type = SQSH_T_WORD;

	/*
	 * Throughout the following loop, word_start will point to the beginning
	 * of the word that we are parsing and sg_cptr will be incremented until
	 * we find the end of the word.  Following that, the string between
	 * word_start and sg_cptr will be expanded into sg_tok->tok_value.
	 */
	while (*sg_cptr != '\0')
	{

		/*
		 * If we hit a white space, or one of the characters that begin
		 * our special tokens, then we may have hit the end of the current
		 * token.
		 */
		if ((isspace((int)*sg_cptr) || strchr("><&|", *sg_cptr) != NULL))
		{
			/*
			 * However, if we are in single then quote then consume this
			 * character by placing it in our token buffer, and move on.
			 */
			if (quote_type != QUOTE_NONE)
			{
				ret = varbuf_charcat( sg_tok->tok_value, *sg_cptr++ );
			}
			else
			{
				break;
			}

		}
		else
		{

			/*
			 * We haven't are still in the middle of a token, so take a
			 * look at the next character.
			 */
			switch (*sg_cptr)
			{
				/*
				 * A single quote is taken literally within a double quote,
				 * otherwise we need to keep track of the quoted string.
				 */
				case '\'' :
					if (quote_type == QUOTE_DOUBLE)
					{
						ret = varbuf_charcat( sg_tok->tok_value, *sg_cptr);
					}
					else
					{
						ret = 0;

						quote_type = (quote_type == QUOTE_SINGLE)? 
							QUOTE_NONE :
							QUOTE_SINGLE;
						
						/*
						 * If the caller wants us to leave quotes in-tact,
						 * then lets do so.
						 */
						if ((tok_flag & TOK_F_LEAVEQUOTE) != 0)
						{
							ret = varbuf_charcat( sg_tok->tok_value, *sg_cptr);
						}
					}
					++sg_cptr;
					break;

				/*
				 * A double quote is taken literally within a single quote,
				 * otherwise we need to keep track of the quoted string.
				 */
				case '\"' :
					if (quote_type == QUOTE_SINGLE)
					{
						ret = varbuf_charcat( sg_tok->tok_value, *sg_cptr );
					}
					else
					{
						ret = 0;
						quote_type = (quote_type == QUOTE_DOUBLE) ?
							QUOTE_NONE :
							QUOTE_DOUBLE;

						/*
						 * If the caller wants us to leave quotes in-tact,
						 * then lets do so.
						 */
						if ((tok_flag & TOK_F_LEAVEQUOTE) != 0)
						{
							ret = varbuf_charcat( sg_tok->tok_value, *sg_cptr);
						}
					}
					++sg_cptr;
					break;

				/*
				 * Escape character.
				 */
				case '\\' :
					/*
					 * If we are inside single quotes, or we don't have a
					 * '\\' or we have been asked to leave quotes in place,
					 * then simply copy the backslash into the buffer.
					 */
					if (quote_type == QUOTE_SINGLE || 
						(tok_flag & TOK_F_LEAVEQUOTE) != 0 ||
						*(sg_cptr+1) != '\\')
					{
						ret = varbuf_charcat( sg_tok->tok_value, *sg_cptr++ );
						break;
					}

					/*
					 * Skip straight to the escaped character.
					 */
					sg_cptr += 2;

					/*
					 * If we are escaping a new-line or end-of-string, then
					 * ignore it (later, we would like to request further input
					 * from the user).
					 */
					if (*sg_cptr != '\0' && *sg_cptr != '\n')
					{
						ret = varbuf_charcat( sg_tok->tok_value, *sg_cptr );
					}
					else
					{
						ret = 0;
					}

					++sg_cptr;
					break;

				default :
					ret = varbuf_charcat( sg_tok->tok_value, *sg_cptr++ );
					break;

			} /* switch() */

		} /* if (!(isspace())) */

		if (ret == -1)
		{
			sqsh_set_error( sqsh_get_error(), "varbuf_charcat: %s",
			                sqsh_get_errstr() );
			return False;
		}

	}  /* while() */

	/*
	 * Check to see if we have an quote laying around without a closing
	 * quote.  That sucks.
	 */
	if( *sg_cptr == '\0' && quote_type != QUOTE_NONE ) {
		sqsh_set_error( SQSH_E_BADQUOTE, "Unbounded %s quote",
		 quote_type == QUOTE_SINGLE ? "single" : "double" );
		return False;
	}

	return True;
}

/*
 * tok_create()
 *
 * Internal function to create an uninitialized token.
 */
static tok_t* tok_create()
{
	tok_t   *tok;

	if( (tok = (tok_t*)malloc(sizeof(tok_t))) == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return NULL;
	}

	if( (tok->tok_value = varbuf_create( 64 )) == NULL ) {
		free( tok );
		sqsh_set_error( sqsh_get_error(), "varbuf_create: %s",
		                sqsh_get_errstr() );
	 	return NULL;
	}

	return tok;
}
