/*
 * dsp.c - Interface to result set display management
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
#include "sqsh_global.h"
#include "sqsh_error.h"
#include "sqsh_sig.h"
#include "dsp.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: dsp.c,v 1.2 2005/07/24 11:41:19 mpeppler Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * g_dsp_interrupted:  This variable is set by the dsp_signal() signal
 *      handler and is to be polled by all display routines to determine
 *      if a signal has been recieved.
 */
int         g_dsp_interrupted  = False;

/*
 * sg_cmd: Used to keep track of which command is currently
 * being worked upon. This is used to determine what needs to
 * be canceled.
 */
static CS_COMMAND *sg_cmd = NULL;

/*
 * g_dsp_props: This data structure contains the current set of 
 *      properties defined for the display sub-system. Be careful
 *      when editing this data strcuture.
 */
dsp_prop_t  g_dsp_props = {
	DSP_HORIZ,       /* p_style */
	80,              /* p_width */
	" ",             /* p_colsep */
	1,               /* p_colsep_len */
	"\n\t",          /* p_linesep */
	8,               /* p_linesep_len */
	"",              /* p_xgeom */
	18,              /* p_flt_prec */
	6,               /* p_flt_scale */
	18,              /* p_real_prec */
	6,               /* p_real_scale */
	32,              /* p_colwidth */
	1,               /* p_outputparms */
	"|",             /* p_bcp_colsep */
	1,               /* p_bcp_colsep_len */
	"|",             /* p_bcp_rowsep */
	1,               /* p_bcp_rowsep_len */
	1,               /* p_bcp_trim */
	32768,            /* p_maxlen */
	",",             /* p_csv_colsep */
	1,               /* p_csv_colsep_len */
};

/*-- Prototypes --*/
static int   dsp_prop_set _ANSI_ARGS(( int, void*, int ));
static int   dsp_prop_get _ANSI_ARGS(( int, void*, int ));
static int   dsp_vlen     _ANSI_ARGS(( char* ));
static void  dsp_signal   _ANSI_ARGS(( int, void* ));

/*
 * dsp_prop():
 *
 * Performs action (wither DSP_GET or DSP_SET) upon the display
 * property identified by prop, placing the results into the
 * location pointed to by ptr, of length len.
 */
int dsp_prop( action, prop, ptr, len )
	int     action;
	int     prop;
	void   *ptr;
	int     len;
{
	if (!DSP_VALID_PROP( prop ))
	{
		sqsh_set_error( SQSH_E_INVAL, "Invalid property type" );
		return DSP_FAIL;
	}

	if (action == DSP_SET)
	{
		return dsp_prop_set( prop, ptr, len );
	}
	else if (action == DSP_GET)
	{
		return dsp_prop_get( prop, ptr, len );
	}

	sqsh_set_error( SQSH_E_INVAL, "Invalid action type" );
	return DSP_FAIL;
}

/*
 * dsp_cmd():
 *
 * Displays the result set of the CS_COMMAND in the current display
 * style.
 */
