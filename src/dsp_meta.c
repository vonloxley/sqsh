/*
 * dsp_meta.c - Display result set meta-data.
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
#include <ctpublic.h>
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_debug.h"
#include "dsp.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: dsp_meta.c,v 1.3 2004/04/11 15:14:32 mpeppler Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes --*/
static int        dsp_meta_fetch      _ANSI_ARGS(( CS_COMMAND*, CS_INT* ));
static CS_RETCODE dsp_meta_desc       _ANSI_ARGS(( dsp_out_t*, CS_COMMAND* ));
static CS_RETCODE dsp_meta_transtate  _ANSI_ARGS(( dsp_out_t*, CS_COMMAND* ));
static CS_CHAR*   dsp_meta_datatype   _ANSI_ARGS(( CS_INT ));
static CS_CHAR*   dsp_meta_result     _ANSI_ARGS(( CS_INT ));
static CS_CHAR*   dsp_meta_status     _ANSI_ARGS(( CS_INT, CS_CHAR* ));
static CS_CHAR*   dsp_meta_format     _ANSI_ARGS(( CS_INT, CS_CHAR* ));
static CS_RETCODE dsp_meta_bool_prop  
    _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, CS_INT, CS_CHAR* ));
static CS_RETCODE dsp_meta_int_prop
    _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, CS_INT, CS_CHAR* ));

int dsp_meta( out, cmd, flags )
    dsp_out_t   *out;
    CS_COMMAND *cmd;
    int         flags;
{
    CS_INT       result_type;
    CS_INT       nrows;
    CS_RETCODE   r;
    int          d;

    /*
     * Fetch results until there ain't any more left.
     */
    while ((r = ct_results( cmd, &result_type ) == CS_SUCCEED))
    {
        if (g_dsp_interrupted)
        {
            return DSP_INTERRUPTED;
        }

        /*
         * Display the result set type.
         */
        dsp_fprintf( out, "%s\n", (char*)dsp_meta_result( result_type ) );

        /*
         * Based upon result type, there may be different meta data
         * available within the result set.
         */
        switch (result_type)
        {
            case CS_CMD_DONE:
            case CS_CMD_SUCCEED:
                dsp_meta_int_prop( out, cmd, CS_CMD_NUMBER, "CS_CMD_NUMBER" );
                dsp_meta_int_prop( out, cmd, CS_ROW_COUNT, "CS_ROW_COUNT" );
                dsp_meta_transtate( out, cmd );
                break;

            case CS_CMD_FAIL:
                dsp_meta_int_prop( out, cmd, CS_CMD_NUMBER, "CS_CMD_NUMBER" );
                dsp_meta_int_prop( out, cmd, CS_ROW_COUNT, "CS_ROW_COUNT" );
                dsp_meta_transtate( out, cmd );
                return DSP_FAIL;
            
            case CS_ROW_RESULT:
                dsp_meta_bool_prop( out, cmd, CS_BROWSE_INFO, "CS_BROWSE_INFO" );
                dsp_meta_int_prop( out, cmd, CS_CMD_NUMBER, "CS_CMD_NUMBER" );
                dsp_meta_int_prop( out, cmd, CS_NUMDATA, "CS_NUMDATA" );
                dsp_meta_int_prop( out, cmd, CS_NUMORDERCOLS, "CS_NUMORDERCOLS" );
                dsp_meta_desc( out, cmd );

		if (g_dsp_interrupted)
		    return DSP_INTERRUPTED;

                if ((d = dsp_meta_fetch( cmd, &nrows )) != DSP_SUCCEED)
                {
                    return d;
                }

                dsp_meta_transtate( out, cmd );
                break;

            case CS_CURSOR_RESULT:
                dsp_meta_int_prop( out, cmd, CS_CMD_NUMBER, "CS_CMD_NUMBER" );
                dsp_meta_int_prop( out, cmd, CS_NUMDATA, "CS_NUMDATA" );
                dsp_meta_desc( out, cmd );

		if (g_dsp_interrupted)
		    return DSP_INTERRUPTED;

                if ((d = dsp_meta_fetch( cmd, &nrows )) != DSP_SUCCEED)
                {
                    return d;
                }

                dsp_meta_transtate( out, cmd );
                break;

            case CS_COMPUTE_RESULT:
                dsp_meta_int_prop( out, cmd, CS_CMD_NUMBER, "CS_CMD_NUMBER" );
                dsp_meta_int_prop( out, cmd, CS_NUMDATA, "CS_NUMDATA" );
                dsp_meta_int_prop( out, cmd, CS_NUM_COMPUTES, "CS_NUM_COMPUTES" );
                dsp_meta_desc( out, cmd );

		if (g_dsp_interrupted)
		    return DSP_INTERRUPTED;

                if ((d = dsp_meta_fetch( cmd, &nrows )) != DSP_SUCCEED)
                {
                    return d;
                }
                dsp_meta_transtate( out, cmd );
                break;

            case CS_PARAM_RESULT:
                dsp_meta_int_prop( out, cmd, CS_CMD_NUMBER, "CS_CMD_NUMBER" );
                dsp_meta_int_prop( out, cmd, CS_NUMDATA, "CS_NUMDATA" );
                dsp_meta_desc( out, cmd );

		if (g_dsp_interrupted)
		    return DSP_INTERRUPTED;

                if ((d = dsp_meta_fetch( cmd, &nrows )) != DSP_SUCCEED)
                {
                    return d;
                }
                dsp_meta_transtate( out, cmd );
                break;

            case CS_STATUS_RESULT:
                dsp_meta_int_prop( out, cmd, CS_CMD_NUMBER, "CS_CMD_NUMBER" );
                dsp_meta_int_prop( out, cmd, CS_NUMDATA, "CS_NUMDATA" );
                dsp_meta_desc( out, cmd );

		if (g_dsp_interrupted)
		    return DSP_INTERRUPTED;

                if ((d = dsp_meta_fetch( cmd, &nrows )) != DSP_SUCCEED)
                {
                    return d;
                }
                dsp_meta_transtate( out, cmd );
                break;
            default:
                break;
        }

	if (g_dsp_interrupted)
	    return DSP_INTERRUPTED;
    }

    if (r == CS_FAIL)
    {
        return DSP_FAIL;
    }

    return DSP_SUCCEED;
}

