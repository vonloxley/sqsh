/*
 * cmd.h - User command entry point
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
#ifndef cmd_h_included
#define cmd_h_included
#include "sqsh_cmd.h"
#include "sqsh_varbuf.h"

/*
 * The following defines are the return values agreed upon by all
 * cmd_* functions.  It should be noted that these are predominantly
 * of importance to cmd_loop().  Typically the return value of each
 * command indiciates to cmd_loop() the action that it should perform
 * on the current working buffer.
 */

#define CMD_FAIL             0    /* The command failed, the current */
                                  /* buffer is untouched */
#define CMD_LEAVEBUF         1    /* Command was succesfull, didn't touch */
                                  /* the current buffer */
#define CMD_ALTERBUF         2    /* Command altered the current buffer */
#define CMD_CLEARBUF         3    /* Command cleared current buffer */
#define CMD_EXIT             4    /* Command requests that cmd_loop() exit */
#define CMD_ABORT            5    /* Request cmd_loop() abort */
#define CMD_INTERRUPTED      6    /* Request cmd_loop() abort */
#define CMD_RETURN           7    /* Pop up a level */
#define CMD_BREAK            8    /* Pop up a level */

/*
 * Place all prototypes for custom commands here.
 */
int  cmd_loop       _ANSI_ARGS(( int, char** )) ;   /* Loop commands */
int  cmd_go         _ANSI_ARGS(( int, char** )) ;   /* Database commands */
int  cmd_connect    _ANSI_ARGS(( int, char** )) ;
int  cmd_bcp        _ANSI_ARGS(( int, char** )) ;
int  cmd_reconnect  _ANSI_ARGS(( int, char** )) ;
int  cmd_rpc        _ANSI_ARGS(( int, char** )) ;
int  cmd_reset      _ANSI_ARGS(( int, char** )) ;   /* Buffer commands */
int  cmd_redraw     _ANSI_ARGS(( int, char** )) ;
int  cmd_history    _ANSI_ARGS(( int, char** )) ;
int  cmd_buf_edit   _ANSI_ARGS(( int, char** )) ;
int  cmd_buf_copy   _ANSI_ARGS(( int, char** )) ;
int  cmd_buf_get    _ANSI_ARGS(( int, char** )) ;
int  cmd_buf_save   _ANSI_ARGS(( int, char** )) ;
int  cmd_buf_load   _ANSI_ARGS(( int, char** )) ;
int  cmd_buf_show   _ANSI_ARGS(( int, char** )) ;
int  cmd_buf_append _ANSI_ARGS(( int, char** )) ;
int  cmd_set        _ANSI_ARGS(( int, char** )) ;   /* Variable commands */
int  cmd_jobs       _ANSI_ARGS(( int, char** )) ;   /* Job Control */
int  cmd_wait       _ANSI_ARGS(( int, char** )) ;
int  cmd_kill       _ANSI_ARGS(( int, char** )) ;
int  cmd_show       _ANSI_ARGS(( int, char** )) ;
int  cmd_exit       _ANSI_ARGS(( int, char** )) ;   /* Misc */
int  cmd_abort      _ANSI_ARGS(( int, char** )) ;
int  cmd_echo       _ANSI_ARGS(( int, char** )) ;
int  cmd_sleep      _ANSI_ARGS(( int, char** )) ;
int  cmd_warranty   _ANSI_ARGS(( int, char** )) ;
int  cmd_help       _ANSI_ARGS(( int, char** )) ;
int  cmd_read       _ANSI_ARGS(( int, char** )) ;
int  cmd_lock       _ANSI_ARGS(( int, char** )) ;
int  cmd_shell      _ANSI_ARGS(( int, char** )) ;
int  cmd_alias      _ANSI_ARGS(( int, char** )) ;   /* Alias commands */
int  cmd_unalias    _ANSI_ARGS(( int, char** )) ;
int  cmd_do         _ANSI_ARGS(( int, char** )) ;
int  cmd_func       _ANSI_ARGS(( int, char** )) ;
int  cmd_call       _ANSI_ARGS(( int, char** )) ;
int  cmd_if         _ANSI_ARGS(( int, char** )) ;
int  cmd_while      _ANSI_ARGS(( int, char** )) ;
int  cmd_return     _ANSI_ARGS(( int, char** )) ;
int  cmd_break      _ANSI_ARGS(( int, char** )) ;
int  cmd_for        _ANSI_ARGS(( int, char** )) ;