int dsp_cmd( output, cmd, sql, flags )
	FILE          *output;
	CS_COMMAND    *cmd;
	char          *sql;
	int            flags;
{
	CS_CONNECTION *con;
	int            ret = DSP_SUCCEED;
	dsp_t         *dsp_func;
	dsp_out_t     *o   = NULL;

	/*
	 * In order to install our callbacks, we must first track down
	 * the connection that owns this command.
	 */
	if (ct_cmd_props( cmd,               /* Command */
	                  CS_GET,            /* Action */
	                  CS_PARENT_HANDLE,  /* Property */
	                  (CS_VOID*)&con,    /* Buffer */
	                  CS_UNUSED,         /* Buffer Length */
	                  (CS_INT*)NULL ) != CS_SUCCEED)
		return DSP_FAIL;

	/*
	 * Convert the FILE* into one of our handy-dandy signal
	 * safe buffered output thingies.
	 */
	o = dsp_fopen( output );

	if (o == NULL)
	{
		return DSP_FAIL;
	}

	/*
	 * Mark the fact that we haven't received an interrupt yet (in
	 * fact we haven't installed our signal handlers yet).
	 */
	g_dsp_interrupted = False;
	sg_cmd = cmd;

	/*
	 * Install our new signal handlers.  From here on out, if one of
	 * these signal is received, the g_dsp_interrupted flag will be
	 * set.
	 */
	DBG(sqsh_debug(DEBUG_DISPLAY,"dsp_print: Installing signal handlers\n");)

	/*
	 * Now, install our signal handlers.
	 */
	sig_save();

	sig_install( SIGINT, dsp_signal, (void*)NULL, 0 );
	sig_install( SIGPIPE, dsp_signal, (void*)NULL, 0 );

	if (ct_send( cmd ) != CS_SUCCEED)
		ret = DSP_FAIL;

	if (g_dsp_interrupted)
		ret = DSP_INTERRUPTED;

	/*
	 * All that is left is to process the results of the current
	 * query, reinstall the old signal handlers, and return!
	 */
	if (ret == DSP_SUCCEED) 
	{
		if ((flags & DSP_F_NOTHING) != 0)
		{
			dsp_func = dsp_none;
		}
		else
		{
			switch (g_dsp_props.p_style)
			{
				case DSP_META:
					dsp_func = dsp_meta;
					break;
				case DSP_PRETTY:
					dsp_func = dsp_pretty;
					break;
				case DSP_HORIZ:
					dsp_func = dsp_horiz;
					break;
				case DSP_VERT:
					dsp_func = dsp_vert;
					break;
				case DSP_HTML:
					dsp_func = dsp_html;
					break;
				case DSP_BCP:
					dsp_func = dsp_bcp;
					break;
				case DSP_NONE:
					dsp_func = dsp_none;
					break;
				case DSP_CSV:
					dsp_func = dsp_csv;
					break;
				default:
					dsp_func = dsp_html;
			}
		}

		if (flags & DSP_F_X)
		{
			ret = dsp_x( o, cmd, flags, dsp_func );
		}
		else
		{
			ret = dsp_func( o, cmd, flags );
		}

	}

	/*
	 * We're done with our output.
	 */
	dsp_fclose( o );

	if (ret != DSP_SUCCEED)
	{
		if (ct_cancel( (CS_CONNECTION*)NULL, cmd, CS_CANCEL_ALL ) != CS_SUCCEED)
		{
			ret =  DSP_FAIL;
		}
	}

	sg_cmd = NULL;

	/*
	 * Uninstall our signal handlers.
	 */
	sig_restore();

	return ret;
}

/*
 * dsp_vlen():
 *
 * Returns the "visual" length of a given string.  That is, it takes into
 * account characters, such as tab, that may expand when displayed on the
 * screen.
 */
static int dsp_vlen( str )
	char    *str;
{
	int    len = 0;

	for (; *str != '\0'; ++str)
	{
		switch (*str)
		{
			case '\n':
				len = 0;
				break;
			case '\t':
				len += 8;
				break;
			default:
				++len;
		}
	}

	return len;
}

/*
 * dsp_prop_set()
 *
 * Set the display property to the value pointed to by ptr of length
 * len.  Upon success DSP_SUCCEED is returned, upon failure DSP_FAIL
 * is returned with an error condition set.
 */