/*
 * dsp_meta_desc():
 *
 * Displays the CS_DATAFMT structure associated with every column
 * in a result set.
 */
static CS_RETCODE dsp_meta_desc( out, cmd )
    dsp_out_t  *out;
    CS_COMMAND *cmd;
{
    CS_CHAR    buf[128];
    CS_INT     ncols;
    CS_DATAFMT fmt;
    CS_INT     i;

    if (ct_res_info(cmd, CS_NUMDATA, (CS_VOID*)&ncols,
        CS_UNUSED, (CS_INT*)NULL) != CS_SUCCEED)
    {
        dsp_fprintf( out, "dsp_meta_desc: Cannot determine #cols\n" );
        return CS_FAIL;
    }

    for (i = 0; i < ncols; i++)
    {
        memset( (void*)&fmt, 0, sizeof(fmt) );
        if (ct_describe( cmd, (i+1), &fmt ) != CS_SUCCEED)
        {
            dsp_fprintf( out, "dsp_meta_desc:   Cannot describe col #%d\n", 
                (int)i+1 );
            continue;
        }

        fmt.name[fmt.namelen] = '\0';

        dsp_fprintf( out, "   COLUMN #%d\n", (int)(i+1) );
        dsp_fprintf( out, "      name      =  %s\n", (char*)fmt.name );
        dsp_fprintf( out, "      namelen   =  %d\n", (int)fmt.namelen );
        dsp_fprintf( out, "      datatype  =  %s\n", 
            (char*)dsp_meta_datatype( fmt.datatype ) );
        dsp_fprintf( out, "      format    =  %s\n", 
            (char*)dsp_meta_format( fmt.format, buf ) );
        dsp_fprintf( out, "      maxlength =  %d\n", (int)fmt.maxlength );
        dsp_fprintf( out, "      scale     =  %d\n", (int)fmt.scale );
        dsp_fprintf( out, "      precision =  %d\n", (int)fmt.precision );
        dsp_fprintf( out, "      status    =  %s\n", 
            (char*)dsp_meta_status( fmt.status, buf ) );
        dsp_fprintf( out, "      count     =  %d\n", (int)fmt.count );
        dsp_fprintf( out, "      usertype  =  %d\n", (int)fmt.usertype );
        dsp_fprintf( out, "      locale    =  0x%p\n", 
            (void*)fmt.locale );
    }

    return CS_SUCCEED;
}

