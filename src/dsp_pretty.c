/*
 * dsp_pretty.c - Pretty formatted display style
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
static char RCS_Id[] = "$Id: dsp_pretty.c,v 1.2 2004/04/11 15:14:32 mpeppler Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes --*/
static void       dsp_prhead         
	_ANSI_ARGS(( dsp_out_t*, dsp_desc_t* ));
static void       dsp_prsep          
	_ANSI_ARGS(( dsp_out_t*, dsp_desc_t*, int ));
static void       dsp_prrow          
	_ANSI_ARGS(( dsp_out_t*, dsp_desc_t* ));
static dsp_col_t* dsp_comp_find      
	_ANSI_ARGS(( dsp_desc_t*, CS_INT,CS_INT ));
static char*      dsp_comp_aggregate 
	_ANSI_ARGS(( CS_INT ));
static void       dsp_comp_prrow 
	_ANSI_ARGS(( dsp_out_t*, dsp_desc_t*, dsp_desc_t* ));
static CS_INT     dsp_comp_prrow_one
	_ANSI_ARGS(( dsp_out_t*, dsp_desc_t*, dsp_desc_t* ));
static void      dsp_calc_width      
	_ANSI_ARGS(( dsp_desc_t* ));

/*
 * dsp_pretty:
 */
int dsp_pretty( o, cmd, flags )
	dsp_out_t   *o;
	CS_COMMAND  *cmd;
	int          flags;
{
	int       dsp_return = DSP_SUCCEED;    /* Return value for this function */
	CS_INT    rows_affected = CS_NO_COUNT; /* Number of rows affected */
	CS_INT    result_status = 0;           /* Return status of stored proc */
	CS_INT    nrows;                       /* Rows returned by fetch */
	CS_INT    result_type;                 /* Current result set type */
	CS_INT    ret;                         /* Return value from ct_results() */
	CS_INT    last_row_result = 0;         /* Last significant result type */

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
		
		/*-- Abort on non-success return codes --*/
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

				/*
				 * If we have a rows affected count from a previous CMD DONE
				 * that hasn't been displayed yet and we haven't reached a
				 * status result then display the count.
				 */
				if (rows_affected   != CS_NO_COUNT && 
				    last_row_result != CS_STATUS_RESULT &&
				    (flags & DSP_F_NOFOOTERS) == 0)
				{
					if (last_row_result != 0)
					{
						dsp_fputc( '\n', o );
					}

					dsp_fprintf( o, "(%d row%s affected)\n", (int)rows_affected,
							 (rows_affected == 1) ? "" : "s");
				}

				if (ct_res_info( cmd,                       /* Command */
				                 CS_ROW_COUNT,              /* Type */
				                 (CS_VOID*)&rows_affected,  /* Buffer */
				                 CS_UNUSED,                 /* Buffer Length */
				                 (CS_INT*)NULL ) != CS_SUCCEED)
				{
					goto dsp_fail;
				}

				DBG(sqsh_debug(DEBUG_DISPLAY, "dsp_horiz:   CS_ROW_COUNT = %d\n", 
					 (int) rows_affected);)
				break;

			case CS_STATUS_RESULT:
			  while (((ret = ct_fetch( cmd,        /* Command */
				                  CS_UNUSED,        /* Type */
				                  CS_UNUSED,        /* Offset */
				                  CS_UNUSED,        /* Option */
						   &nrows )) == CS_SUCCEED)
				 || (ret == CS_ROW_FAIL))
				{
					if (g_dsp_interrupted)
						goto dsp_interrupted;
					
					if (ret == CS_ROW_FAIL)
					    continue;

					if (ct_get_data( cmd,                      /* Command */
					                 1,                        /* Item */
					                 (CS_VOID*)&result_status, /* Buffer */
					                 CS_SIZEOF(CS_INT),        /* Buffer Length */
					                 (CS_INT*)NULL ) != CS_END_DATA)
					{
						goto dsp_fail;
					}
				}

				/*
				 * Abort on abnormal return value from ct_fetch().
				 */
				if (ret != CS_END_DATA)
				{
					goto dsp_fail;
				}

				if ((flags & DSP_F_NOFOOTERS) == 0)
				{
					/*
					 * If any result type that is displayed to the screen has
					 * been reached, then print a new-line between this message
					 * and the result set.
					 */
					if (last_row_result != 0)
					{
						dsp_fputc( '\n', o );
					}

					if (rows_affected != CS_NO_COUNT)
					{
						dsp_fprintf( o, "(%d row%s affected, return status = %d)\n",
						         (int)rows_affected,
						         (rows_affected == 1) ? "" : "s",
						         (int)result_status );
					}
					else
					{
						dsp_fprintf( o,  "(return status = %d)\n", 
						         (int)result_status );
					}

				}

				last_row_result = CS_STATUS_RESULT;
				rows_affected   = CS_NO_COUNT;
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

				/*
				 * Only display a new-line between consecutive ROW results,
				 * we want to avoid displaying a new-line between a COMPUTE
				 * result and its associated ROW result.
				 */
				if (last_row_result == CS_ROW_RESULT ||
				    last_row_result == CS_STATUS_RESULT)
				{
					dsp_fputc( '\n', o );
				}
				
				if (rows_affected != CS_NO_COUNT && !(flags & DSP_F_NOFOOTERS))
				{
					dsp_fprintf( o,  "(%d row%s affected)\n", (int)rows_affected,
							 (rows_affected == 1) ? "" : "s");
				}
				last_row_result = result_type;
				rows_affected   = CS_NO_COUNT;

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
				{
					goto dsp_fail;
				}

				/*
				 * Now, calculate the width of the columns.
				 */
				dsp_calc_width( select_desc );
				
				/*
				 * Display our headers.
				 */
				if ((flags & DSP_F_NOHEADERS) == 0)
				{
					if (g_dsp_interrupted)
						goto dsp_interrupted;

					dsp_prhead( o, select_desc );
				}

                if (g_dsp_interrupted)
                    goto dsp_interrupted;

				/*
				 * Then, while there is data to fetch, display the
				 * data for each row as it comes back.
				 */
				while ((ret = dsp_desc_fetch( cmd, select_desc )) == CS_SUCCEED)
				{
					if (g_dsp_interrupted)
						goto dsp_interrupted;

					dsp_prrow( o, select_desc );

					if (g_dsp_interrupted)
						goto dsp_interrupted;
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

					dsp_comp_prrow( o, select_desc, compute_desc );

					if (g_dsp_interrupted)
						goto dsp_interrupted;
				}

				if (ret != CS_END_DATA)
				{
					goto dsp_fail;
				}
				break;
			
			default:
				break;
		}

		if (g_dsp_interrupted)
			goto dsp_interrupted;
	}

	if (last_row_result != CS_STATUS_RESULT && rows_affected != CS_NO_COUNT && 
	    !(flags & DSP_F_NOFOOTERS))
	{
		if (last_row_result != 0)
		{
			dsp_fputc( '\n', o );
		}
		dsp_fprintf( o, "(%d row%s affected)\n", (int)rows_affected,
		       (rows_affected == 1) ? "" : "s");
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

static void dsp_calc_width( desc )
	dsp_desc_t   *desc;
{
	dsp_col_t   *col;
	int          total_width = 0;
	int          ntrunc      = 0;
	int          add_width   = 0;
	int          w;
	int          i;
	int          adj_width;

	/*
	 * First, we want to make an initial pass through all of the columns
	 * and assign default widths based upon the actual amount of data
	 * in the column, and the current colwidth limit.
	 */
	for (i = 0; i < desc->d_ncols; i++)
	{
		/*-- Handy Pointer --*/
		col = &desc->d_cols[i];

		col->c_width = max(col->c_format.namelen, col->c_maxlength);

		if (col->c_width > g_dsp_props.p_colwidth)
		{
			++ntrunc;
			col->c_width = g_dsp_props.p_colwidth;
		}

		/*
		 * Total width is the width of the column plus the leading and
		 * trailing spaces, plus the '|' separator.
		 */
		total_width += (col->c_width + 3);
	}

	/*
	 * Add one extra trailing '|' character.
	 */
	++total_width;

	/*
	 * Now, if the total width of the display is less that the width
	 * of the screen and one or more columns were truncated, then we
	 * can widen those columns up to fill the screen.
	 */
	if (total_width < g_dsp_props.p_width && ntrunc > 0)
	{
		add_width = (g_dsp_props.p_width - total_width) / ntrunc;

		for (i = 0; ntrunc > 0 && i < desc->d_ncols; i++)
		{
			col = &desc->d_cols[i];
			w = max(col->c_format.namelen, col->c_maxlength);

			if (col->c_width != w)
			{
				--ntrunc;

				/*
				 * If this is the last column to be lengthened, then just
				 * let it have whatever is left of the screen.
				 */
				if (ntrunc == 0)
				{
					col->c_width = min(col->c_width + 
						(g_dsp_props.p_width - total_width),w);
				}
				else
				{
					adj_width = (min(col->c_width + add_width, w)) - col->c_width;
					col->c_width += adj_width;
					total_width  += adj_width;
				}
			}
		}
	}
}

static void dsp_prsep( output, desc, sep )
	dsp_out_t   *output;
	dsp_desc_t  *desc;
	int          sep;
{
	int           i, j;
	dsp_col_t    *col;

	/*-- Blast through available data --*/
	for (i = 0; i < desc->d_ncols; i++)
	{
		/*-- Handy Pointer --*/
		col = &desc->d_cols[i];

		dsp_fputc( '+', output );
		dsp_fputc( sep, output );

		for (j = 0; j < (col->c_width+1); j++)
		{
			dsp_fputc( sep, output );
		}
	}

	dsp_fputs( "+\n", output );
}
	

/*
 * dsp_prhead():
 *
 * Displays the column headers as described by result set desc, sending
 * the output to stream output.
 */
static void dsp_prhead( output, desc )
	dsp_out_t   *output;
	dsp_desc_t  *desc;
{
	CS_INT       i;
	CS_INT       j;
	dsp_col_t    *col;

	/*
	 * Display a line separator.
	 */
	dsp_prsep( output, desc, (int)'=' );

	/*-- Blast through available data --*/
	for (i = 0; i < desc->d_ncols; i++)
	{
		/*-- Handy Pointer --*/
		col = &desc->d_cols[i];

		dsp_fputs( "| ", output );

		if (col->c_justification == DSP_JUST_RIGHT)
		{
			for (j = 0; j < (col->c_width - col->c_format.namelen); j++)
			{
				dsp_fputc( ' ', output );
			}

			for (j = 0; j < col->c_width && j < col->c_format.namelen; j++)
			{
				dsp_fputc( col->c_format.name[j], output );
			}
		}
		else
		{
			for (j = 0; j < col->c_width; j++)
			{
				if (j < col->c_format.namelen)
				{
					dsp_fputc( col->c_format.name[j], output );
				}
				else
				{
					dsp_fputc( ' ', output );
				}
			}
		}

		dsp_fputc( ' ', output );
	}

	dsp_fputs( "|\n", output );

	/*
	 * Display a line separator.
	 */
	dsp_prsep( output, desc, (int)'=' );
}

static void dsp_prrow( output, desc )
	dsp_out_t   *output;
	dsp_desc_t  *desc;
{
	int   first_time = True;
	int   i, j;
	int   done;
	dsp_col_t    *col;
	int   dat_width;

	do
	{
		done = True;

		/*-- Blast through available data --*/
		for (i = 0; i < desc->d_ncols; i++)
		{
			/*-- Handy Pointer --*/
			col = &desc->d_cols[i];

			if (first_time == True)
			{
				col->c_ptr = col->c_data;
			}

			dsp_fputs( "| ", output );

			if (col->c_justification == DSP_JUST_RIGHT)
			{
				dat_width = strlen( col->c_ptr );

				for (j = 0; j < (col->c_width - dat_width); j++)
				{
					dsp_fputc( ' ', output );
				}

				for (j = 0; j < col->c_width && col->c_ptr[j] != '\0'; j++)
				{
					dsp_fputc( col->c_ptr[j], output );
				}
				col->c_ptr += j;
			}
			else
			{
				for( j = 0; j < col->c_width && col->c_ptr[j] != '\0' &&
					col->c_ptr[j] != '\n'; j++)
				{
					dsp_fputc( col->c_ptr[j], output );
				}
				col->c_ptr += j;

				if (*col->c_ptr == '\n')
				{
					++col->c_ptr;
				}

				if (*col->c_ptr != '\0')
				{
					done = False;
				}

				for (; j < col->c_width; ++j)
				{
					dsp_fputc( ' ', output );
				}
			}

			dsp_fputc( ' ', output );
		}

		dsp_fputs( "|\n", output );

		first_time = False;
	}
	while (done == False);

	dsp_prsep( output, desc, (int)'-' );

	return;
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
		dsp_fputc( '\n', output );
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
	int          dat_width;        /* Width of data to display */
	CS_INT       i;
	CS_INT       j;
	dsp_col_t    *sel_col;
	dsp_col_t    *com_col;
	int          done;
	int          first_time = True;
	int          count = 0;

	/**
	 ** STAGE 1: Display column headers
	 **/
	dsp_prsep( output, sel_desc, (int)'=' );

	/*
	 * As with dsp_prrow(), this function blasts through the set of
	 * columns from the select list...the difference here is that we
	 * want to display the value from the compute set instead.
	 */
	for (i = 0; i < sel_desc->d_ncols; i++)
	{
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
		}
		else
		{
			aggregate_op = "";
		}

		dsp_fputs( "| ", output );

		for (j = 0; j < sel_col->c_width && aggregate_op[j] != '\0'; j++)
		{
			dsp_fputc( aggregate_op[j], output );
		}

		for (; j < (sel_col->c_width+1); j++)
		{
			dsp_fputc( ' ', output );
		}

	}
	dsp_fputs( "|\n", output );

	/**
	 ** STAGE 2: Display dashed line
	 **/
	dsp_prsep( output, sel_desc, (int)'=' );

	/**
	 ** STAGE 3: Display data
	 **/
	do
	{
		done = True;

		/*-- Blast through available data --*/
		for (i = 0; i < sel_desc->d_ncols; i++)
		{
			/*-- Handy Pointer --*/
			sel_col = &sel_desc->d_cols[i];

			dsp_fputs( "| ", output );

			/*
			 * Look up the value for this select column by looking it up
			 * from the list of unprocessed compute columns.
			 */
			com_col = dsp_comp_find( com_desc, i+1, DSP_PROC_DATA );

			if (com_col == NULL)
			{
				for (j = 0; j < (sel_col->c_width + 1); j++)
				{
					dsp_fputc( ' ', output );
				}
			}
			else
			{
				if (first_time == True)
				{
					++count;
					com_col->c_ptr = com_col->c_data;
				}

				if (sel_col->c_justification == DSP_JUST_RIGHT)
				{
					dat_width = strlen( com_col->c_ptr );

					for (j = 0; j < (sel_col->c_width - dat_width); j++)
					{
						dsp_fputc( ' ', output );
					}

					for (j = 0; j < sel_col->c_width && com_col->c_ptr[j] != '\0';
						j++)
					{
						dsp_fputc( com_col->c_ptr[j], output );
					}
					com_col->c_ptr += j;
				}
				else
				{
					for( j = 0; j < sel_col->c_width && com_col->c_ptr[j] != '\0' &&
						com_col->c_ptr[j] != '\n'; j++)
					{
						dsp_fputc( com_col->c_ptr[j], output );
					}
					com_col->c_ptr += j;

					if (*com_col->c_ptr == '\n')
					{
						++com_col->c_ptr;
					}

					if (*com_col->c_ptr != '\0')
					{
						done = False;

						/*
						 * Icky.  This column was marked as processed by
						 * dsp_comp_find(), above, however, since we haven't
						 * completed printing all of its contents, we want
						 * to unmark it.  This way, during the next go-around
						 * dsp_comp_find() will find the column again.
						 */
						com_col->c_processed &= ~(DSP_PROC_DATA);
					}

					for (; j < sel_col->c_width; ++j)
					{
						dsp_fputc( ' ', output );
					}
				}

				dsp_fputc( ' ', output );
			}
		}

		dsp_fputs( "|\n", output );

		first_time = False;
	}
	while (done == False);

	dsp_prsep( output, sel_desc, (int)'-' );

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