static int dsp_prop_set( prop, ptr, len )
	int    prop;
	void  *ptr;
	int    len;
{
	switch (prop)
	{
		case DSP_DATEFMT:
			return dsp_datefmt_set( (char*)ptr );
			break;

		case DSP_COLWIDTH:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_COLWDTH, %d)\n", *((int*)ptr));)

			if (*((int*)ptr) < 1)
			{
				sqsh_set_error( SQSH_E_EXIST, "Invalid column width\n" );
				return DSP_FAIL;
			}

			g_dsp_props.p_colwidth = *((int*)ptr);
			break;
		
		case DSP_FLOAT_PREC:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_FLOAT_PREC, %d)\n", *((int*)ptr));)

			if (*((int*)ptr) < 0 || *((int*)ptr) < g_dsp_props.p_flt_scale)
			{
				sqsh_set_error( SQSH_E_EXIST, "Invalid float precision" );
				return DSP_FAIL;
			}

			g_dsp_props.p_flt_prec = *((int*)ptr);
			break;

		case DSP_FLOAT_SCALE:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_FLOAT_SCALE, %d)\n",
				*((int*)ptr));)

			if (*((int*)ptr) < 0 || *((int*)ptr) >= g_dsp_props.p_flt_prec)
			{
				sqsh_set_error( SQSH_E_EXIST, "Invalid float scale" );
				return DSP_FAIL;
			}

			g_dsp_props.p_flt_scale = *((int*)ptr);
			break;

		case DSP_REAL_PREC:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_REAL_PREC, %d)\n", *((int*)ptr));)

			if (*((int*)ptr) < 0 || *((int*)ptr) < g_dsp_props.p_real_scale)
			{
				sqsh_set_error( SQSH_E_EXIST, "Invalid real precision" );
				return DSP_FAIL;
			}

			g_dsp_props.p_real_prec = *((int*)ptr);
			break;

		case DSP_REAL_SCALE:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_REAL_SCALE, %d)\n",
				*((int*)ptr));)

			if (*((int*)ptr) < 0 || *((int*)ptr) >= g_dsp_props.p_real_prec)
			{
				sqsh_set_error( SQSH_E_EXIST, "Invalid real scale" );
				return DSP_FAIL;
			}

			g_dsp_props.p_real_scale = *((int*)ptr);
			break;

		case DSP_STYLE:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_STYLE, %d)\n", *((int*)ptr));)

			if (!(DSP_VALID_STYLE( *((int*)ptr) )))
			{
				sqsh_set_error( SQSH_E_EXIST, "Invalid display style" );
				return DSP_FAIL;
			}

			g_dsp_props.p_style = *((int*)ptr);
			break;

		case DSP_WIDTH: 
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_WIDTH, %d)\n", *((int*)ptr));)

			if (*((int*)ptr) >= 30)
			{
				g_dsp_props.p_width = *((int*)ptr);
			}
			else
			{
				sqsh_set_error( SQSH_E_INVAL, "Width must be >= 30" );
				return DSP_FAIL;
			}
			break;

		case DSP_OUTPUTPARMS: 
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_OUTPUTPARMS, %d)\n",
				*((int*)ptr));)

			if (*((int*)ptr) == 1 || *((int*)ptr) == 0)
			{
				g_dsp_props.p_outputparms = *((int*)ptr);
			}
			else
			{
				sqsh_set_error( SQSH_E_INVAL, "Output parameter must be 1 or 0" );
				return DSP_FAIL;
			}
			break;
		
		case DSP_COLSEP:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_COLSEP, '%s')\n", 
				(ptr == NULL)?"NULL":((char*)ptr));)

			if (len == DSP_NULLTERM)
			{
				len = strlen( (char*)ptr );
			}

			if (len > MAX_SEPLEN || len < 0)
			{
				sqsh_set_error( SQSH_E_INVAL, 
					"Invalid length of column separator (between 0 and %d allowed)",
					MAX_SEPLEN );
				return DSP_FAIL;
			}

			strncpy( g_dsp_props.p_colsep, (char*)ptr, len );

			g_dsp_props.p_colsep[len] = '\0';
			g_dsp_props.p_colsep_len = dsp_vlen( g_dsp_props.p_colsep );

			break;

		case DSP_BCP_COLSEP:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_BCP_COLSEP, '%s')\n", 
				(ptr == NULL)?"NULL":((char*)ptr));)

			if (len == DSP_NULLTERM)
			{
				len = strlen( (char*)ptr );
			}

			if (len > MAX_SEPLEN || len < 0)
			{
				sqsh_set_error( SQSH_E_INVAL, 
					"Invalid length of bcp col separator (between 0 and %d allowed)",
					MAX_SEPLEN );
				return DSP_FAIL;
			}

			strncpy( g_dsp_props.p_bcp_colsep, (char*)ptr, len );

			g_dsp_props.p_bcp_colsep[len] = '\0';
			g_dsp_props.p_bcp_colsep_len = dsp_vlen( g_dsp_props.p_bcp_colsep );

			break;

		case DSP_BCP_ROWSEP:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_BCP_ROWSEP, '%s')\n", 
				(ptr == NULL)?"NULL":((char*)ptr));)

			if (len == DSP_NULLTERM)
			{
				len = strlen( (char*)ptr );
			}

			if (len > MAX_SEPLEN || len < 0)
			{
				sqsh_set_error( SQSH_E_INVAL, 
					"Invalid length of bcp row separator (between 0 and %d allowed)",
					MAX_SEPLEN );
				return DSP_FAIL;
			}

			strncpy( g_dsp_props.p_bcp_rowsep, (char*)ptr, len );

			g_dsp_props.p_bcp_rowsep[len] = '\0';
			g_dsp_props.p_bcp_rowsep_len = dsp_vlen( g_dsp_props.p_bcp_rowsep );

			break;

		case DSP_BCP_TRIM:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_BCP_TRIM, '%d')\n", 
				(ptr == NULL)?-1:*((int*)ptr));)

			if (*((int*)ptr) == 0)
			{
				g_dsp_props.p_bcp_trim = False;
			}
			else
			{
				g_dsp_props.p_bcp_trim = True;
			}
			break;
		
		case DSP_LINESEP:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_LINESEP, '%s')\n", 
				(ptr == NULL)?"NULL":((char*)ptr));)

			if (len == DSP_NULLTERM)
			{
				len = strlen( (char*)ptr );
			}

			if (len > MAX_SEPLEN || len < 0)
			{
				sqsh_set_error( SQSH_E_INVAL, 
					"Invalid length of line separator (between 0 and %d allowed)",
					MAX_SEPLEN );
				return DSP_FAIL;
			}

			strncpy( g_dsp_props.p_linesep, (char*)ptr, len );

			g_dsp_props.p_linesep[len] = '\0';
			g_dsp_props.p_linesep_len = dsp_vlen( g_dsp_props.p_linesep );
			break;

		case DSP_XGEOM:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_XGEOM, '%s')\n", 
				(ptr == NULL)?"NULL":((char*)ptr));)

			if (len == DSP_NULLTERM)
			{
				len = strlen( (char*)ptr );
			}

			if (len > MAX_XGEOM || len < 0)
			{
				sqsh_set_error( SQSH_E_INVAL, 
					"Invalid length of X geometry (between 0 and %d allowed)",
					MAX_XGEOM );
				return DSP_FAIL;
			}

			strncpy( g_dsp_props.p_xgeom, (char*)ptr, len );
			g_dsp_props.p_xgeom[len] = '\0';
			break;

		case DSP_MAXLEN:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_SET, DSP_MAXLEN, %d)\n", *((int*)ptr));)

			if (*((int*)ptr) < 0)
			{
				sqsh_set_error( SQSH_E_EXIST, "Invalid width specified\n" );
				return DSP_FAIL;
			}

			g_dsp_props.p_maxlen = *((int*)ptr);
			break;
		
		default:
			sqsh_set_error( SQSH_E_EXIST, "Invalid property type" );
			return DSP_FAIL;
	}

	return DSP_SUCCEED;
}

