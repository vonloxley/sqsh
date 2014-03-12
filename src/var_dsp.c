/*
 * var_dbproc.c - Variables that affect the global CS_CONNECTION
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
#include "sqsh_env.h"
#include "sqsh_error.h"
#include "sqsh_fd.h"
#include "var.h"
#include "dsp.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: var_dsp.c,v 1.6 2014/01/18 18:36:34 mwesdorp Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

int var_set_fltfmt( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	int    prec;
	int    scale;
	char   number[64];
	char  *v, *n;

	v = *var_value;
	n = number;
	while (isdigit((int)*v))
	{
		*n++ = *v++;
	}
	*n = '\0';

	if (*v != '.')
	{
		sqsh_set_error( SQSH_E_INVAL, "Expected <precision>.<scale>" );
		return False;
	}
	prec = atoi(number);

	++v;
	n = number;
	while (isdigit((int)*v))
	{
		*n++ = *v++;
	}
	*n = '\0';

	if (*v != '\0')
	{
		sqsh_set_error( SQSH_E_INVAL, "Expected <precision>.<scale>" );
		return False;
	}
	scale = atoi(number);

	if (strcmp( var_name, "float" ) == 0)
	{
		if (dsp_prop( DSP_SET, DSP_FLOAT_PREC, (void*)&prec, DSP_UNUSED ) != DSP_SUCCEED)
		{
			return False;
		}

		if (dsp_prop( DSP_SET, DSP_FLOAT_SCALE, (void*)&scale, DSP_UNUSED ) != DSP_SUCCEED)
		{
			return False;
		}
	}
	else
	{
		if (dsp_prop( DSP_SET, DSP_REAL_PREC, (void*)&prec, DSP_UNUSED ) != DSP_SUCCEED)
		{
			return False;
		}

		if (dsp_prop( DSP_SET, DSP_REAL_SCALE, (void*)&scale, DSP_UNUSED ) != DSP_SUCCEED)
		{
			return False;
		}
	}

	return True;
}

/*
 * sqsh-2.1.9 - Implement date and time datatype conversion routines.
 * New and modified functions: var_set_datetime, var_get_datetime,
 * var_set_datefmt, var_get_datefmt, var_set_timefmt, var_get_timefmt.
 */
int var_set_datetime( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
        if ( *var_value == NULL || strcasecmp( *var_value, "NULL" ) == 0 || **var_value == '\0' )
                *var_value = NULL ;

	if (dsp_prop( DSP_SET, DSP_DATETIMEFMT, (void*)(*(var_value)), DSP_NULLTERM ) != DSP_SUCCEED)
	{
		return False;
	}

	return True;
}

int var_get_datetime( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	static char date_str[64];

	if (dsp_prop( DSP_GET, DSP_DATETIMEFMT, (void*)date_str, sizeof(date_str) ) != DSP_SUCCEED)
	{
		return False;
	}

	*var_value = date_str;
	return True;
}

int var_set_datefmt( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
        if ( *var_value == NULL || strcasecmp( *var_value, "NULL" ) == 0 || **var_value == '\0' )
                *var_value = NULL ;

	if (dsp_prop( DSP_SET, DSP_DATEFMT, (void*)(*(var_value)), DSP_NULLTERM ) != DSP_SUCCEED)
	{
		return False;
	}

	return True;
}

int var_get_datefmt( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	static char date_str[64];

	if (dsp_prop( DSP_GET, DSP_DATEFMT, (void*)date_str, sizeof(date_str) ) != DSP_SUCCEED)
	{
		return False;
	}

	*var_value = date_str;
	return True;
}

int var_set_timefmt( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
        if ( *var_value == NULL || strcasecmp( *var_value, "NULL" ) == 0 || **var_value == '\0' )
                *var_value = NULL ;

	if (dsp_prop( DSP_SET, DSP_TIMEFMT, (void*)(*(var_value)), DSP_NULLTERM ) != DSP_SUCCEED)
	{
		return False;
	}

	return True;
}

int var_get_timefmt( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	static char date_str[64];

	if (dsp_prop( DSP_GET, DSP_TIMEFMT, (void*)date_str, sizeof(date_str) ) != DSP_SUCCEED)
	{
		return False;
	}

	*var_value = date_str;
	return True;
}

int var_set_style( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	int  style;

	if (*var_value == NULL)
	{
		sqsh_set_error( SQSH_E_INVAL, "Invalid display style 'NULL'" );
		return False;
	}

	if (strcmp( *var_value, "hor" )        == 0 ||
	    strcmp( *var_value, "horiz" )      == 0 ||
	    strcmp( *var_value, "horizontal" ) == 0)
	{
		style = DSP_HORIZ;
	}
	else if (strcmp( *var_value, "vert" )     == 0 ||
	         strcmp( *var_value, "vertical" ) == 0)
	{
		style = DSP_VERT;
	}
	else if (strcmp( *var_value, "bcp" ) == 0)
	{
		style = DSP_BCP;
	}
	else if (strcmp( *var_value, "csv" ) == 0)
	{
		style = DSP_CSV;
	}
	else if (strcmp( *var_value, "html" ) == 0)
	{
		style = DSP_HTML;
	}
	else if (strcmp( *var_value, "none" ) == 0)
	{
		style = DSP_NONE;
	}
	else if (strcmp( *var_value, "meta" ) == 0)
	{
		style = DSP_META;
	}
	else if (strcmp( *var_value, "pretty" ) == 0)
	{
		style = DSP_PRETTY;
	}
	else
	{
		sqsh_set_error( SQSH_E_INVAL, "Invalid display style '%s'", *var_value );
		return False;
	}

	if (dsp_prop( DSP_SET, DSP_STYLE, (void*)&style, DSP_UNUSED ) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN,"var_set_style: Display style now set to %s\n", *var_value);)

	return True;
}