/*
 * dsp_meta_status():
 * 
 * Return string descriptions of a datafmt.status field.
 */
static CS_CHAR* dsp_meta_status( s, fmt )
    CS_INT   s;
    CS_CHAR *fmt;
{
    CS_BOOL need_comma = CS_FALSE;

    fmt[0] = '\0';
    if (s == CS_UNUSED)
    {
        strcat(fmt,  "CS_UNUSED" );
    }

    if (s & CS_HIDDEN)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_HIDDEN" );
    }
    if (s & CS_KEY)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_KEY" );
    }
    if (s & CS_VERSION_KEY)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_VERSION_KEY" );
    }
    if (s & CS_NODATA)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_NODATA" );
    }
    if (s & CS_UPDATABLE)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_UPDATABLE" );
    }
    if (s & CS_CANBENULL)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_CANBENULL" );
    }
    if (s & CS_DESCIN)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_DESCIN" );
    }
    if (s & CS_DESCOUT)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_DESCOUT" );
    }
    if (s & CS_INPUTVALUE)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_INPUTVALUE" );
    }
    if (s & CS_UPDATECOL)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_UPDATECOL" );
    }
    if (s & CS_RETURN)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_RETURN" );
    }
#if defined(CS_RETURN_CANBENULL)
    if (s & CS_RETURN_CANBENULL)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_RETURN_CANBENULL" );
    }
#endif
    if (s & CS_TIMESTAMP)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_TIMESTAMP" );
    }
    if (s & CS_NODEFAULT)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_NODEFAULT" );
    }
    if (s & CS_IDENTITY)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_IDENTITY" );
    }

    return fmt;
}

/*
 * dsp_meta_format():
 *
 * Returns a string description of a datafmt.format field.
 */
static CS_CHAR* dsp_meta_format( f, fmt )
    CS_INT   f;
    CS_CHAR *fmt;
{
    CS_BOOL need_comma = CS_FALSE;

    fmt[0] = '\0';
    if (f == CS_FMT_UNUSED)
    {
        strcat( fmt, "CS_FMT_UNUSED" );
    }

    if (f & CS_FMT_NULLTERM)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_FMT_NULLTERM" );
    }
    if (f & CS_FMT_PADNULL)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_FMT_PADNULL" );
    }
    if (f & CS_FMT_PADBLANK)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_FMT_PADBLANK" );
    }
    if (f & CS_FMT_JUSTIFY_RT)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_FMT_JUSTIFY_RT" );
    }

#if defined(CS_FMT_STRIPBLANKS)
    if (f & CS_FMT_STRIPBLANKS)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_FMT_STRIPBLANKS" );
    }
#endif

#if defined(CS_FMT_SAFESTR)
    if (f & CS_FMT_SAFESTR)
    {
        if (need_comma)
        {
            strcat(fmt, "," );
        }
        need_comma = CS_TRUE;

        strcat( fmt, "CS_FMT_SAFESTR" );
    }
#endif

    return fmt;
}

/*
 * dsp_meta_datatype():
 *
 * Return a string description of a data type.
 */
