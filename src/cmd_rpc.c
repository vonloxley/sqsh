/*
 * cmd_rcp.c - Execute a remote procedure call in a server
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
#include "sqsh_global.h"
#include "sqsh_expand.h"
#include "sqsh_error.h"
#include "sqsh_varbuf.h"
#include "sqsh_getopt.h"
#include "sqsh_env.h"
#include "sqsh_cmd.h"
#include "sqsh_sig.h"
#include "sqsh_stdin.h"
#include "dsp.h"
#include "cmd.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: cmd_rpc.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

typedef struct rpc_param_st {
	CS_INT                rp_nbytes;
	CS_VOID              *rp_buf;
	struct rpc_param_st  *rp_nxt;
} rpc_param_t;

/*-- Prototypes --*/
static int     is_binary         _ANSI_ARGS(( char* ));
static int     is_money          _ANSI_ARGS(( char* ));
static int     is_float          _ANSI_ARGS(( char* ));
static int     is_int            _ANSI_ARGS(( char* ));
static rpc_param_t* rpc_param  
	_ANSI_ARGS(( CS_COMMAND*, CS_CHAR*, CS_DATAFMT*, CS_INT ));

/*
 * cmd_rpc():
 *
 * Implements a method to call a stored procedure within a SQL Server
 * or an Open Server.  This command takes as argument a line that look
 * something like:
 *
 *    \rpc rpc_name [@var=]value [@var2=]value [@var3=]var_name
 *
 * Where var_name indicates the name of a sqsh variable to be used for
 * the output parameter of the stored procedure.
 */