int var_get_style( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	int  style;

	if (dsp_prop( DSP_GET, DSP_STYLE, (void*)&style, DSP_UNUSED ) != DSP_SUCCEED)
	{
		return False;
	}

	switch (style)
	{
		case DSP_HORIZ: 
			*var_value = "horizontal";
			break;
		case DSP_VERT:
			*var_value = "vertical";
			break;
		case DSP_HTML:
			*var_value = "html";
			break;
		case DSP_BCP:
			*var_value = "bcp";
			break;
		case DSP_NONE:
			*var_value = "none";
			break;
		case DSP_META:
			*var_value = "meta";
			break;
		case DSP_PRETTY:
			*var_value = "pretty";
			break;
		case DSP_CSV:
			*var_value = "csv";
			break;
		default:
			*var_value = "unknown";
			break;
	}

	return True;
}

int var_set_colwidth( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	int  colwidth;

	if (var_set_nullint( env, var_name, var_value ) == False)
	{
		return False;
	}

	colwidth = (*var_value != NULL) ? atoi(*var_value) : 30;

	if (dsp_prop( DSP_SET, DSP_COLWIDTH, (void*)&colwidth, DSP_UNUSED ) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN,"var_set_colwidth: Column width now set to %s\n", *var_value);)

	return True ;
}

int var_get_colwidth( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	static char nbr[16];
	int   colwidth;

	if (dsp_prop( DSP_GET, DSP_COLWIDTH, (void*)&colwidth, DSP_UNUSED ) != DSP_SUCCEED)
	{
		*var_value = NULL;
		return False;
	}

	sprintf( nbr, "%d", colwidth );

	*var_value = nbr;
	return True;
}

int var_set_outputparms( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	int  outputparms;

	if (var_set_bool( env, var_name, var_value ) == False)
	{
		return False;
	}

	outputparms = atoi(*var_value);

	if (dsp_prop( DSP_SET, DSP_OUTPUTPARMS, (void*)&outputparms, DSP_UNUSED ) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN, "var_set_output_parms: Output parameters now set to %s\n", *var_value);)

	return True;
}

int var_get_outputparms( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	static char nbr[4];
	int   outputparms;

	if (dsp_prop( DSP_GET, DSP_OUTPUTPARMS, (void*)&outputparms, DSP_UNUSED ) != DSP_SUCCEED)
	{
		*var_value = NULL;
		return False;
	}

	sprintf( nbr, "%d", outputparms );

	*var_value = nbr;
	return True;
}

int var_set_width( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	int  width;

	if (var_set_nullint( env, var_name, var_value ) == False)
	{
		return False;
	}

	width = (*var_value != NULL) ? atoi(*var_value) : 0;

	if (dsp_prop( DSP_SET, DSP_WIDTH, (void*)&width, DSP_UNUSED ) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN,"var_set_width: Width now set to %s\n", *var_value);)

	return True ;
}

int var_get_width( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	static char nbr[16];
	int   width;

	if (dsp_prop( DSP_GET, DSP_WIDTH, (void*)&width, DSP_UNUSED ) != DSP_SUCCEED)
	{
		*var_value = NULL;
		return False;
	}

	sprintf( nbr, "%d", width );

	*var_value = nbr;
	return True;
}

int var_set_colsep( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	if (var_set_esc( env, var_name, var_value ) == False)
	{
		return False;
	}

	if (dsp_prop( DSP_SET, DSP_COLSEP, (void*)(*var_value), DSP_NULLTERM ) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN,"var_set_colsep: Column separator now set to %s\n", *var_value);)

	return True ;
}

int var_get_colsep( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	static char colsep[32];

	if (dsp_prop( DSP_GET, DSP_COLSEP, (void*)colsep, sizeof(colsep) ) != DSP_SUCCEED)
	{
		*var_value = NULL;
		return False;
	}

	*var_value = colsep;
	return True ;
}

int var_set_bcp_colsep( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	if (var_set_esc( env, var_name, var_value ) == False)
	{
		return False;
	}

	if (dsp_prop( DSP_SET, DSP_BCP_COLSEP, (void*)(*var_value), DSP_NULLTERM ) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN,"var_set_bcp_colsep: BCP column separator now set to %s\n", *var_value);)

	return True ;
}