static CS_CHAR* dsp_meta_datatype( t )
    CS_INT  t;
{

    switch(t)
    {
        case CS_ILLEGAL_TYPE:
            return "CS_ILLEGAL_TYPE";
        case CS_CHAR_TYPE:
            return "CS_CHAR_TYPE";
        case CS_BINARY_TYPE:
            return "CS_BINARY_TYPE";
        case CS_LONGCHAR_TYPE:
            return "CS_LONGCHAR_TYPE";
        case CS_LONGBINARY_TYPE:
            return "CS_LONGBINARY_TYPE";
        case CS_TEXT_TYPE:
            return "CS_TEXT_TYPE";
        case CS_IMAGE_TYPE:
            return "CS_IMAGE_TYPE";
        case CS_TINYINT_TYPE:
            return "CS_TINYINT_TYPE";
        case CS_SMALLINT_TYPE:
            return "CS_SMALLINT_TYPE";
        case CS_INT_TYPE:
            return "CS_INT_TYPE";
        case CS_REAL_TYPE:
            return "CS_REAL_TYPE";
        case CS_FLOAT_TYPE:
            return "CS_FLOAT_TYPE";
        case CS_BIT_TYPE:
            return "CS_BIT_TYPE";
        case CS_DATETIME_TYPE:
            return "CS_DATETIME_TYPE";
        case CS_DATETIME4_TYPE:
            return "CS_DATETIME4_TYPE";
        case CS_MONEY_TYPE:
            return "CS_MONEY_TYPE";
        case CS_MONEY4_TYPE:
            return "CS_MONEY4_TYPE";
        case CS_NUMERIC_TYPE:
            return "CS_NUMERIC_TYPE";
        case CS_DECIMAL_TYPE:
            return "CS_DECIMAL_TYPE";
        case CS_VARCHAR_TYPE:
            return "CS_VARCHAR_TYPE";
        case CS_VARBINARY_TYPE:
            return "CS_VARBINARY_TYPE";
        case CS_LONG_TYPE:
            return "CS_LONG_TYPE";
        case CS_SENSITIVITY_TYPE:
            return "CS_SENSITIVITY_TYPE";
        case CS_BOUNDARY_TYPE:
            return "CS_BOUNDARY_TYPE";
        case CS_VOID_TYPE:
            return "CS_VOID_TYPE";
        case CS_USHORT_TYPE:
            return "CS_USHORT_TYPE";
#if defined(CS_UNICHAR_TYPE)
	case CS_UNICHAR_TYPE:
	    return "CS_UNICHAR_TYPE";
#endif
#if defined(CS_BLOB_TYPE)
	case CS_BLOB_TYPE:
	    return "CS_BLOB_TYPE";
#endif
#if defined(CS_DATE_TYPE)
	case CS_DATE_TYPE:
	    return "CS_DATE_TYPE";
#endif
#if defined(CS_TIME_TYPE)
	case CS_TIME_TYPE:
	    return "CS_TIME_TYPE";
#endif
#if defined(CS_UNITEXT_TYPE)
	case CS_UNITEXT_TYPE:
	    return "CS_UNITEXT_TYPE";
#endif
#if defined(CS_BIGINT_TYPE)
	case CS_BIGINT_TYPE:
	    return "CS_BIGINT_TYPE";
#endif
#if defined(CS_USMALLINT_TYPE)
	case CS_USMALLINT_TYPE:
	    return "CS_USMALLINT_TYPE";
#endif
#if defined(CS_UINT_TYPE)
	case CS_UINT_TYPE:
	    return "CS_UINT_TYPE";
#endif
#if defined(CS_UBIGINT_TYPE)
	case CS_UBIGINT_TYPE:
	    return "CS_UBIGINT_TYPE";
#endif
#if defined(CS_XML_TYPE)
	case CS_XML_TYPE:
	    return "CS_XML_TYPE";
#endif
#if defined(CS_BIGDATETIME_TYPE)
	case CS_BIGDATETIME_TYPE:
	    return "CS_BIGDATETIME_TYPE";
#endif
#if defined(CS_BIGTIME_TYPE)
	case CS_BIGTIME_TYPE:
	    return "CS_BIGTIME_TYPE";
#endif
        default:
            break;
    }
    return "<INVALID>";
}

/*
 * dsp_meta_bool_prop():
 *
 * Displays a boolean result set property value.
 */
static CS_RETCODE dsp_meta_bool_prop( out, cmd, type, desc )
    dsp_out_t  *out;
    CS_COMMAND *cmd;
    CS_INT      type;
    CS_CHAR    *desc;
{
    CS_BOOL  bool;

    if (ct_res_info(cmd, type, (CS_VOID*)&bool,
        CS_UNUSED, (CS_INT*)NULL) != CS_SUCCEED)
    {
        dsp_fprintf( out, "dsp_meta_bool_prop:   Failed fetching %s\n", desc );
        return CS_FAIL;
    }
    else
    {
        dsp_fprintf( out, "   %-20s  = %s\n",
            desc, ((bool == CS_TRUE) ? "CS_TRUE" : 
                ((bool == CS_FALSE) ? "CS_FALSE": "<INVALID>")) );
    }

    return CS_SUCCEED;
}

