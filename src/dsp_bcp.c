/*
 * dsp_bcp.c - Display result set in separator delimited mode
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
#include "sqsh_global.h"
#include "sqsh_debug.h"
#include "dsp.h"

extern int errno;

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: dsp_bcp.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

static void dsp_col _ANSI_ARGS(( dsp_out_t*, char*, int ));

/*
 * dsp_bcp:
 *
 * Kind of stupid routine for displaying a result set in a packed
 * format, separated by $colsep only.  No formatting is done.
 *
 */
int dsp_bcp( output, cmd, flags )
	dsp_out_t   *output;
	CS_COMMAND  *cmd;
	int          flags;
{
	int      i;                /* Iterator */

	CS_INT       result_type;  /* Current result set type */
	CS_INT       ret;          /* ct_results return code */
	CS_INT       nrows;        /* Number of rows fetch */
	dsp_desc_t  *desc;         /* Result set description */

	/*
	 * Start blasting through result sets...
	 */
	while ((ret = ct_results( cmd, &result_type )) != CS_END_RESULTS)
	{
		if (g_dsp_interrupted)
			return DSP_INTERRUPTED;

		if (ret != CS_SUCCEED)
			return DSP_FAIL;

		switch (result_type)
		{
			case CS_STATUS_RESULT:
				while ((ret = ct_fetch( cmd,          /* Command */
				                  CS_UNUSED,    /* Type */
				                  CS_UNUSED,    /* Offset */
				                  CS_UNUSED,    /* Option */
				                  &nrows )) == CS_SUCCEED);

				if (ret != CS_END_DATA)
				{
					return DSP_FAIL;
				}
				break;
			
			case CS_PARAM_RESULT:
			case CS_ROW_RESULT:
			case CS_COMPUTE_RESULT:
				if (result_type == CS_PARAM_RESULT &&
					g_dsp_props.p_outputparms == 0)
				{
					while ((ret = ct_fetch( cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED,
						&nrows )) == CS_SUCCEED);
					
					if (ret != CS_END_DATA)
					{
						return DSP_FAIL;
					}

					break;
				}

				desc = dsp_desc_bind( cmd, result_type );

				if (desc == NULL)
					return DSP_FAIL;
				
				while ((ret = dsp_desc_fetch( cmd, desc )) == CS_SUCCEED)
				{
					if (g_dsp_interrupted)
					{
						dsp_desc_destroy( desc );
						return DSP_INTERRUPTED;
					}

					for (i = 0; i < desc->d_ncols; i++)
					{
						if (desc->d_cols[i].c_nullind == 0)
						{
							dsp_col( output, 
										desc->d_cols[i].c_data,
										strlen( desc->d_cols[i].c_data ) );
						}
						else
						{
							dsp_col( output, "", 0 );
						}

						if (i < (desc->d_ncols-1))
						{
							dsp_fputs( g_dsp_props.p_bcp_colsep, output );
						}
					}

					dsp_fputs( g_dsp_props.p_bcp_rowsep, output );
					dsp_fputc( '\n', output );
				}

				dsp_desc_destroy( desc );

				if (ret != CS_END_DATA)
				{
					return DSP_FAIL;
				}
				break;
			
			default:
				break;
		}
	}

	return DSP_SUCCEED;
}

/*
 * dsp_col():
 *
 */
static void dsp_col( output, col_value, col_width )
	dsp_out_t *output;
	char      *col_value;
	int        col_width;
{
	char    *end;

	if (col_width > 0)
	{
		/*
		 * Traverse backwards over any whitespace that we find.
		 */
		end = (col_value + col_width - 1);

		if (g_dsp_props.p_bcp_trim == True)
		{
			for (; end >= col_value && (*end == '\0' || isspace((int)*end));
				--end);
		}

		if (end > col_value || (end == col_value && !isspace((int)*end))) {
			for (; col_value <= end; ++col_value)
				dsp_fputc( *col_value, output );
		}
	}

	return;
}
