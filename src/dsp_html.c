/*
 * dsp_horiz.c - Display result set horizontally (standard isql mode)
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
static char RCS_Id[] = "$Id: dsp_html.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes --*/
static void       dsp_comp_prrow
	_ANSI_ARGS((dsp_out_t*, dsp_desc_t*, dsp_desc_t*));
static CS_INT     dsp_comp_prrow_one
	_ANSI_ARGS((dsp_out_t*, dsp_desc_t*, dsp_desc_t*));
static char*      dsp_comp_aggregate
	_ANSI_ARGS((CS_INT));
static dsp_col_t* dsp_comp_find
	_ANSI_ARGS((dsp_desc_t*, CS_INT, CS_INT));

/*
 * dsp_html:
 *
 * Displays a result set as a html formated table.
 */
int dsp_html( output, cmd, flags )
	dsp_out_t   *output;
	CS_COMMAND  *cmd;
	int          flags;
{
	int      dsp_return = DSP_SUCCEED;    /* Return value for this function */
	CS_INT   rows_affected = CS_NO_COUNT; /* Number of rows affected */
	CS_INT   result_status = 0;           /* Return status of stored proc */
	CS_INT   nrows;                       /* Rows returned by fetch */
	CS_INT   result_type;                 /* Current result set type */
	CS_INT   ret;                         /* Return value from ct_results() */
	CS_INT   last_row_result = 0;         /* Last result that displayed stuff */
	CS_BOOL  in_table = CS_FALSE;         /* Are we dumping a table? */
	CS_INT   i;
	dsp_col_t  *col;

	/*
	 * The following variables must be cleaned up before returning
	 * from this function (see the nasty nest of goto's at the bottom
	 * of this function).
	 */
	dsp_desc_t  *select_desc = NULL;     /* Description of regular result set */
	dsp_desc_t  *compute_desc = NULL;    /* Description of computed result set */

	/*
	 * That's it for the setup, now start banging through the set of
	 * results coming back from the server.
	 */
	while ((ret = ct_results( cmd, &result_type )) != CS_END_RESULTS)
	{
		/*-- Check for interrupt --*/
		if (g_dsp_interrupted)
			goto dsp_interrupted;

		/*-- Ingore bad return codes --*/
		if (ret != CS_SUCCEED)
		{
			goto dsp_fail;
		}

		/*
		 * Depending on which result set is being returned from the server
		 * we need to alter what we display.  For most result sets, we
		 * simply ignore them, we really only care about data and compute
		 * results.
		 */
		switch (result_type)
		{
			case CS_CMD_DONE:
				if (rows_affected != CS_NO_COUNT && 
					 last_row_result != CS_STATUS_RESULT &&
				    (flags & DSP_F_NOFOOTERS) == 0)
				{
					dsp_fputs( "\n<p>\n", output );
					dsp_fprintf( output, "   (%d row%s affected)\n", (int)rows_affected,
							 (rows_affected == 1) ? "" : "s");
					dsp_fputs( "</p>\n", output );
				}

				if (ct_res_info( cmd,                       /* Command */
				                 CS_ROW_COUNT,              /* Type */
				                 (CS_VOID*)&rows_affected,  /* Buffer */
				                 CS_UNUSED,                 /* Buffer Length */
				                 (CS_INT*)NULL ) != CS_SUCCEED)
				{
					goto dsp_fail;
				}
				break;

			case CS_STATUS_RESULT:
				if (in_table == CS_TRUE)
				{
					dsp_fputs( "</table>\n", output );
					in_table = CS_FALSE;
				}

				while ((ret = ct_fetch( cmd,              /* Command */
				                  CS_UNUSED,        /* Type */
				                  CS_UNUSED,        /* Offset */
				                  CS_UNUSED,        /* Option */
				                  &nrows )) == CS_SUCCEED)
				{
					if (g_dsp_interrupted)
						goto dsp_interrupted;

					if (ct_get_data( cmd,                      /* Command */
					                 1,                        /* Item */
					                 (CS_VOID*)&result_status, /* Buffer */
					                 CS_SIZEOF(CS_INT),        /* Buffer Length */
					                 (CS_INT*)NULL ) != CS_END_DATA)
					{
						goto dsp_fail;
					}
				}

				if (ret != CS_END_DATA)
				{
					goto dsp_fail;
				}

				if ((flags & DSP_F_NOFOOTERS) == 0)
				{
					dsp_fprintf( output,"\n<p>\n" );
					if (rows_affected != CS_NO_COUNT)
					{
						dsp_fprintf( output,"   (%d row%s affected, return status = %d)\n",
						         (int)rows_affected,
						         (rows_affected == 1) ? "" : "s",
						         (int)result_status );
					}
					else
					{
						dsp_fprintf( output, "   (return status = %d)\n", 
						         (int)result_status );
					}
					dsp_fprintf( output,"</p>\n" );
				}

				last_row_result = CS_STATUS_RESULT;
				rows_affected = CS_NO_COUNT;
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

				if (in_table == CS_TRUE)
				{
					dsp_fputs( "</table>\n", output );
				}

				if (rows_affected != CS_NO_COUNT && !(flags & DSP_F_NOFOOTERS))
				{
					dsp_fputs( "\n<p>\n", output );
					dsp_fprintf( output, "   (%d row%s affected)\n", (int)rows_affected,
							 (rows_affected == 1) ? "" : "s");
					dsp_fputs( "</p>\n", output );
				}
				last_row_result = result_type;
				rows_affected = CS_NO_COUNT;

				/*
				 * After a row result is processed, the description is kept
				 * around for use when and if the compute set comes along.
				 * Because of this, we need to check to see if we have already
				 * allocated a description...if we have, then destroy it.
				 */
				if (select_desc != NULL)
				{
					dsp_desc_destroy( select_desc );
				}

				/*
				 * Pull in a description of the result set, this also binds
				 * each row to its own internal data space.
				 */
				select_desc = dsp_desc_bind( cmd, result_type );

				if (select_desc == NULL)
					goto dsp_fail;
				
				dsp_fputs( "\n<table border>\n", output );
				in_table = CS_TRUE;
				
				/*
				 * Display our headers.
				 */
				if ((flags & DSP_F_NOHEADERS) == 0)
				{
					if (g_dsp_interrupted)
						goto dsp_interrupted;
					
					dsp_fputs( "<tr>\n", output );
					for (i = 0; i < select_desc->d_ncols; i++)
					{
						col = &select_desc->d_cols[i];

						dsp_fputs( "   <th>", output );
						if (col->c_format.namelen > 0)
						{
							dsp_fputs( col->c_format.name, output );
						}
						else
						{
							dsp_fputs( "<br>", output );
						}
						dsp_fputs( "</th>\n", output );
					}
					dsp_fputs( "</tr>\n", output );
				}

				/*
				 * Then, while there is data to fetch, display the
				 * data for each row as it comes back.
				 */
				while ((ret = dsp_desc_fetch( cmd, select_desc )) == CS_SUCCEED)
				{
					if (g_dsp_interrupted)
						goto dsp_interrupted;

					dsp_fputs( "<tr>\n", output );
					for (i = 0; i < select_desc->d_ncols; i++)
					{
						col = &select_desc->d_cols[i];

						if (col->c_justification == DSP_JUST_RIGHT)
						{
							dsp_fputs( "   <td align=right>", output );
						}
						else
						{
							dsp_fputs( "   <td align=left>", output );
						}

						if (col->c_nullind == -1)
						{
							dsp_fputs( "<br>", output );
						}
						else
						{
							dsp_fputs( col->c_data, output );
						}

						dsp_fputs( "</td>\n", output );
					}
					dsp_fputs( "</tr>\n", output );
				}

				if (ret != CS_END_DATA)
				{
					goto dsp_fail;
				}
				break;
			
			case CS_COMPUTE_RESULT:
				last_row_result = CS_COMPUTE_RESULT;

				/*
				 * If we have a compute description laying around from
				 * a previous result set, then destroy it.
				 */
				if (compute_desc != NULL)
				{
					dsp_desc_destroy( compute_desc );
				}

				/*
				 * Pull in a description of the result set, this also binds
				 * each row to its own internal data space.
				 */
				compute_desc = dsp_desc_bind( cmd, result_type );

				if (compute_desc == NULL)
					goto dsp_fail;

				while ((ret = dsp_desc_fetch( cmd, compute_desc )) == CS_SUCCEED)
				{
					if (g_dsp_interrupted)
						goto dsp_interrupted;

					dsp_comp_prrow( output, select_desc, compute_desc );
				}

				if (ret != CS_END_DATA)
				{
					goto dsp_fail;
				}
				break;
			
			default:
				break;
		}

	}

	if (in_table == CS_TRUE)
	{
		dsp_fputs( "</table>\n", output );
	}

	if (last_row_result != CS_STATUS_RESULT && rows_affected != CS_NO_COUNT && 
	    !(flags & DSP_F_NOFOOTERS))
	{
		dsp_fputs( "\n<p>\n", output );
		dsp_fprintf( output, "   (%d row%s affected)\n", (int)rows_affected,
		       (rows_affected == 1) ? "" : "s");
		dsp_fputs( "</p>\n", output );
	}

	dsp_return = DSP_SUCCEED;
	goto dsp_leave;

dsp_fail:
	dsp_return = DSP_FAIL;
	goto dsp_leave;

dsp_interrupted:
	dsp_return = DSP_INTERRUPTED;

dsp_leave:
	if (compute_desc != NULL)
		dsp_desc_destroy( compute_desc );
	if (select_desc != NULL)
		dsp_desc_destroy( select_desc );

	return dsp_return;
}

