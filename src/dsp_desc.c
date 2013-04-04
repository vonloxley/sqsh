/*
 * dsp_desc.c - Describe a result set.
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
static char RCS_Id[] = "$Id: dsp_desc.c,v 1.9 2013/02/19 18:06:42 mwesdorp Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Local Prototypes --*/
static CS_INT dsp_dlen           _ANSI_ARGS(( CS_DATAFMT* ));
static CS_INT dsp_just           _ANSI_ARGS(( CS_INT ));

/*
 * This is almost entirely cheesy.  Since CT-Lib does a crappy job
 * at converting most types to strings, we do it ourselves.  However
 * for several cases (such as when the data is already a string)
 * it is OK for CT-Lib to do the work.  This macro allows us to test
 * a given type as to whether or not we are going to let CT-Lib
 * do the dirty work.
 * sqsh-2.1.8: Add CS_BINARY_TYPE and CS_IMAGE_TYPE to the list.
 *               Fix bugreport 3079678.
 */
#define LET_CTLIB_CONV(t) \
    (((t) == CS_CHAR_TYPE)      || \
     ((t) == CS_TEXT_TYPE)      || \
     ((t) == CS_TINYINT_TYPE)   || \
     ((t) == CS_SMALLINT_TYPE)  || \
     ((t) == CS_INT_TYPE)       || \
     ((t) == CS_BIT_TYPE)       || \
     ((t) == CS_NUMERIC_TYPE)   || \
     ((t) == CS_DECIMAL_TYPE)   || \
     ((t) == CS_VARCHAR_TYPE)   || \
     ((t) == CS_LONGCHAR_TYPE)  || \
     ((t) == CS_LONGBINARY_TYPE)|| \
     ((t) == CS_VARBINARY_TYPE) || \
     ((t) == CS_BINARY_TYPE)    || \
     ((t) == CS_IMAGE_TYPE)     || \
     ((t) == CS_UNICHAR_TYPE))

/*
 * dsp_desc_bind():
 *
 * This function is responsible for allocating and populating a dsp_desc
 * data structure, which represents a result set coming back from the
 * server.  As such, it is expected to be called immediately following
 * a ct_results() call.
 */