#ifdef SQSH_INIT
/*
 * The following describes a command entry.  These are used by
 * sqsh_init() to populate g_cmdset (see sqsl_global.h) with the
 * initial set of available commands.
 */
typedef struct cmd_entry_st
{
	char      *ce_name ;           /* The name of the command */
	char      *ce_alias ;          /* If non-null, the aliased command */
	cmd_f     *ce_func ;           /* Pointer to the actual function */
}
cmd_entry_t ;

static cmd_entry_t  sg_cmd_entry[] = {

/*	COMMAND          ALIAS          FUNCTION  */
/*	------------     ----------     --------  */
#if !defined(NO_BCP)
	{ "\\bcp",       NULL,          cmd_bcp          },
#endif
	{ "\\rpc",       NULL,          cmd_rpc          },
	{ "\\lock",      NULL,          cmd_lock         },
	{ "\\loop",      NULL,          cmd_loop         },
	{ "\\exit",      NULL,          cmd_exit         },
	{ "\\abort",     NULL,          cmd_abort        },
	{ "\\quit",      NULL,          cmd_exit         },
	{ "\\done",      NULL,          cmd_exit         },
	{ "\\connect",   NULL,          cmd_connect      },
	{ "\\reset",     NULL,          cmd_reset        },
	{ "\\set",       NULL,          cmd_set          },
	{ "\\echo",      NULL,          cmd_echo         },
	{ "\\sleep",     NULL,          cmd_sleep        },
	{ "\\jobs",      NULL,          cmd_jobs         },
	{ "\\wait",      NULL,          cmd_wait         },
	{ "\\kill",      NULL,          cmd_kill         },
	{ "\\show",      NULL,          cmd_show         },
	{ "\\warranty",  NULL,          cmd_warranty     },
	{ "\\go",        NULL,          cmd_go           },
	{ "\\reconnect", NULL,          cmd_reconnect    },
	{ "\\read",      NULL,          cmd_read         },
	{ "\\help",      NULL,          cmd_help         },
	{ "\\alias",     NULL,          cmd_alias        },
	{ "\\unalias",   NULL,          cmd_unalias      },
	{ "\\redraw",    NULL,          cmd_redraw       },
	{ "\\history",   NULL,          cmd_history      },
	{ "\\shell",     NULL,          cmd_shell        },
	{ "\\do",        NULL,          cmd_do           },
	{ "\\func",      NULL,          cmd_func         },
	{ "\\call",      NULL,          cmd_call         },
	{ "\\if",        NULL,          cmd_if           },
	{ "\\while",     NULL,          cmd_while        },
	{ "\\for",       NULL,          cmd_for          },
	{ "\\return",    NULL,          cmd_return       },
	{ "\\break",     NULL,          cmd_break        },
	{ "\\buf-edit",  NULL,          cmd_buf_edit     },
	{ "\\buf-copy",  NULL,          cmd_buf_copy     },
	{ "\\buf-save",  NULL,          cmd_buf_save     },
	{ "\\buf-load",  NULL,          cmd_buf_load     },
	{ "\\buf-show",  NULL,          cmd_buf_show     },
	{ "\\buf-append",NULL,          cmd_buf_append   },
	{ "\\buf-get",   NULL,          cmd_buf_get      },
	{ ":r",          NULL,          cmd_buf_load     },
	{ "vi",          NULL,          cmd_buf_edit     },
	{ "emacs",       NULL,          cmd_buf_edit     },

} ;
#endif

/* cmd_do.c */
int cmd_body_input _ANSI_ARGS(( varbuf_t* ));

/* cmd_if.c */
int cmd_if_exec    _ANSI_ARGS(( int, char**, int* ));

#endif /* cmd_h_included */