int cmd_rpc( argc, argv )
	int   argc;
	char *argv[];
{
	extern int        sqsh_optind;          /* Required by sqsh_getopt */
	extern char*      sqsh_optarg;          /* Required by sqsh_getopt */
	char             *headers;              /* Value of environment var */
	char             *footers;              /* Dito. */
	int               ch;
	int               have_error             = False;
	int               dsp_flags     = 0;
	int               dsp_old       = -1;
	CS_INT            options       = CS_UNUSED;
	CS_INT            i,j;
	CS_DATAFMT        fmt;
	CS_SMALLINT       nullind;
	CS_CHAR          *pptr;
	CS_INT            return_code;

	/*
	 * Variables that need to be cleaned up before returning
	 * from this function.
	 */
	rpc_param_t      *param, *param_list = NULL;
	CS_COMMAND       *cmd                = NULL;
	char             *dsp_name           = NULL;

	/*
	 * Since we will be temporarily replacing some of our global
	 * settings, we want to set up a save-point to which we can
	 * restore when we are done.
	 */
	env_tran( g_env );

	/*
	 * Parse the command line options. Note the special "-" modifier
	 * to the command line flags, this indicates that all occurances
	 * of -o are to be left in place as if they were non-flags.
	 */
	while ((ch = sqsh_getopt( argc, argv, "rfhm:w:d:x;u-i-d-b-n-m-c-o-" )) 
		!= EOF) 
	{
		switch (ch) 
		{
			case 'r' :
#if defined(CS_RECOMPILE)
				options = CS_RECOMPILE;
#endif /* CS_RECOMPILE */
				break;

			case 'w' :
				if (env_put( g_env, "width", sqsh_optarg, ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\go: -h: %s\n", sqsh_get_errstr() );
				}
				break;

			case 'd' :
				if (env_put( g_env, "DISPLAY", sqsh_optarg, ENV_F_TRAN ) == False)
				{
					fprintf( stderr, "\\go: -d: %s\n", sqsh_get_errstr() );
					have_error = True;
				}
				break;

			case 'x' :
				dsp_flags |= DSP_F_X;

				if (sqsh_optarg != NULL)
				{
					if (env_put( g_env, "xgeom", sqsh_optarg, ENV_F_TRAN ) == False)
					{
						fprintf( stderr, "\\go: -x: %s\n", sqsh_get_errstr() );
						have_error = True;
					}
				}
				break;
			case 'f' :
				dsp_flags |= DSP_F_NOFOOTERS;
				break;
			case 'm' :
				dsp_name = sqsh_optarg;
				break;
			case 'h' :
				dsp_flags |= DSP_F_NOHEADERS;
				break;

			default :
				fprintf( stderr, "\\rpc: %s\n", sqsh_get_errstr() );
				have_error = True;
		}
	}

	/*
	 * If there isn't at least the name of the stored procedure left on
	 * the command line, then generate an error.
	 */
	if ((argc - sqsh_optind) < 1 || have_error) 
	{
		fprintf( stderr,
			"Use: \\rpc [dsp_options] rpc_name [[parm_options]\n"
			"          [@var=]value ...]\n"
			"  General Options\n"
			"     -r         Recompile the procedure\n"
			"  Display Options\n"
			"     -f         Suppress headers\n"
			"     -h         Suppress footers\n"
			"     -m mode    Switch display mode for result set\n"
			"     -x [xgeom] X display\n"
			"     -w width   Display width\n"
			"\n"
			"  Parameter Options\n"
			"     -b        Value is in binary (hex) format\n"
			"     -c        Value is in string format\n"
			"     -d        Value is a double (float)\n"
			"     -i        Value is an int\n"
			"     -m        Value is a money\n"
			"     -n        Value is a numeric\n"
			"     -o        Output value\n"
			"     -u        NULL value\n"
			"\n"
			"  @var         Optional name of rpc parameter\n"
			"  value        Value for rpc parameter\n"
		);

		env_rollback( g_env );
		return CMD_FAIL;
	}

	/*
	 * Retrieve the value of any variables that we want to use for
	 * configuration.
	 */
	env_get( g_env, "headers", &headers );
	env_get( g_env, "footers", &footers );

	if (headers != NULL && *headers == '0')
		dsp_flags |= DSP_F_NOHEADERS;
	if (footers != NULL && *footers == '0')
		dsp_flags |= DSP_F_NOFOOTERS;

	/*
	 * Allocate a new command structure.
	 */
	if (ct_cmd_alloc( g_connection, &cmd ) != CS_SUCCEED)
	{
		env_rollback( g_env );
		return CMD_FAIL;
	}
	
	/*
	 * If the user requests that we use a different display style
	 * than the one currently set, then we need to set the requested
	 * one.  We don't need to check the return value from dsp_set_name()
	 * because we already verified the name with dsp_valid() above.
	 */
	if (dsp_name != NULL) 
	{
		if (dsp_prop( DSP_GET,
		              DSP_STYLE,
		              (void*)&dsp_old,
		              DSP_UNUSED ) != DSP_SUCCEED)
		{
			fprintf( stderr, "\\rpc: %s\n", sqsh_get_errstr() );
			goto cmd_rpc_fail;
		}

		if (env_put( g_env, "style", dsp_name, ENV_F_TRAN ) != True)
		{
			fprintf( stderr, "\\rpc: %s\n", sqsh_get_errstr() );
			goto cmd_rpc_fail;
		}
	}


	/*
	 * Set up the rpc call.
	 */
	if (ct_command( cmd, CS_RPC_CMD, (CS_VOID*)argv[sqsh_optind],
		CS_NULLTERM, options ) != CS_SUCCEED)
	{
		goto cmd_rpc_fail;
	}


	/*
	 * Now, process the remainder of the arguments on the command
	 * line.
	 */
	i = sqsh_optind + 1;

	while (i < argc)
	{
		fmt.name[0]   = '\0';
		fmt.namelen   = 0;
		fmt.datatype  = (CS_INT)-1;
		fmt.maxlength = 0;
		fmt.status    = CS_INPUTVALUE;
		fmt.format    = CS_FMT_UNUSED;
		fmt.locale    = NULL;

		nullind = CS_GOODDATA;

		while (i < argc && argv[i][0] == '-' && argv[i][2] == '\0')
		{
			switch (argv[i][1])
			{
				case 'o':
					fmt.status = CS_RETURN;
					break;

				case 'u':
					nullind = CS_NULLDATA;
					break;

				case 'i':
					fmt.datatype  = CS_INT_TYPE;
					fmt.maxlength = CS_SIZEOF(CS_INT);
					break;
				
				case 'd':
					fmt.datatype  = CS_FLOAT_TYPE;
					fmt.maxlength = CS_SIZEOF(CS_FLOAT);
					break;
				
				case 'b':
					fmt.datatype  = CS_BINARY_TYPE;
					fmt.maxlength = 255;
					break;
				
				case 'n':
					fmt.datatype  = CS_NUMERIC_TYPE;
					fmt.maxlength = CS_SIZEOF(CS_NUMERIC);
					fmt.scale     = 6;
					fmt.precision = 17;
					break;
				
				case 'y':
					fmt.datatype  = CS_MONEY_TYPE;
					fmt.maxlength = CS_SIZEOF(CS_MONEY);
					break;
				
				case 'c':
					fmt.datatype  = CS_CHAR_TYPE;
					fmt.format    = CS_FMT_NULLTERM;
					fmt.maxlength = 255;
					break;
				
				default:
					fprintf( stderr, "\\rpc: Invalid flag `%s' specified\n",
						argv[i] );
					goto cmd_rpc_fail;
			}

			++i;
		}

		/*
		 * Try to capture the name of the variable.
		 */
		if (i < argc)
		{
			pptr = argv[i];

			if (*pptr == '@')
			{
				j = 0;
				while (*pptr != '\0' && *pptr != '=')
				{
					fmt.name[j++] = *pptr++;
				}
				fmt.name[j] = '\0';
				fmt.namelen = CS_NULLTERM;
			}

			if (*pptr == '=')
			{
				++pptr;
			}

			if (*pptr == '\0')
			{
				nullind = CS_NULLDATA;
			}
		}
		else
		{
			pptr = NULL;
		}

		if (nullind == CS_NULLDATA)
		{
			param = rpc_param( cmd, NULL, &fmt, (CS_INT)CS_NULLDATA );
		}
		else
		{
			param = rpc_param( cmd, pptr, &fmt, (CS_INT)CS_GOODDATA );
		}

		if (param == NULL)
		{
			goto cmd_rpc_fail;
		}

		param->rp_nxt = param_list;
		param_list    = param;

		++i;
	}

	i = dsp_cmd( stdout, cmd, NULL, dsp_flags );

	switch (i)
	{
		case DSP_INTERRUPTED:
			goto cmd_rpc_interrupt;
		case DSP_FAIL:
			goto cmd_rpc_fail;
		case DSP_SUCCEED:
			break;
		default:
			fprintf( stderr, "\\rpc: Invalid return code from dsp_cmd!\n" );
			goto cmd_rpc_fail;
	}

	goto cmd_rpc_succeed;

	/*
	 * The following jump points are used by the various parts of
	 * cmd_rpc, above, to deal with various styles of returning.
	 */
cmd_rpc_interrupt:
	if (!sqsh_stdin_isatty())
		return_code = CMD_ABORT;
	else
		return_code = CMD_CLEARBUF;

	goto cmd_rpc_leave;

cmd_rpc_fail:
	ct_cancel( (CS_CONNECTION*)NULL, cmd, CS_CANCEL_ALL );

	/*
	 * If something went wrong, and clear_on_fail is 1, then request
	 * that the work buffer be cleared.
	 */
	return_code = CMD_FAIL;
	goto cmd_rpc_leave;

cmd_rpc_succeed:
	return_code = CMD_CLEARBUF;

cmd_rpc_leave:
	if (dsp_old != -1)
	{
		dsp_prop( DSP_SET, DSP_STYLE, (void*)&dsp_old, DSP_UNUSED );
	}

	/*
	 * Restore the environment variables back to their
	 * original state.
	 */
	env_rollback( g_env );

	while (param_list != NULL)
	{
		param = param_list->rp_nxt;
		if (param_list->rp_buf != NULL)
		{
			free( param_list->rp_buf );
		}
		free( param_list );
		param_list = param;
	}

	ct_cmd_drop( cmd );

	return return_code;
}

static rpc_param_t* rpc_param( cmd, str, fmt, nullind )
	CS_COMMAND   *cmd;
	CS_CHAR      *str;
	CS_DATAFMT   *fmt;
	CS_INT        nullind;
{
	CS_DATAFMT     srcfmt;
	rpc_param_t   *param;
	CS_INT         len;

	if (fmt->datatype == (CS_INT)-1)
	{
		if (nullind == CS_NULLDATA)
		{
			fmt->datatype = CS_INT_TYPE;
		}
		else if (is_binary( str ))
		{
			fmt->datatype  = CS_BINARY_TYPE;
			fmt->maxlength = 255;
		}
		else if (is_money( str ))
		{
			fmt->datatype  = CS_MONEY_TYPE;
			fmt->maxlength = CS_SIZEOF(CS_MONEY);
		}
		else if (is_float( str ))
		{
			fmt->datatype = CS_FLOAT_TYPE;
			fmt->maxlength = CS_SIZEOF(CS_FLOAT);
		}
		else if (is_int( str ))
		{
			fmt->datatype  = CS_INT_TYPE;
			fmt->maxlength = CS_SIZEOF(CS_INT);
		}
		else
		{
			fmt->datatype  = CS_CHAR_TYPE;
			fmt->format    = CS_FMT_NULLTERM;
			fmt->maxlength = 255;
		}
	}

	srcfmt.datatype  = CS_CHAR_TYPE;
	srcfmt.maxlength = (str == NULL) ? 0 : (CS_INT)strlen(str);
	srcfmt.format    = CS_FMT_NULLTERM;
	srcfmt.status    = 0;
	srcfmt.locale    = NULL;

	param = (rpc_param_t*)malloc(sizeof(rpc_param_t));

	if (param == NULL)
	{
		fprintf( stderr, "rpc_param: Memory allocation failure\n" );
		return NULL;
	}

	if (nullind == (CS_INT)CS_NULLDATA)
	{
		param->rp_nbytes = 0;
		param->rp_buf    = NULL;
		len              = 0;
	}
	else
	{
		param->rp_buf = (CS_VOID*)malloc(fmt->maxlength);

		if (param->rp_buf == NULL)
		{
			free(param->rp_buf);
			fprintf( stderr, "rpc_param: Memory allocation failure\n" );
			return NULL;
		}

		param->rp_nbytes = fmt->maxlength;
		param->rp_nxt    = NULL;

		if (cs_convert( g_context, &srcfmt, (CS_VOID*)str, fmt, param->rp_buf,
			&len ) != CS_SUCCEED)
		{
			fprintf( stderr, "rpc_param: Error converting `%s'\n",
				str );
			free( param->rp_buf );
			free( param );
			return NULL;
		}
		len = (CS_INT)strlen(str);
	}

	fmt->precision = 0;
	fmt->scale = 0;
	if (ct_param( cmd, fmt, param->rp_buf, len, 
		(CS_SMALLINT)nullind ) != CS_SUCCEED)
	{
		fprintf( stderr, "rpc_param: Error creating parameter\n" );
		free( param->rp_buf );
		free( param );
		return NULL;
	}

	return param;
}

/*
 * is_binary():
 *
 * Used by to identify if string could represent an binary string.
 * Pretty much this tests to see if the string starts with 0x and
 * is followed by only hexidecimal numbers.
 */
static int is_binary( str )
	char *str;
{
	/*-- Skip white space --*/
	while (*str != '\0' && isspace((int)*str))
		++str;

	/*-- Must have 0x --*/
	if (*str == '\0')
		return False;

	if (*str != '0' || *(str+1) != 'x')
		return False;

	/*-- Make sure everything following the 0x is a hex digit --*/
	for( str += 2; *str != '\0' && 
	     strchr("0123456789abcdefABCDEF", *str) != NULL; ++str );

	/*-- Skip white space --*/
	while (*str != '\0' && isspace((int)*str))
		++str;

	return (*str == '\0');
}

/*
 * is_float():
 *
 * Attempts to determine if str represents a floating point value. Which
 * is considered [+-]0.0, [+-]0.0e[+-]0, [+-]0e[+-0].
 */
static int is_float( str )
	char *str;
{
	/*-- Skip white space --*/
	while (*str != '\0' && isspace((int)*str))
		++str;

	/*-- Must have at least one digit --*/
	if (*str == '\0')
		return False;

	/*-- Skip leading sign --*/
	if (*str == '+' || *str == '-')
		++str;

	/*-- Check all of the digits --*/
	for( ++str; *str != '\0' && isdigit((int)*str); ++str );

	if (*str != '.' && *str != 'e' && *str != 'E')
		return False;

	if (*str == '.') {
		/*-- Must have digit following decimal --*/
		if (!isdigit((int)*(str+1)))
			return False;

		/*-- Digits following decimal place --*/
		for( str += 2; *str != '\0' && isdigit((int)*str); ++str );
	}

	if (*str == 'e' || *str == 'E') {
		++str;
		if (*str == '+' || *str == '-')
			++str;

		/*-- Digits in exponent place --*/
		for( ++str; *str != '\0' && isdigit((int)*str); ++str );
	}

	/*-- Skip white space --*/
	while (*str != '\0' && isspace((int)*str))
		++str;

	return (*str == '\0');
}

/*
 * is_int():
 *
 * Used to check to see if str could possibly represent an integer.
 * An integer is considered (in regex) [ ]*[+-]?[0-9]+[ ]*.
 */
static int is_int( str )
	char *str;
{
	/*-- Skip white space --*/
	while (*str != '\0' && isspace((int)*str))
		++str;

	/*-- Must have at least one digit --*/
	if (*str == '\0')
		return False;

	/*-- Skip leading sign --*/
	if (*str == '+' || *str == '-')
		++str;

	/*-- Check all of the digits --*/
	for( ++str; *str != '\0' && isdigit((int)*str); ++str );

	/*-- Skip white space --*/
	while (*str != '\0' && isspace((int)*str))
		++str;

	return (*str == '\0');
}

/*
 * is_money():
 *
 * Used to check to see if str could possibly represent an money
 * value (i.e. has a '$').
 */
static int is_money( str )
	char *str;
{
	/*-- Skip white space --*/
	while (*str != '\0' && isspace((int)*str))
		++str;

	/*-- Skip leading sign --*/
	if (*str == '+' || *str == '-')
		++str;

	/*-- Must have dollar sign --*/
	if (*str != '$')
		return False;

	/*-- Skip trailing sign --*/
	if (*str == '+' || *str == '-')
		++str;

	/*-- Check all of the digits --*/
	for( ++str; *str != '\0' && isdigit((int)*str) && *str == '.'; ++str );

	/*-- Skip white space --*/
	while (*str != '\0' && isspace((int)*str))
		++str;

	return (*str == '\0');
}

