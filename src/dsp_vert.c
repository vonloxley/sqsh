/*
 * dsp_vert.c - Display result set vertically ("qisql" mode)
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
static char RCS_Id[] = "$Id: dsp_vert.c,v 1.2 2004/04/11 15:14:32 mpeppler Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

static void dsp_wrap _ANSI_ARGS(( dsp_out_t*, int, int, char*, int ));

/*
 * dsp_vert:
 *
 * Display routine to display the current result set in a readable
 * sideways format.
 */
int dsp_vert( output, cmd, flags )
	dsp_out_t   *output;
	CS_COMMAND  *cmd;
	int          flags;
{
	int      col_width;           /* Width of current column */
	int      screen_width;        /* Width of the screen in characters */
	int      i, j;                /* Iterators */
	char    *col_data_ptr;        /* Pointer to textual data */
	int      col_data_len;        /* Length of data in column */
	int      dsp_return;          /* Return value from this function */

	char     *aggregate_op;     /* Name of aggregate operator */
	CS_INT    result_type,      /* Type of result set coming back */
	          ret;              /* Success or failure of db call */
	
	int       max_col_width = 0;
	CS_INT    rows_affected = CS_NO_COUNT;
	CS_INT    result_status;
	CS_INT    nrows;

	dsp_col_t  *select_col;           /* Single column from select list */
	dsp_desc_t *select_desc  = NULL;  /* Description of select result set */
	dsp_desc_t *compute_desc = NULL;  /* Description of compute result set */

	screen_width = g_dsp_props.p_width;
	
	/*
	 * Start blasting through result sets...
	 */
	while ((ret = ct_results( cmd, &result_type )) != CS_END_RESULTS)
	{
		DBG(sqsh_debug(DEBUG_DISPLAY, "dsp_vert: Result %s\n",
		               dsp_result_name( result_type ));)

		/*-- Abort on interrupt --*/
		if (g_dsp_interrupted)
			goto dsp_interrupted;
		
		/*-- Ingore any faulty return codes --*/
		if (ret != CS_SUCCEED)
		{
			goto dsp_fail;
		}
		
		/*
		 * Depending on the result type we choose how to display the
		 * data.  For most result sets we mearly ignore them, we really
		 * only care about data and compute results.
		 */
		switch (result_type) 
		{
			case CS_CMD_DONE:
				if (rows_affected != CS_NO_COUNT)
				{
					DBG(sqsh_debug(DEBUG_DISPLAY, "dsp_vert: Fetching affected.\n");)

					if (ct_res_info( cmd,                       /* Command */
					                 CS_ROW_COUNT,              /* Type */
					                 (CS_VOID*) &rows_affected, /* Buffer */
					                 CS_UNUSED,                 /* Buffer Length */
					                 (CS_INT*)NULL ) != CS_SUCCEED)
					{
						DBG(sqsh_debug(DEBUG_DISPLAY, 
						               "dsp_vert: ct_res_info failed.\n" );)
						goto dsp_fail;
					}
				}
				break;

			case CS_STATUS_RESULT:

				while ((ret = ct_fetch( cmd,              /* Command */
				                  CS_UNUSED,        /* Type */
				                  CS_UNUSED,        /* Offset */
				                  CS_UNUSED,        /* Option */
				                  &nrows )) == CS_SUCCEED)
				{
					if (ct_get_data( cmd,                      /* Command */
					                 1,                        /* Item */
					                 (CS_VOID*)&result_status, /* Buffer */
					                 CS_SIZEOF(CS_INT),        /* Buffer Length */
					                 (CS_INT*)NULL ) != CS_END_DATA)
					{
						DBG(sqsh_debug(DEBUG_DISPLAY, 
						               "dsp_vert: ct_get_data failed.\n" );)
						goto dsp_fail;
					}
				}

				if (ret != CS_END_DATA)
				{
					goto dsp_fail;
				}

				if ((flags & DSP_F_NOFOOTERS) == 0)
				{
					if (rows_affected != CS_NO_COUNT)
					{
						dsp_fprintf( output, "(%d row%s affected, return status = %d)\n",
						         (int)rows_affected,
						         (rows_affected == 1) ? "" : "s",
						         (int)result_status );
					}
					else
					{
						dsp_fprintf( output, "(return status = %d)\n", 
						         (int)result_status );
					}
				}
				rows_affected = CS_NO_COUNT;

				break;
			
			case CS_COMPUTE_RESULT:

				if (compute_desc != NULL)
				{
					dsp_desc_destroy( compute_desc );
				}

				/*
				 * First, we must get the description for the compute
				 * result set, we wil lthen use this in conjunction
				 * with the select row description during the output.
				 */
				compute_desc = dsp_desc_bind( cmd, result_type );

				if (compute_desc == NULL)
				{
					goto dsp_fail;
				}

				/*
				 * Calculate which column has the widest column name, so's 
				 * we can print things out all pretty like.
				 */
				max_col_width = 0;
				for (i = 0; i < compute_desc->d_ncols; ++i)
				{
					select_col = &select_desc->d_cols[
						compute_desc->d_cols[i].c_column_id - 1];
					max_col_width = max( max_col_width, 
					                     select_col->c_format.namelen );
				}

				/*
				 * Now we begin fetching our compute result set.
				 */
				while ((ret = dsp_desc_fetch( cmd, select_desc )) == CS_SUCCEED) 
				{
					if (g_dsp_interrupted) 
					{
						goto dsp_interrupted;
					}

					for (i = 0; i < compute_desc->d_ncols; i++)
					{
						col_width = 0;

						/*
						 * If the user asked us to print headers, then we do so,
						 * otherwise we just fill in blanks where the headers
						 * would have gone.
						 */
						if (!(flags & DSP_F_NOHEADERS))
						{
							switch (compute_desc->d_cols[i].c_aggregate_op)
							{
								case CS_OP_SUM:
									aggregate_op = (compute_desc->d_bylist_size == 0) ?
														"SUM" : "sum";
									break;
								case CS_OP_AVG:
									aggregate_op = (compute_desc->d_bylist_size == 0) ?
														"AVG" : "avg";
									break;
								case CS_OP_COUNT:
									aggregate_op = (compute_desc->d_bylist_size == 0) ?
														"CNT" : "cnt";
									break;
								case CS_OP_MIN:
									aggregate_op = (compute_desc->d_bylist_size == 0) ?
														"MIN" : "min";
									break;
								case CS_OP_MAX:
									aggregate_op = (compute_desc->d_bylist_size == 0) ?
														"MAX" : "max";
									break;
								default:
									aggregate_op = "UNK";
							}
								
							dsp_fputs( aggregate_op, output );
							dsp_fputc( '(', output );

							/*
							 * Look up the column from the select list from which
							 * this compute column is derived.
							 */
							select_col = &select_desc->d_cols[
								compute_desc->d_cols[i].c_column_id - 1];
							
							if (select_col->c_format.namelen > 0)
							{
								col_width += select_col->c_format.namelen;
								dsp_fputs( select_col->c_format.name, output );
							}

							dsp_fputs( "): ", output );

						}
						else
						{
							dsp_fputc( ' ', output );
						}

						for (j = col_width; j < max_col_width; j++)
							dsp_fputc( ' ', output );

						if (compute_desc->d_cols[i].c_nullind != 0)
						{
							col_data_ptr = "NULL";
							col_data_len = 4;
						}
						else
						{
							col_data_ptr = compute_desc->d_cols[i].c_data;
							col_data_len = strlen( col_data_ptr );
						}

						/*-- Display output --*/
						dsp_wrap( output, max_col_width+1, screen_width,
						          col_data_ptr, col_data_len );
						dsp_fputc( '\n', output );
					}

					if (g_dsp_interrupted) 
					{
						goto dsp_interrupted;
					}
				}

				if (ret != CS_END_DATA)
				{
					goto dsp_fail;
				}

				break;
					
			case CS_PARAM_RESULT:
			case CS_ROW_RESULT:

				if (result_type == CS_PARAM_RESULT &&
					g_dsp_props.p_outputparms == 0)
				{
					while ((ret = ct_fetch( cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED,
						&nrows )) == CS_SUCCEED);
					
					if (ret != CS_END_DATA)
					{
						goto dsp_fail;
					}

					break;
				}

				if (rows_affected != CS_NO_COUNT)
				{
					if ((flags & DSP_F_NOFOOTERS) == 0)
					{
						dsp_fprintf( output, "(%d row%s affected)\n", (int)rows_affected,
									(rows_affected == 1) ? "" : "s" );
					}
				}
				rows_affected = 0;

				/*
				 * Since we need to keep this result set around in case we
				 * have a compute result set, then we need to destroy and
				 * recreate it during row results.
				 */
				if (select_desc != NULL)
				{
					dsp_desc_destroy( select_desc );
				}

				/*
				 * Fetch the description of the result set and bind to the
				 * results.
				 */
				select_desc = dsp_desc_bind( cmd, result_type );

				if (select_desc == NULL) 
				{
					goto dsp_fail;
				}
				
				/*
				 * Calculate which column has the widest column name, so's 
				 * we can print things out all pretty like.
				 */
				max_col_width = 0;
				for (i = 0; i < select_desc->d_ncols; ++i)
				{
					max_col_width = max( max_col_width, 
					                     select_desc->d_cols[i].c_format.namelen );
				}

				while ((ret = dsp_desc_fetch( cmd, select_desc )) == CS_SUCCEED)
				{
					if (g_dsp_interrupted) 
					{
						goto dsp_interrupted;
					}

					for (i = 0; i < select_desc->d_ncols; i++)
					{
						col_width = 0;

						/*
						 * If the user asked us to print headers, then we do so,
						 * otherwise we just fill in blanks where the headers
						 * would have gone.
						 */
						if (!(flags & DSP_F_NOHEADERS))
						{
							if (select_desc->d_cols[i].c_format.namelen > 0) 
							{
								col_width += select_desc->d_cols[i].c_format.namelen;
								dsp_fputs( select_desc->d_cols[i].c_format.name, output );
							}

							dsp_fputs( ": ", output );
						} 
						else
						{
							dsp_fputc( ' ', output );
						}

						/*
						 * Pad the line out to the width of the longest column
						 * name.
						 */
						for (j = col_width; j < max_col_width; j++)
							dsp_fputc( ' ', output );

						if (select_desc->d_cols[i].c_nullind != 0)
						{
							col_data_ptr = "NULL";
							col_data_len = 4;
						}
						else
						{
							col_data_ptr = select_desc->d_cols[i].c_data;
							col_data_len = strlen( col_data_ptr );
						}

						/*-- Display output --*/
						dsp_wrap( output, max_col_width+1, screen_width,
						          col_data_ptr, col_data_len );
					}

					dsp_fputs( " \n", output );

					if (g_dsp_interrupted) 
					{
						goto dsp_interrupted;
					}
				}

				if (ret != CS_END_DATA)
				{
					goto dsp_fail;
				}
				break;

			default:
				break;

		} /* switch (result_type) */

	} /* while (ct_results()) */

	if (rows_affected != CS_NO_COUNT && !(flags & DSP_F_NOFOOTERS))
	{
		printf("(%d row%s affected)\n", (int)rows_affected,
		       (rows_affected == 1) ? "" : "s");
	}

	dsp_return = DSP_SUCCEED;
	goto dsp_leave;

dsp_interrupted:
	dsp_return = DSP_INTERRUPTED;
	goto dsp_leave;

dsp_fail:
	dsp_return = DSP_FAIL;

dsp_leave:
	if (select_desc != NULL)
		dsp_desc_destroy( select_desc );
	if (compute_desc != NULL)
		dsp_desc_destroy( compute_desc );

	return dsp_return;
}