/*
 * dsp_prop_get()
 *
 * Fetch the display property into the value pointed to by ptr of length
 * len.  Upon success DSP_SUCCEED is returned, upon failure DSP_FAIL
 * is returned with an error condition set.
 */
static int dsp_prop_get( prop, ptr, len )
	int   prop;
	void *ptr;
	int   len;
{
	switch (prop)
	{
		case DSP_COLWIDTH:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_COLWIDTH) = %d\n", 
			   g_dsp_props.p_colwidth);)

			*((int*)ptr) = g_dsp_props.p_colwidth;
			break;

		case DSP_FLOAT_PREC:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_FLOAT_PREC) = %d\n", 
			   g_dsp_props.p_flt_prec);)

			*((int*)ptr) = g_dsp_props.p_flt_prec;
			break;

		case DSP_FLOAT_SCALE:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_FLOAT_SCALE) = %d\n", 
			   g_dsp_props.p_flt_scale);)

			*((int*)ptr) = g_dsp_props.p_flt_scale;
			break;

		case DSP_REAL_PREC:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_REAL_PREC) = %d\n", 
			   g_dsp_props.p_real_prec);)

			*((int*)ptr) = g_dsp_props.p_real_prec;
			break;

		case DSP_REAL_SCALE:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_REAL_SCALE) = %d\n", 
			   g_dsp_props.p_real_scale);)

			*((int*)ptr) = g_dsp_props.p_real_scale;
			break;

		case DSP_DATEFMT:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_DATEFMT) = %s\n", 
				dsp_datefmt_get());)

			if (len <= 0)
			{
				sqsh_set_error( SQSH_E_INVAL, "length must be greater than 0" );
				return DSP_FAIL;
			}

			strncpy( (char*)ptr, dsp_datefmt_get(), len );
			break;
			
		case DSP_STYLE:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_STYLE) = %d\n", 
				g_dsp_props.p_style);)
		
			*((int*)ptr) = g_dsp_props.p_style;
			break;

		case DSP_WIDTH: 
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_WIDTH) = %d\n", 
				g_dsp_props.p_width);)

			*((int*)ptr) = g_dsp_props.p_width;
			break;

		case DSP_OUTPUTPARMS: 
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_OUTPUTPARMS) = %d\n", 
				g_dsp_props.p_outputparms);)

			*((int*)ptr) = g_dsp_props.p_outputparms;
			break;
		
		case DSP_COLSEP:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_COLSEP) = %s\n", 
				g_dsp_props.p_colsep);)

			strncpy( (char*)ptr, g_dsp_props.p_colsep, len );
			break;

		case DSP_BCP_COLSEP:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_BCP_COLSEP) = %s\n", 
				g_dsp_props.p_bcp_colsep);)

			strncpy( (char*)ptr, g_dsp_props.p_bcp_colsep, len );
			break;

		case DSP_BCP_ROWSEP:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_BCP_ROWSEP) = %s\n", 
				g_dsp_props.p_bcp_rowsep);)

			strncpy( (char*)ptr, g_dsp_props.p_bcp_rowsep, len );
			break;

		case DSP_BCP_TRIM:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_BCP_TRIM) = %d\n", 
				g_dsp_props.p_bcp_trim);)

			*((int*)ptr) = g_dsp_props.p_bcp_trim;
			break;
		
		case DSP_LINESEP:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_LINESEP) = %s\n", 
				g_dsp_props.p_linesep);)

			strncpy( (char*)ptr, g_dsp_props.p_linesep, len );
			break;

		case DSP_XGEOM:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_XGEOM) = %s\n", 
				g_dsp_props.p_xgeom);)

			if (len <= 0)
			{
				sqsh_set_error( SQSH_E_INVAL, "length must be greater than 0" );
				return DSP_FAIL;
			}

			strncpy( (char*)ptr, g_dsp_props.p_xgeom, len );
			break;

		case DSP_MAXLEN:
			DBG(sqsh_debug(DEBUG_DISPLAY, 
				"dsp_prop: dsp_prop(DSP_GET, DSP_MAXLEN) = %d\n", 
			   g_dsp_props.p_maxlen);)

			*((int*)ptr) = g_dsp_props.p_maxlen;
			break;
		
		default:
			sqsh_set_error( SQSH_E_EXIST, "Invalid property type" );
			return DSP_FAIL;
	}

	return DSP_SUCCEED;
}