static void dsp_comp_prrow( output, sel_desc, com_desc )
	dsp_out_t   *output;
	dsp_desc_t  *sel_desc;
	dsp_desc_t  *com_desc;
{
	CS_INT       ncols = 0;
	CS_INT       i;

	/*
	 * The c_processed flag is used by dsp_comp_prhead_one to
	 * determine if a compute column has been processed yet,
	 * so we want to clear this flag before continuing.
	 */
	for (i = 0; i < com_desc->d_ncols; i++)
	{
		com_desc->d_cols[i].c_processed = False;
	}

	while (ncols < com_desc->d_ncols)
	{
		ncols += dsp_comp_prrow_one( output, sel_desc, com_desc );
	}
}

/*
 * dsp_comp_prrow_one():
 *
 * For each column in the sel_desc (select result set description)
 * a header for each compute column (com_desc) that represents a 
 * a select column is displayed. Note that only one header will be
 * displayed per select column...if more than one compute column
 * exists, dsp_comp_prhead() must be called again.  This function
 * returns the number of compute columns displayed.
 */
static CS_INT dsp_comp_prrow_one( output, sel_desc, com_desc )
	dsp_out_t   *output;
	dsp_desc_t  *sel_desc;
	dsp_desc_t  *com_desc;
{
	char        *aggregate_op;     /* Name of aggregate operator */
	CS_INT       i;
	CS_INT       count = 0;
	dsp_col_t    *sel_col;
	dsp_col_t    *com_col;

	dsp_fputs( "<tr>\n", output );
	for (i = 0; i < sel_desc->d_ncols; i++)
	{
		dsp_fputs( "   <th>", output );

		/*-- Handy Pointer --*/
		sel_col = &sel_desc->d_cols[i];

		/*
		 * Attempt to find the first compute column that hasn't been
		 * displayed yet that represents this select column, it isn't
		 * a big deal if there isn't any.
		 */
		com_col = dsp_comp_find( com_desc, i+1, DSP_PROC_HEADER );

		/*-- Display aggregate operator name --*/
		if (com_col != NULL)
		{
			aggregate_op = dsp_comp_aggregate( com_col->c_aggregate_op );
			dsp_fputs( aggregate_op, output );
		}
		else
		{
			dsp_fputs( "<br>", output );
		}

		dsp_fputs( "</th>", output );
	}
	dsp_fputs( "</tr>\n", output );

	dsp_fputs( "<tr>\n", output );
	for (i = 0; i < sel_desc->d_ncols; i++)
	{
		/*-- Handy Pointer --*/
		sel_col = &sel_desc->d_cols[i];

		if (sel_col->c_justification == DSP_JUST_RIGHT)
		{
			dsp_fputs( "   <td align=right>", output );
		}
		else
		{
			dsp_fputs( "   <td align=left>", output );
		}

		/*
		 * Look up the value for this select column by looking it up
		 * from the list of unprocessed compute columns.
		 */
		com_col = dsp_comp_find( com_desc, i+1, DSP_PROC_DATA );

		if (com_col != NULL)
		{
			++count;

			if (com_col->c_nullind == -1)
			{
				dsp_fputs( "<br>", output );
			}
			else
			{
				dsp_fputs( com_col->c_data, output );
			}
		}
		else
		{
			dsp_fputs( "<br>", output );
		}
	}
	dsp_fputs( "</tr>\n", output );

	return count;
}

