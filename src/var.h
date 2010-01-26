/*
 * var.h - Entry point for variable initialization and validation funcs.
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
#ifndef var_h_included
#define var_h_included
#include "sqsh_env.h"

/*-- Set validation function prototypes --*/
int var_set_bool            _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_int             _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_nullint         _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_nullstr         _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_severity        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_interfaces      _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_path_rw         _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_path_r          _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_histsize        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_notempty        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_add             _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_packet          _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_completion      _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_esc             _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_debug           _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_readonly        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_lock            _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_password        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_env             _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_width           _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_colwidth        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_colsep          _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_linesep         _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_outputparms     _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_xgeom           _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_style           _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_datetime        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_rl_histsize     _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_fltfmt          _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_bcp_colsep      _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_bcp_rowsep      _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_bcp_trim        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_set_maxlen          _ANSI_ARGS(( env_t*, char*, char** )) ;

/*-- Retrieval validation functions --*/
int var_get_date            _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_time            _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_env             _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_width           _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_colsep          _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_colwidth        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_linesep         _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_outputparms     _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_xgeom           _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_style           _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_datetime        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_bcp_colsep      _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_bcp_rowsep      _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_bcp_trim        _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_interactive     _ANSI_ARGS(( env_t*, char*, char** )) ;
int var_get_maxlen          _ANSI_ARGS(( env_t*, char*, char** )) ;

#ifdef SQSH_INIT

/*
 * The following data structure is used by sqsh_init() to initalize
 * the internal environment variable.
 */
typedef struct var_entry_st {
    char     *ve_name ;          /* Name of the variable */
    char     *ve_value ;         /* Default value of variable */
    env_f    *ve_set ;           /* Set validation function */
    env_f    *ve_get ;           /* Get validation function */
} var_entry_t ;