/*
 * dsp_wrap():
 *
 */
static void dsp_wrap( output, indent, width, str, str_len )
	dsp_out_t *output;
	int        indent;
	int        width;
	char      *str;
	int        str_len;
{
	char *max_end;            /* Pointer to longest possible end point */
	char *end;                /* Pointer to actual end point */
	char *new_start;
	int   first_loop = 1;
	int   i;

	/*
	 * sqsh-2.1.7 - If str is an empty string, just print a newline.
	 * When this happens, it is actually some sort of error, that is,
	 * we received no data at all for a non-nullable column.
	 * Shouldn't we print some sort of error message, or just continue as if
	 * nothing happened?
	*/
	if (str_len == 0) {
		dsp_fputc( '\n', output );
		return;
	}

	/*
	 * Our "real" usable width is the total screen width minus the
	 * amount we need to indent on every line.
	 */
	width -= indent;

	while (str_len > 0) {

		/*
		 * The first time through this loop, don't bother to indent
		 * the line, only on subsequent loops do we do this.
		 */
		if (first_loop)
			first_loop = 0;
		else {
			for (i = 0; i < indent; i++)
				dsp_fputc( ' ', output );
		}

		/*-- Calculate maximum possible length of str --*/
		max_end = str + min(str_len, width);

		/*-- See if we have a '\n' anywhere --*/
		for (end = str; end < max_end && *end != '\n'; ++end);

		/*
		 * If, after all that, end is still pointing the the very last
		 * character that we could possibly print, then we want to do
		 * a little word wrapping.
		 */
		if (end == (str + width)) {
			while (end != str && !(isspace((int)*end)))
				--end;
			
			/*
			 * If we couldn't find a white-space to break on, then give
			 * up and just break mid-word.
			 */
			if (end == str)
				end = max_end;
			
			new_start = end;
		} else 
			new_start = end + 1;

		str_len -= (int)(new_start - str);
		while (str != end)
			dsp_fputc( *str++, output );
		dsp_fputc( '\n', output );

		str = new_start;
		while (str_len > 0 && *str != '\n' && isspace((int)*str)) {
			--str_len;
			++str;
		}
	}
}