/*
 * dsp_comp_aggregate():
 *
 * Converts an aggregate type into a readable string.
 */
static char* dsp_comp_aggregate( aggregate_op )
	CS_INT   aggregate_op;
{
	switch (aggregate_op)
	{
		case CS_OP_SUM:   return "sum";
		case CS_OP_AVG:   return "avg";
		case CS_OP_COUNT: return "count";
		case CS_OP_MIN:   return "min";
		case CS_OP_MAX:   return "max";
		default:
			break;
	}

	return "";
}

/*
 * dsp_comp_find():
 *
 * Given a column number from a select list, dsp_comp_find() looks
 * up the column from the compute column description "comp" that
 * represents an aggregate of the col_id. NULL is returned if no
 * such compute column exists.
 */
static dsp_col_t* dsp_comp_find( comp, col_id, proc_type )
	dsp_desc_t  *comp;
	CS_INT       col_id;
	CS_INT       proc_type;
{
	CS_INT     i;

	for (i = 0; i < comp->d_ncols; i++)
	{
		if ((comp->d_cols[i].c_processed & proc_type) != proc_type)
		{
			if (comp->d_cols[i].c_column_id == col_id)
			{
				comp->d_cols[i].c_processed |= proc_type;
				return &comp->d_cols[i];
			}
			else
			{
				return (dsp_col_t*)NULL;
			}
		}
	}

	return (dsp_col_t*)NULL;
}