static var_entry_t  sg_var_entry[] = {

/*  VARIABLE              DEFAULT        SET VAL              GET VAL
   NAME                  VALUE          FUNCTION             FUNCTION
    ---------------       ------------   --------------       ------------- */
    { "banner",           "1",           var_set_bool,        NULL            },
    { "batch_failcount",  "0",           var_set_add,         NULL            },
    { "batch_pause",      "0",           var_set_bool,        NULL            },
    { "bcp_colsep",       NULL,          var_set_bcp_colsep, var_get_bcp_colsep},
    { "bcp_rowsep",       NULL,          var_set_bcp_rowsep, var_get_bcp_rowsep},
    { "bcp_trim",         NULL,          var_set_bcp_trim,    var_get_bcp_trim},
    { "chained",          NULL,          var_set_bool,        NULL }, /* XXX */
    { "charset",          NULL,          var_set_nullstr,     NULL            },
    { "clear_on_fail",    "1",           var_set_bool,        NULL            },
    { "colsep",           NULL,          var_set_colsep,      var_get_colsep  },
    { "colwidth",         "32",          var_set_colwidth,    var_get_colwidth},
    { "DISPLAY",          NULL,          var_set_env,         var_get_env     },
    { "database",         NULL,          var_set_nullstr,     NULL            },
    { "datetime",         NULL,          var_set_datetime,    var_get_datetime},
    { "debug",            "0",           var_set_debug,       NULL            },
    { "defer_bg",         "1",           var_set_bool,        NULL            },
    { "echo",             "0",           var_set_bool,        NULL            },
    { "encryption",       "0",           var_set_bool,        NULL            },
    { "exit_failcount",   "0",           var_set_bool,        NULL            },
    { "expand",           "1",           var_set_bool,        NULL            },
    { "filter",           "0",           var_set_bool,        NULL            },
    { "filter_prog",      "m4 -",        var_set_nullstr,     NULL            },
    { "float",            "18.6",        var_set_fltfmt,      NULL            },
    { "real",             "18.6",        var_set_fltfmt,      NULL            },
    { "footers",          "1",           var_set_bool,        NULL            },
    { "headers",          "1",           var_set_bool,        NULL            },
    { "help_dir",         SQSH_HELP,     var_set_path_r,      NULL            },
    { "history",          SQSH_HISTORY,  NULL,                NULL            },
    { "history_shorthand","0",           var_set_bool,        NULL            },
    { "histnum",          "0",           var_set_int,         NULL            },
    { "histsave",         "1",           var_set_bool,        NULL            },
    { "histsize",         "10",          var_set_histsize,    NULL            },
    { "hostname",         NULL,          var_set_nullstr,     NULL            },
    { "ifs",              "\n\f\r\t\v",  var_set_esc,         NULL            },
    { "interactive",      "1",           NULL,           var_get_interactive  },
    { "interfaces",        NULL,         var_set_interfaces,  NULL            },
    { "keyword_completion", "0",         var_set_completion,  NULL            },
    { "keyword_file",     SQSH_WORDS,    NULL,                NULL            },
    { "language",         NULL,          var_set_nullstr,     NULL            },
    { "lineno",           "1",           var_set_add,         NULL            },
    { "linesep",          NULL,          var_set_linesep,     var_get_linesep },
    { "lock",             NULL,          var_set_lock,        NULL            },
    { "maxlen",           NULL,          var_set_maxlen,      var_get_maxlen  },
    { "newline_go",       "0",           var_set_bool,        NULL            },
    { "output_parms",     "1",           var_set_outputparms, var_get_outputparms },
    { "packet_size",      NULL,          var_set_packet,      NULL            },
    { "password",         "",            var_set_password,    NULL            },
    { "password_retry",   "1",           var_set_bool,        NULL            },
    { "prompt",         "\\\\${lineno}> ",NULL,               NULL            },
    { "prompt2",        "\\\\${lineno}--> ",NULL,             NULL            },
    { "rcfile",           SQSH_RC,       NULL,                NULL            },
    { "repeat_batch",    "0",            var_set_bool,        NULL            },
    { "readline_history",SQSH_RLHISTORY, NULL,                NULL            },
    { "readline_histsize","100",         var_set_rl_histsize, NULL            },
    { "semicolon_hack",  "0",            var_set_bool,        NULL            },
    { "semicolon_cmd",   "\\go",         var_set_nullstr,     NULL            },
    { "session",          SQSH_SESSION,  var_set_nullstr,     NULL            },
    { "start_connected", "1",            var_set_bool,        NULL            },
    { "statistics",      "0",            var_set_bool,        NULL            },
    { "style",           "horiz",        var_set_style,       var_get_style   },
    { "thresh_display",  "0",            var_set_severity,    NULL            },
    { "thresh_exit",     "0",            var_set_int,         NULL            },
    { "thresh_fail",     "11",           var_set_severity,    NULL            },
    { "tmp_dir",          SQSH_TMP,      NULL,                NULL            },
    { "username",         NULL,          NULL,                NULL            },
    { "version",          SQSH_VERSION,  var_set_readonly,    NULL            },
    { "width",            NULL,          var_set_width,       var_get_width   },
    { "xgeom",            NULL,          var_set_xgeom,       var_get_xgeom   },
    { "date",             "%d-%b-%y",    var_set_notempty,    var_get_date    },
    { "time",             "%H:%M:%S",    var_set_notempty,    var_get_time    },
    /* sqsh-2.1.6 - New variables */
    { "appname",          "sqsh-2.1.7",  var_set_nullstr,     NULL            },
    { "histunique",       "0",           var_set_bool,        NULL            },
    { "ignoreeof",        "0",           var_set_bool,        NULL            },
    { "login_timeout",    NULL,          var_set_nullint,     NULL            },
    { "query_timeout",    NULL,          var_set_nullint,     NULL            },
    { "max_timeout",      NULL,          var_set_nullint,     NULL            },
    { "keytab_file",      NULL,          var_set_nullstr,     NULL            },
    { "principal",        NULL,          var_set_nullstr,     NULL            },
    { "secmech",          NULL,          var_set_nullstr,     NULL            },
    { "secure_options",   NULL,          var_set_nullstr,     NULL            },
    /* sqsh-2.1.7 - New variables */
    { "exit_value",       "0",           var_set_int,         NULL            },
    { "hist_auto_save",   "0",           var_set_int,         NULL            },
    { "xwin_title",       NULL,          var_set_nullstr,     NULL            },
    { "term_title",       NULL,          var_set_nullstr,     NULL            },
} ;

#endif /* SQSH_INIT */

#endif /* var_h_included */