dsp_desc_t* dsp_desc_bind( cmd, result_type )
    CS_COMMAND  *cmd;
    CS_INT       result_type;
{
    CS_INT       ncols;
    CS_INT       i;
    dsp_desc_t  *d = NULL;
    dsp_col_t   *c = NULL;
    CS_DATAFMT   str_fmt;

    /*-- Retrieve the number of columns in the result set --*/
    if (ct_res_info( cmd,               /* Command */
                     CS_NUMDATA,        /* Type */
                     (CS_VOID*)&ncols,  /* Buffer */
                     CS_UNUSED,         /* Buffer Length */
                     (CS_INT*)NULL) != CS_SUCCEED)
    {
        fprintf( stderr, 
            "dsp_desc_bind: Unable to retrieve column count (CS_NUMDATA)\n" );
        return NULL;
    }

    d = (dsp_desc_t*)malloc( sizeof( dsp_desc_t ) );
    c = (dsp_col_t*)malloc( sizeof( dsp_col_t ) * ncols );

    if (d == NULL || c == NULL) 
    {
        fprintf( stderr, "dsp_desc_bind: Memory allocation failure.\n" );
        if (d != NULL)
            free( d );
        if (c != NULL)
            free( c );
        return NULL;
    }

    d->d_type        = result_type;
    d->d_ncols       = ncols;
    d->d_cols        = c;
    d->d_bylist_size = 0;
    d->d_bylist      = NULL;

    for (i = 0; i < ncols; i++) 
    {
        d->d_cols[i].c_data = NULL;

        /*-- This should be replaced with memset() --*/
        d->d_cols[i].c_format.name[0]     = '\0';
        d->d_cols[i].c_format.namelen     = 0;
        d->d_cols[i].c_format.datatype    = 0;
        d->d_cols[i].c_format.format      = 0;
        d->d_cols[i].c_format.maxlength   = 0;
        d->d_cols[i].c_format.scale       = 0;
        d->d_cols[i].c_format.precision   = 0;
        d->d_cols[i].c_format.status      = 0;
        d->d_cols[i].c_format.count       = 0;
        d->d_cols[i].c_format.usertype    = 0;
        d->d_cols[i].c_format.locale      = NULL;
    }
    
    if (result_type == CS_COMPUTE_RESULT)
    {
        if (ct_compute_info( cmd,                         /* Command */
                             CS_BYLIST_LEN,               /* Type */
                             CS_UNUSED,                   /* Colnum */
                             (CS_VOID*)&d->d_bylist_size, /* Buffer */
                             CS_UNUSED,                   /* Buffer Length */
                             (CS_INT*)NULL ) != CS_SUCCEED)
        {
            dsp_desc_destroy( d );
            fprintf( stderr, 
                "dsp_desc_bind: Unable to fetch by-list len of compute results\n" );
            return NULL;
        }

        if (d->d_bylist_size > 0)
        {

            d->d_bylist = (CS_SMALLINT*)malloc( sizeof(CS_SMALLINT) * 
                                                d->d_bylist_size );
        
            if (d->d_bylist == NULL)
            {
                dsp_desc_destroy( d );
                fprintf( stderr, 
                    "dsp_desc_bind: Memory failure for by-list size array\n" );
                return NULL;
            }

            if (ct_compute_info( cmd,                         /* Command */
                                 CS_COMP_BYLIST,              /* Type */
                                 CS_UNUSED,                   /* Colnum */
                                 (CS_VOID*)d->d_bylist,       /* Buffer */
                                 (CS_INT)sizeof(CS_SMALLINT) * d->d_bylist_size,
                                 (CS_INT*)NULL) != CS_SUCCEED)
            {
                dsp_desc_destroy( d );
                fprintf( stderr, 
                    "dsp_desc_bind: Memory allocation failure for by-list array\n" );
                return NULL;
            }
        }
    }

    
    /*
     * Blast through the set of columns, binding the output to a
     * friendly chunk of memory.
     */
    for (i = 0; i < ncols; i++) 
    {
        /*-- Get description for column --*/
        if (ct_describe( cmd, i+1, &d->d_cols[i].c_format ) != CS_SUCCEED) 
        {
            dsp_desc_destroy( d );
            fprintf( stderr, 
                "dsp_desc_bind: Unable to fetch description of column #%d\n",
                (int)i+1 );
            return NULL;
        }

        d->d_cols[i].c_justification= dsp_just( d->d_cols[i].c_format.datatype );
        d->d_cols[i].c_maxlength    = dsp_dlen( &d->d_cols[i].c_format );
        d->d_cols[i].c_processed    = False;
        d->d_cols[i].c_colid        = i + 1;
        d->d_cols[i].c_native       = NULL;
        d->d_cols[i].c_data         = NULL;

#if 0
	/* This code has been commented out as it generates the dreaded
	   "bind resulted in truncation" error. */
        if (g_dsp_props.p_maxlen > 0 && 
            d->d_cols[i].c_maxlength > g_dsp_props.p_maxlen)
        {
            d->d_cols[i].c_maxlength = g_dsp_props.p_maxlen;
        }
#endif

        /*
         * Allocate enough space to hold the native data-type as it
         * comes from the server as well as enough space to convert
         * it to a string.  Note that I am being cheesy here.  Since
         * I rely on sprintf() to convert CS_FLOAT's and CS_REAL's
         * to readable strings, and I know that sprintf() can be
         * sloppy (will happily over-run the precision that you
         * request), I am forcing these data types to malloc a tad
         * more memory then they may need.
         */
	/* The amount of memory alloced used to be 64 bytes. Pushed 
	   to 256 following discovery of buffer overflow problems by
	   Mike Tibbetts */
        if (d->d_cols[i].c_format.datatype == CS_FLOAT_TYPE ||
            d->d_cols[i].c_format.datatype == CS_REAL_TYPE)
        {
            d->d_cols[i].c_data = (CS_CHAR*)malloc(256);
        }
        else
        {
            d->d_cols[i].c_data = (CS_CHAR*)malloc(d->d_cols[i].c_maxlength + 1);
        }


        if (d->d_cols[i].c_data == NULL)
        {
            fprintf( stderr,
                "dsp_desc_bind: Memory allocation failure for column #%d\n",
                (int)i + 1 );
            dsp_desc_destroy( d );
            return NULL;
        }

        /*
         * There are two types of data that we are going to deal with. Those
         * that we allow CT-Lib to do the conversion for us implicitly by
         * simply binding the incoming row to a CS_CHAR, and those that we
         * would rather do the conversion ourselves.
         */
        if (LET_CTLIB_CONV(d->d_cols[i].c_format.datatype))
        {
            /*
             * If CT-Lib is doing the conversion then we don't need to
             * allocate any space to stored the data in its native format
             */
            d->d_cols[i].c_is_native = CS_FALSE;
            d->d_cols[i].c_native    = NULL;

            /*
             * Create a format description for the string representation
             * of the outgoing data.
             */
            str_fmt.datatype  = CS_CHAR_TYPE;
            str_fmt.format    = CS_FMT_NULLTERM;
            str_fmt.maxlength = d->d_cols[i].c_maxlength + 1;
            str_fmt.scale     = 0;
            str_fmt.precision = 0;
            str_fmt.count     = 1;
            str_fmt.locale    = NULL;

            DBG(sqsh_debug(DEBUG_DISPLAY,
                "dsp_desc_bind: ct_bind(\n"
                "    cmd  = %p,\n"
                "    item = %d,\n"
                "    fmt  = [datatype  = CS_CHAR_TYPE,\n"
                "            format    = CS_FMT_NULLTERM,\n"
                "            maxlength = %d,\n"
                "            count     = 1,\n"
                "            locale    = NULL],\n"
                "    buf  = %p,\n"
                "    bytes= NULL,\n"
                "    ind  = %p)\n",
                (void*)cmd, 
                i + 1, 
                (int)str_fmt.maxlength, 
                (void*)d->d_cols[i].c_data,
                (void*)&d->d_cols[i].c_nullind);)
                

            /*
             * Now, bind the incoming row to the native data type. That
             * is about it.  The only important thing here is to remember
             * during dsp_desc_fetch() which data types need conversion.
             */
            if (ct_bind( cmd,                                 /* Command */
                         i + 1,                               /* Item */
                         &str_fmt,                            /* Format */
                         (CS_VOID*)d->d_cols[i].c_data,       /* Buffer */
                         (CS_INT*)NULL,                       /* Bytes Xfered */
                         &d->d_cols[i].c_nullind              /* NULL Indicator */
                       ) != CS_SUCCEED)
            {
                dsp_desc_destroy( d );

                fprintf( stderr,
                    "dsp_desc_bind: Native bind of column #%d failed\n", (int)i + 1);
                return NULL;
            }
        }
        else
        {
            /*
             * If we are going to do the conversion to a displayable string
             * then we want to allocate enough space to hold the native
             * data prior to doing the conversion.
             */
            d->d_cols[i].c_is_native = CS_TRUE;
            d->d_cols[i].c_native    = 
                (CS_VOID*)malloc(d->d_cols[i].c_format.maxlength);
            
            if (d->d_cols[i].c_native == NULL)
            {
                fprintf( stderr,
                    "dsp_desc_bind: Memory alloc failure for native column %d\n",
                   (int)i + 1 );
                dsp_desc_destroy( d );
                return NULL;
            }

            /*
             * This probably isn't necessary, but I just want to be
             * on the safe side.
             */
            d->d_cols[i].c_format.count = 1;

            DBG(sqsh_debug(DEBUG_DISPLAY,
                "dsp_desc_bind: ct_bind(\n"
                "    cmd  = %p,\n"
                "    item = %d,\n"
                "    fmt  = [datatype  = %d,\n"
                "            format    = %d,\n"
                "            maxlength = %d,\n"
                "            scale     = %d,\n"
                "            precision = %d,\n"
                "            count     = %d,\n"
                "            locale    = %p],\n"
                "    buf  = %p,\n"
                "    bytes= %p,\n"
                "    ind  = %p)\n",
                (void*)cmd, 
                i + 1, 
                (int)d->d_cols[i].c_format.datatype,
                (int)d->d_cols[i].c_format.format,
                (int)d->d_cols[i].c_format.maxlength,
                (int)d->d_cols[i].c_format.scale,
                (int)d->d_cols[i].c_format.precision,
                (int)d->d_cols[i].c_format.count,
                (void*)d->d_cols[i].c_format.locale,
                (void*)d->d_cols[i].c_native,
                (void*)&d->d_cols[i].c_native_len,
                (void*)&d->d_cols[i].c_nullind );)

            /*
             * Now, bind the incoming row to the native data type. That
             * is about it.  The only important thing here is to remember
             * during dsp_desc_fetch() which data types need conversion.
             */
            if (ct_bind( cmd,                                 /* Command */
                         i + 1,                               /* Item */
                         &d->d_cols[i].c_format,              /* Format */
                         (CS_VOID*)d->d_cols[i].c_native,     /* Buffer */
                         (CS_INT*)&d->d_cols[i].c_native_len, /* Bytes Xfered */
                         &d->d_cols[i].c_nullind              /* NULL Indicator */
                       ) != CS_SUCCEED)
            {
                fprintf( stderr,
                    "dsp_desc_bind: Convert bind of column #%d failed\n",(int)i + 1);
                dsp_desc_destroy( d );
                return NULL;
            }

        }

        /*
         * If we are dealing with a compute result, then we want to dig
         * up a little more information about the result set and move
         * it into our column description.
         */
        if (result_type == CS_COMPUTE_RESULT)
        {
            if (ct_compute_info( cmd,                         /* Command */
                                        CS_COMP_OP,                  /* Type */
                                        i + 1,                       /* Colnum */
                                        (CS_VOID*)&d->d_cols[i].c_aggregate_op,
                                        CS_UNUSED,                   /* Buffer Length */
                                        (CS_INT*)NULL ) != CS_SUCCEED)
            {
                fprintf( stderr,
                    "dsp_desc_bind: Failed to get aggregate operator of col #%d\n",
                    (int)i + 1 );
                dsp_desc_destroy( d );
                return NULL;
            }

            if (ct_compute_info( cmd,                         /* Command */
                                        CS_COMP_COLID,               /* Type */
                                        i + 1,                       /* Colnum */
                                        (CS_VOID*)&d->d_cols[i].c_column_id,
                                        CS_UNUSED,                   /* Buffer Length */
                                        (CS_INT*)NULL ) != CS_SUCCEED)
            {
                fprintf( stderr,
                    "dsp_desc_bind: Failed to get aggregate column id of col #%d\n",
                    (int)i + 1 );
                dsp_desc_destroy( d );
                return NULL;
            }
        }
    }

    return d;
}