int var_get_bcp_colsep( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	static char bcp_colsep[32];

	if (dsp_prop( DSP_GET, DSP_BCP_COLSEP, (void*)bcp_colsep, sizeof(bcp_colsep) ) != DSP_SUCCEED)
	{
		*var_value = NULL;
		return False;
	}

	*var_value = bcp_colsep;
	return True ;
}

int var_set_bcp_rowsep( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	if (var_set_esc( env, var_name, var_value ) == False)
	{
		return False;
	}

	if (dsp_prop( DSP_SET, DSP_BCP_ROWSEP, (void*)(*var_value), DSP_NULLTERM ) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN,"var_set_bcp_rowsep: BCP row separator now set to %s\n", *var_value);)

	return True ;
}

int var_get_bcp_rowsep( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	static char bcp_rowsep[32];

	if (dsp_prop( DSP_GET, DSP_BCP_ROWSEP, (void*)bcp_rowsep, sizeof(bcp_rowsep) ) != DSP_SUCCEED)
	{
		*var_value = NULL;
		return False;
	}

	*var_value = bcp_rowsep;
	return True ;
}

int var_set_bcp_trim( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	int bcp_trim;

	if (var_set_bool( env, var_name, var_value ) == False)
	{
		return False;
	}

	/* MW: Applied fix from patch 2061950 by Klaus-Martin Hansche. */
	if (strcmp(*var_value , "1") == 0)
	{
		bcp_trim = True;
	}
	else
	{
		bcp_trim = False;
	}

	if (dsp_prop( DSP_SET, DSP_BCP_TRIM, (void*)&bcp_trim, DSP_UNUSED ) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN,"var_set_bcp_trim: BCP trim now set to %s\n", *var_value);)

	return True ;
}

int var_get_bcp_trim( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	int bcp_trim;

	if (dsp_prop( DSP_GET, DSP_BCP_TRIM, (void*)&bcp_trim, DSP_UNUSED ) != DSP_SUCCEED)
	{
		return False;
	}

	if (bcp_trim == True)
	{
		*var_value = "1";
	}
	else
	{
		*var_value = "0";
	}
	return True ;
}

int var_set_linesep( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	if (var_set_esc( env, var_name, var_value ) == False)
	{
		return False;
	}

	if (dsp_prop( DSP_SET, DSP_LINESEP, (void*)(*var_value), DSP_NULLTERM ) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN,"var_set_linesep: Line separator now set to %s\n", *var_value);)

	return True ;
}

int var_get_linesep( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	static char linesep[32];

	if (dsp_prop( DSP_GET, DSP_LINESEP, (void*)linesep, sizeof(linesep) ) != DSP_SUCCEED)
	{
		*var_value = NULL;
		return False;
	}

	*var_value = linesep;
	return True ;
}

int var_set_xgeom( env, var_name, var_value )
	env_t    *env ;
	char     *var_name ;
	char     **var_value ;
{
	char   *cp;
	int     have_error = False;

	if (var_value == NULL || *var_value == NULL)
	{
		return True;
	}

	/*-- Blast through digits --*/
	for (cp = *var_value; *cp != '\0' && isdigit( (int)*cp ); ++cp);

	if (*cp != '\0')
	{
		if (*cp != 'x' && *cp != 'X')
		{
			have_error = True;
		}
		else
		{
			/*-- Blast through digits --*/
			for (++cp; *cp != '\0' && isdigit( (int)*cp ); ++cp);

			if (*cp != '\0')
			{
				have_error = True;
			}
		}
	}

	if (have_error == True)
	{
		sqsh_set_error( SQSH_E_INVAL, 
		        "Invalid geometry, format must be WW or WWxHH" );
		return False;
	}

	if (dsp_prop( DSP_SET, DSP_XGEOM, (void*)(*var_value), DSP_NULLTERM) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN,"var_set_xgeom: Xgeom now set to %s\n", *var_value);)

	return True;
}

int var_get_xgeom( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	static char xgeom[32];

	if (dsp_prop( DSP_GET, DSP_XGEOM, (void*)xgeom, sizeof(xgeom) ) != DSP_SUCCEED)
	{
		*var_value = NULL;
		return False;
	}

	*var_value = xgeom;
	return True ;
}

int var_set_maxlen( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	int  maxlen;

	if (var_set_nullint( env, var_name, var_value ) == False)
	{
		return False;
	}

	maxlen = (*var_value != NULL) ? atoi(*var_value) : 0;

	if (dsp_prop( DSP_SET, DSP_MAXLEN, (void*)&maxlen, DSP_UNUSED ) != DSP_SUCCEED)
	{
		return False;
	}

	DBG(sqsh_debug(DEBUG_SCREEN, "var_set_maxlen: Maximum column width now set to %s\n", *var_value);)

	return True ;
}

int var_get_maxlen( env, var_name, var_value )
	env_t    *env;
	char     *var_name;
	char     **var_value;
{
	static char nbr[16];
	int   maxlen;

	if (dsp_prop( DSP_GET, DSP_MAXLEN, (void*)&maxlen, DSP_UNUSED) != DSP_SUCCEED)
	{
		*var_value = NULL;
		return False;
	}

	sprintf( nbr, "%d", maxlen );

	*var_value = nbr;
	return True;
}
