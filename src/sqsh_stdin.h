/*
 * sqsh_stdin.h - Steaming data input.
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
#ifndef sqsh_stdin_h_included
#define sqsh_stdin_h_included

int   sqsh_stdin_buffer _ANSI_ARGS(( char*, int ));
int   sqsh_stdin_file   _ANSI_ARGS(( FILE* ));
int   sqsh_stdin_pop    _ANSI_ARGS(( void ));
int   sqsh_stdin_isatty _ANSI_ARGS(( void ));
char* sqsh_stdin_fgets  _ANSI_ARGS(( char*, int ));

#endif /* sqsh_stdin_h_included */
