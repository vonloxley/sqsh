/*
 * alias.h - User alias entry point
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
#ifndef alias_h_included
#define alias_h_included
#include "sqsh_alias.h"

#ifdef SQSH_INIT
/*
 * The following describes a command entry.  These are used by
 * sqsh_init() to populate g_cmdset (see sqsl_global.h) with the
 * initial set of available commands.
 */
typedef struct alias_entry_st {
	char      *ae_name ;           /* The name of the alias */
	char      *ae_body ;           /* The body of the alias */
} alias_entry_t ;

static alias_entry_t  sg_alias_entry[] = {

/*	NAME             BODY                                 */
/*	------------     ------------------------------------ */
   { "!",           "\\buf-append !. !!*"           },
	{ "reset",       "\\reset"                       },
	{ "reset",       "\\reset"                       },
	{ "exit",        "\\exit"                        },
	{ "quit",        "\\exit"                        },
	{ "edit",        "\\buf-edit"                    },
	{ "go",          "\\go"                          },
	{ "help",        "\\help"                        },

} ;

#endif

#endif /* alias_h_included */