/*
 * dsp_signal():
 *
 * This function is called whenever a SIGINT signal is received, its
 * only real job is to register the fact that a signal was caught
 * during processing via the g_dsp_interrupted flag.
 */
static void dsp_signal( sig, user_data )
	int   sig;
	void *user_data;
{
	if (g_dsp_interrupted)
		return;

	DBG(sqsh_debug(DEBUG_DISPLAY,"dsp_signal: Received %s\n",
		(sig == SIGINT) ? "SIGINT" : "SIGPIPE" );)

	/*
	 * Unlike most signal handlers in sqsh, we don't want to longjmp()
	 * out of here...it seems that ctlib really freaks out on some
	 * systems if an interrupt happens and you leave it in a wierd
	 * state.
	 */
	g_dsp_interrupted = True;

	if (sg_cmd != NULL)
	{
		if (ct_cancel((CS_CONNECTION*)NULL, sg_cmd, CS_CANCEL_ATTN)
			!= CS_SUCCEED)
		{
			fprintf( stderr,"dsp_signal: Error from ct_cancel(CS_CANCEL_ATTN)\n" );
		}
	}
}

#if defined(DEBUG)

char* dsp_result_name ( result_type )
	CS_INT result_type;
{
	switch (result_type)
	{
		case CS_ROW_RESULT:        return "CS_ROW_RESULT";
		case CS_CURSOR_RESULT:     return "CS_CURSOR_RESULT";
		case CS_PARAM_RESULT:      return "CS_PARAM_RESULT";
		case CS_STATUS_RESULT:     return "CS_STATUS_RESULT";
		case CS_MSG_RESULT:        return "CS_MSG_RESULT";
		case CS_COMPUTE_RESULT:    return "CS_COMPUTE_RESULT";
		case CS_CMD_DONE:          return "CS_CMD_DONE";
		case CS_CMD_SUCCEED:       return "CS_CMD_SUCCEED";
		case CS_CMD_FAIL:          return "CS_CMD_FAIL";
		case CS_ROWFMT_RESULT:     return "CS_ROWFMT_RESULT";
		case CS_COMPUTEFMT_RESULT: return "CS_COMPUTEFMT_RESULT";
		case CS_DESCRIBE_RESULT:   return "CS_DESCRIBE_RESULT";
		default:                   break;
	}

	return "UNKNOWN_RESULTS";
}

#endif /* DEBUG */