CS_INT dsp_desc_fetch( cmd, d )
    CS_COMMAND  *cmd;
    dsp_desc_t  *d;
{
    CS_RETCODE  r;
    CS_INT      nrows;
    CS_INT      i;
    CS_INT      j;
    CS_INT      p;
    CS_DATAFMT  str_fmt;

    if ((r = ct_fetch( cmd,              /* Command */
                       CS_UNUSED,        /* Type */
                       CS_UNUSED,        /* Offset */
                       CS_UNUSED,        /* Option */
                       &nrows )) == CS_END_DATA)
    {
        return CS_END_DATA;
    }

	/* mpeppler - 4/9/2004
	   allow CS_ROW_FAIL results to go through and not abort the entire
	   query. CS_ROW_FAIL usually means a conversion or truncation error
	   which shouldn't be fatal to the entire query, although a warning 
	   should be printed. */
    if (r != CS_SUCCEED && r != CS_ROW_FAIL)
    {
        return r;
    }

    /*-- Note, should use memset() here --*/
    str_fmt.name[0]   = '\0';
    str_fmt.namelen   = 0;
    str_fmt.datatype  = CS_CHAR_TYPE;
    str_fmt.format    = CS_FMT_NULLTERM;
    str_fmt.maxlength = 0;
    str_fmt.scale     = 0;
    str_fmt.precision = 0;
    str_fmt.status    = 0;
    str_fmt.count     = 0;
    str_fmt.usertype  = 0;
    str_fmt.locale    = NULL;

    for (i = 0; i < d->d_ncols; i++)
    {
        /*
         * If the column is NULL, then don't bother to attempt any
         * sort of conversion, just empty out the data string and
         * continue on to the next column.
         */
        if (d->d_cols[i].c_nullind != 0)
        {
            d->d_cols[i].c_data[0] = '\0';
            continue;
        }

        /*
         * If the is_native flag is FALSE then the data was already
         * converted into a string for us by CT-Lib, so there is
         * nothing left to be done.
         * sqsh-2.1.8: Except when the source datatype was binary, then
         * we have to prepend the result string with characters '0x'.
	 * 20110107: Prepend 0x0 in case of odd number of chars in string.
         */
        if (d->d_cols[i].c_is_native == CS_FALSE)
        {
            if (d->d_cols[i].c_format.datatype == CS_BINARY_TYPE     ||
                d->d_cols[i].c_format.datatype == CS_LONGBINARY_TYPE ||
                d->d_cols[i].c_format.datatype == CS_VARBINARY_TYPE  ||
                d->d_cols[i].c_format.datatype == CS_IMAGE_TYPE)
            {
                for (p = strlen (d->d_cols[i].c_data), j = p%2==0?2:3; p >= 0;
                                 d->d_cols[i].c_data[p+j] = d->d_cols[i].c_data[p]) p--;
                d->d_cols[i].c_data[0] = '0';
                d->d_cols[i].c_data[1] = 'x';
                if (j==3) d->d_cols[i].c_data[2] = '0';
            }

            continue;
        }

        /*
         * The datatype is declared native, so we have to convert it to displayable
         * character data here.
        */
        str_fmt.maxlength = d->d_cols[i].c_maxlength + 1;
        switch (d->d_cols[i].c_format.datatype)
        {
            case CS_REAL_TYPE:
                sprintf( (char*)d->d_cols[i].c_data, "%*.*f", 
                         g_dsp_props.p_real_prec + 2,
                         g_dsp_props.p_real_scale,
                         (double)(*((CS_REAL*)d->d_cols[i].c_native)) );
                break;

            case CS_FLOAT_TYPE:
                sprintf( (char*)d->d_cols[i].c_data, "%*.*f", 
                         g_dsp_props.p_flt_prec + 2,
                         g_dsp_props.p_flt_scale,
                         (double)(*((CS_FLOAT*)d->d_cols[i].c_native)) );
                break;

            /*
             * sqsh-2.1.9 - Also take date and time datatypes into consideration
            */
            case CS_DATETIME_TYPE:
            case CS_DATETIME4_TYPE:
#if defined(CS_DATE_TYPE)
	    case CS_DATE_TYPE:
#endif
#if defined(CS_TIME_TYPE)
	    case CS_TIME_TYPE:
#endif
#if defined(CS_BIGTIME_TYPE)
	    case CS_BIGTIME_TYPE:
#endif
#if defined(CS_BIGDATETIME_TYPE)
	    case CS_BIGDATETIME_TYPE:
#endif
                if (dsp_datetime_conv( g_context,              /* Context */
                                      &d->d_cols[i].c_format,  /* Data format */
                                       d->d_cols[i].c_native,  /* Data */
                                       d->d_cols[i].c_data,    /* Destination */
                                       d->d_cols[i].c_maxlength+1,
				       d->d_cols[i].c_format.datatype ) != CS_SUCCEED)
                {
                    return CS_FAIL;
                }
                break;

            default:
                d->d_cols[i].c_format.maxlength = d->d_cols[i].c_native_len;

                if (cs_convert( g_context,                     /* Context */
                                &d->d_cols[i].c_format,        /* Source Format */
                                d->d_cols[i].c_native,         /* Source Data */
                                &str_fmt,                      /* Dest Format */
                                (CS_VOID*)d->d_cols[i].c_data, /* Dest Data */
                                (CS_INT*)NULL ) != CS_SUCCEED)
                {
                    fprintf( stderr, 
                        "dsp_desc_fetch: cs_convert(%d->CHAR) column %d failed\n",
                       (int)d->d_cols[i].c_format.datatype, (int)i+1 );
                    return CS_FAIL;
                }
                break;
        }
    }

    return CS_SUCCEED;
}