/*
 * dsp_meta_int_prop():
 *
 * Displays an integer result set property value.
 */
static  CS_RETCODE dsp_meta_int_prop( out, cmd, type, desc )
    dsp_out_t  *out;
    CS_COMMAND *cmd;
    CS_INT      type;
    CS_CHAR    *desc;
{
    CS_INT  num;

    if (ct_res_info(cmd, type, (CS_VOID*)&num,
        CS_UNUSED, (CS_INT*)NULL) != CS_SUCCEED)
    {
        dsp_fprintf( out, "dsp_meta_int_prop: Failed fetching %s\n", desc );
        return CS_FAIL;
    }
    else
    {
        dsp_fprintf( out, "   %-20s  = %d\n",
            desc, (int)num );
    }

    return CS_SUCCEED;
}

/*
 * dsp_meta_transtate():
 *
 * Display the transaction state of a command.
 */
static CS_RETCODE dsp_meta_transtate( out, cmd )
    dsp_out_t  *out;
    CS_COMMAND *cmd;
{
    CS_INT  transtate;
    CS_CHAR *s;

    if (ct_res_info(cmd, CS_TRANS_STATE, (CS_VOID*)&transtate,
        CS_UNUSED, (CS_INT*)NULL) != CS_SUCCEED)
    {
        dsp_fprintf( out, "Failed CS_TRANS_STATE\n" );
        return CS_FAIL;
    }
    else
    {
        switch (transtate)
        {
            case CS_TRAN_IN_PROGRESS:
                s = "CS_TRAN_IN_PROGRESS";
                break;

            case CS_TRAN_COMPLETED:
                s = "CS_TRAN_COMPLETED";
                break;

            case CS_TRAN_STMT_FAIL:
                s = "CS_TRAN_STMT_FAIL";
                break;
            
            case CS_TRAN_FAIL:
                s = "CS_TRAN_FAIL";
                break;
            
            case CS_TRAN_UNDEFINED:
                s = "CS_TRAN_UNDEFINED";
                break;
            
            default:
                s = "<INVALID>";
            }

        dsp_fprintf( out, "   %-20s  = %s\n",
            "CS_TRANS_STATE", (char*)s );
    }

    return CS_SUCCEED;
}

/*
 * dsp_meta_fetch():
 *
 * Fetch and discard rows from a result set.
 */
static int dsp_meta_fetch( cmd, rows )
    CS_COMMAND  *cmd;
    CS_INT      *rows;
{
    CS_INT  nrows;
    CS_RETCODE r;

    *rows = 0;

    while ((r = ct_fetch( cmd, CS_UNUSED, CS_UNUSED, 
                          CS_UNUSED, &nrows )) == CS_SUCCEED)
    {
        if (g_dsp_interrupted)
        {
            return DSP_INTERRUPTED;
        }
        *rows += nrows;
    }

    if (r != CS_END_DATA)
    {
        return DSP_FAIL;
    }

    return DSP_SUCCEED;
}

static CS_CHAR* dsp_meta_result( CS_INT type )
{
    switch (type)
    {
        case CS_CMD_DONE:          return "CS_CMD_DONE";
        case CS_CMD_FAIL:          return "CS_CMD_FAIL";
        case CS_CMD_SUCCEED:       return "CS_CMD_SUCCEED";
        case CS_COMPUTE_RESULT:    return "CS_COMPUTE_RESULT";
        case CS_CURSOR_RESULT:     return "CS_CURSOR_RESULT";
        case CS_PARAM_RESULT:      return "CS_PARAM_RESULT";
        case CS_ROW_RESULT:        return "CS_ROW_RESULT";
        case CS_STATUS_RESULT:     return "CS_STATUS_RESULT";
        case CS_COMPUTEFMT_RESULT: return "CS_COMPUTEFMT_RESULT";
        case CS_ROWFMT_RESULT:     return "CS_ROWFMT_RESULT";
        case CS_MSG_RESULT:        return "CS_MSG_RESULT";
        case CS_DESCRIBE_RESULT:   return "CS_DESCRIBE_RESULT";
        default:                   break;
    }

    return "UNKNOWN";
}


