/*
 * sqsh_error.h - Functions for setting/retrieving error state
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
#ifndef sqsh_error_h_included
#define sqsh_error_h_included

/*
 * The following define the set of errors that may be raised within
 * the sqsh shell.  Any positive values returned are considered OS
 * errors.
 */
#define SQSH_E_NONE       0     /* No error has ocurred */
#define SQSH_E_NOMEM     -1     /* Memory allocation failure */
#define SQSH_E_BADPARAM  -2     /* Invalid parameter to function */
#define SQSH_E_EXIST     -3     /* Object does not exist */
#define SQSH_E_INVAL     -4     /* Validation test failure */
#define SQSH_E_BADVAR    -5     /* Bad variable definition */
#define SQSH_E_BADQUOTE  -6     /* Unbound quote */
#define SQSH_E_SYNTAX    -7     /* Syntax error */
#define SQSH_E_RANGE     -9     /* Attempt to access item out of range */
#define SQSH_E_BADSTATE -10     /* Invalid state for operation */

/*-- Prototypes --*/
void  sqsh_set_error  _ANSI_ARGS(( int, char*, ... )) ;
int   sqsh_get_error  _ANSI_ARGS(( void )) ;
char* sqsh_get_errstr _ANSI_ARGS(( void )) ;

#endif  /* sqsh_error_h_included */