/*
 * dsp_desc_destroy():
 *
 * Destroys a dsp_desc structure.
 */
void dsp_desc_destroy( d )
    dsp_desc_t  *d;
{
    CS_INT       i;

    if (d != NULL) 
    {
        if (d->d_cols != NULL) 
        {
            for (i = 0; i < d->d_ncols; i++) 
            {
                if (d->d_cols[i].c_native != NULL)
                {
                    free( d->d_cols[i].c_native );
                }

                if (d->d_cols[i].c_data != NULL)
                {
                    free( d->d_cols[i].c_data );
                }
            }

            free( d->d_cols );
        }

        if (d->d_bylist != NULL)
            free( d->d_bylist );

        free( d );
    }
}

/*
 * dsp_just():
 *
 * Determine the type of justification to use for a given data type.
 */
static CS_INT dsp_just( type )
    CS_INT  type;
{
    switch (type) 
    {
        case CS_BIT_TYPE:
        case CS_TINYINT_TYPE:
        case CS_SMALLINT_TYPE:
        case CS_INT_TYPE:
        case CS_REAL_TYPE:
        case CS_FLOAT_TYPE:
        case CS_MONEY_TYPE:
        case CS_MONEY4_TYPE:
        case CS_NUMERIC_TYPE:
        case CS_DECIMAL_TYPE:
        case CS_DATETIME_TYPE:
        case CS_DATETIME4_TYPE:
            return DSP_JUST_RIGHT;
        default:
            break;
    }

    return DSP_JUST_LEFT;
}


