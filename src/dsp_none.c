/*
 * dsp_none.c - Suppress all results from being displayed.
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

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: dsp_none.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*
 * dsp_none:
 *
 * Suck the result set in from the server, but don't bother to
 * display anything.
 */
int dsp_none( output, cmd, flags )
	dsp_out_t   *output;
	CS_COMMAND  *cmd;
	int          flags;
{
	CS_INT   nrows;
	CS_INT   result_type;
	CS_INT   return_code;

	while ((return_code = ct_results( cmd, &result_type )) != CS_END_RESULTS)
	{
		if (g_dsp_interrupted)
			return DSP_INTERRUPTED;
		
		switch (result_type) 
		{
			case CS_COMPUTE_RESULT:
			case CS_PARAM_RESULT:
			case CS_ROW_RESULT:
			case CS_STATUS_RESULT:
				while (ct_fetch(cmd,             /* Command */
				                CS_UNUSED,       /* Type */
				                CS_UNUSED,       /* Offset */
				                CS_UNUSED,       /* Option */
				                &nrows) != CS_END_DATA)
				{
					if (g_dsp_interrupted)
						return DSP_INTERRUPTED;
				}
				break;
			default:
				break;
		}
	}
				                 
	return DSP_SUCCEED;
}