/*
 * dsp_dlen():
 *
 * Helper function to return the number of characters required to
 * hold the output of a given data type.
 */
static CS_INT dsp_dlen( fmt )
    CS_DATAFMT   *fmt;
{
    switch (fmt->datatype) 
    {
        case CS_CHAR_TYPE:
        case CS_LONGCHAR_TYPE:
        case CS_TEXT_TYPE:
        case CS_VARCHAR_TYPE:
            return fmt->maxlength;
        case CS_IMAGE_TYPE:
        case CS_BINARY_TYPE:
        case CS_LONGBINARY_TYPE:
        case CS_VARBINARY_TYPE:
        case CS_UNICHAR_TYPE:
            return (2 * fmt->maxlength) + 2; /* sqsh-2.1.8 fix  */
        case CS_BIT_TYPE:
            return 1;
        case CS_TINYINT_TYPE:
            return 3;
        case CS_SMALLINT_TYPE:
            return 6;
        case CS_INT_TYPE:
            return 11;
        case CS_REAL_TYPE:
            /*-- Sign + Decimal + Precision --*/
            return 2 + g_dsp_props.p_real_prec;
        case CS_FLOAT_TYPE:
            /*-- Sign + Decimal + Precision --*/
            return 2 + g_dsp_props.p_flt_prec;
        case CS_MONEY4_TYPE:
            return dsp_money4_len( g_context );
        case CS_MONEY_TYPE:
            return dsp_money_len( g_context );
        case CS_DATETIME_TYPE:
        case CS_DATETIME4_TYPE:
#if defined(CS_DATE_TYPE)
	    case CS_DATE_TYPE:
#endif
#if defined(CS_TIME_TYPE)
	    case CS_TIME_TYPE:
#endif
            return dsp_datetime_len( g_context,  fmt->datatype);
        case CS_NUMERIC_TYPE:
        case CS_DECIMAL_TYPE:
            return (fmt->precision + 3);
        default:
            break;
    }

    return 256;
}
