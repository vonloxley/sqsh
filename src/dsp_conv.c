/*
 * dsp_conv.c - Datatype conversion functions
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
#include <cspublic.h>
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_compat.h"
#include "dsp.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: dsp_conv.c,v 1.1.1.1 2004/04/07 12:35:02 chunkm0nkey Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Local Globals --*/
static char sg_datetime_fmt[64]  = "\0";
static int  sg_datetime_len      = -1;
static int  sg_datetime4_len     = -1;
static int  sg_date_len          = -1;
static int  sg_time_len          = -1;

/*-- Prototypes --*/
static CS_INT dsp_type_len       _ANSI_ARGS(( CS_CONTEXT*, CS_CHAR*, 
                                              CS_DATAFMT*, CS_VOID* ));
static char*  dsp_datetime_strip _ANSI_ARGS(( CS_INT, char*, int ));

/*
 * dsp_datefmt_set():
 *
 * Set the conversion format to be used by the display routine when
 * converting dates.  A format string of NULL or "default" indicates
 * that the default CT-Lib conversion method should be used.  Otherwise
 * a string suitable for use by strftime() should be supplied.
 */
int dsp_datefmt_set( fmt )
    char         *fmt;
{
    /*
     * The dsp_datetime_len() function uses these global variables
     * to store the results of the last time it was called.  Since
     * we are now changing the format from what it was, these
     * variables should be reset.
     */
    sg_datetime_len  = -1;
    sg_datetime4_len = -1;

    /*
     * If the format is NULL or "default", then we simply store
     * away the format to be used as an empty string.
     */
    if (fmt == NULL || strcmp( fmt, "default" ) == 0)
    {
        sg_datetime_fmt[0] = '\0';
        return DSP_SUCCEED;
    }

    /*
     * This is a little cheesy. If the user passes us a string
     * longer than the size of sg_datetime_fmt, then we quietly
     * trim it to the appropriate length.
     */
    strncpy( sg_datetime_fmt, fmt, sizeof(sg_datetime_fmt) );
    return DSP_SUCCEED;
}

/*
 * dsp_datefmt_get():
 *
 * Retrieve the conversion format to be used by the display routine when
 * converting dates.  A format string of NULL or "default" indicates
 * that the default CT-Lib conversion method should be used.
 */
char* dsp_datefmt_get()
{
    if (*sg_datetime_fmt == '\0')
    {
        return "default";
    }

    return sg_datetime_fmt;
}

/*
 * dsp_datetime_len():
 *
 * Calculates the maximum number of characters required to store
 * a date in a string format of the current locale.  This function
 * is similar to dsp_money_len() except that every month is 
 * converted to verify if particular month names are longer than
 * others.
 */
CS_INT dsp_datetime_len( ctxt, type )
    CS_CONTEXT   *ctxt;
    CS_INT        type;
{
    CS_DATAFMT      dt_fmt;
    CS_DATETIME     dt;
    CS_CHAR         dt_buf[64];
    struct tm       tm;
    int             i;
    int             max_month;
    int             max_len;
    int             len;
    char           *fmt;

    /*
     * Check to see if we have cached the value from the last time
     * that this function was called.  If we have, then go ahead
     * and return it.
     */
    if (type == CS_DATETIME_TYPE && sg_datetime_len != -1)
    {
        return sg_datetime_len;
    }

    if (type == CS_DATETIME4_TYPE && sg_datetime4_len != -1)
    {
        return sg_datetime4_len;
    }

#if defined(CS_DATE_TYPE)
    if (type == CS_DATE_TYPE && sg_date_len != -1)
    {
        return sg_date_len;
    }
#endif

#if defined(CS_TIME_TYPE)
    if (type == CS_TIME_TYPE && sg_time_len != -1)
    {
        return sg_time_len;
    }
#endif

    /*
     * If the user is using the default CT-Lib conversion format then
     * we try a brute force test of all of the months in the year,
     * allowing CT-Lib to do a conversion for us.
     */
    max_len = 0;

	/* The user defined conversion with strftime, etc. is not implemented
	   for the CS_DATE_TYPE and CS_TIME_TYPE datatypes */
    if (*sg_datetime_fmt == '\0' || 
#if defined(CS_DATE_TYPE)
		(type == CS_DATE_TYPE || type == CS_TIME_TYPE)
#else
		0
#endif
		)
    {
        dt_fmt.datatype  = type;
        dt_fmt.maxlength = sizeof(CS_DATETIME);
        dt_fmt.locale    = NULL;
        dt_fmt.format    = CS_FMT_UNUSED;
        dt_fmt.scale     = 0;
        dt_fmt.precision = 0;

        /*
         * The algorithm here is pretty simple.  We simply create a date
         * string containing each month of the year, then convert the
         * string to a datetime and back to a string again in the default
         * CT-Lib format.  During this process we keep track of the
         * longest string generated by CT-Lib.
         */
        for (i = 1; i <= 12; i++)
        {
            /*-- Build a made-up day of the month --*/
            sprintf( dt_buf, "%d/12/1997 11:59:53:123PM", i );

            len = dsp_type_len( ctxt, (CS_CHAR*)dt_buf, &dt_fmt, &dt );

            /*-- Keep track of the longest date encountered --*/
            max_len = max(len, max_len);
        }
    }
    else
    {
        /*
         * If the user has supplied a display format to be used in
         * place of the CT-Lib format, then we have a lot of work
         * to do; beginning wth building a reasonable UNIX time
         * structure.
         */
        tm.tm_sec   = 59;
        tm.tm_min   = 59;
        tm.tm_hour  = 23;
        tm.tm_mday  = 11;
        tm.tm_mon   = 0;
        tm.tm_wday  = 2;
        tm.tm_year  = 97;
        tm.tm_isdst = -1;

        /*
         * Strip down the format string as defined by the CS_DATETIME
         * datatype, and replace the ms (%u) with the longest possible
         * number.
         */
        fmt = dsp_datetime_strip( type, sg_datetime_fmt, 999 );

        max_len   = 0;
        max_month = 0;

        /*
         * The first step is to blast through all of the months in the
         * year and determine which one causes the longest string.
         */
        for (i = 0; i < 12; i++)
        {
            tm.tm_mon = i;

            strftime( (char*)dt_buf, sizeof(dt_buf), fmt, &tm );

            len = strlen( dt_buf );

            if (len > max_len)
            {
                max_month = i;
                max_len   = len;
            }
        }

        /*
         * At this point max_month contains the number of the month
         * with the longest length (max_len).  Now we want to blast
         * through every day of the week until we find the longest
         * week day name.
         */
        tm.tm_mon = max_month;

        for (i = 0; i < 7; i++)
        {
            /*
             * The following expression was chosen carefully.  Since
             * we have already tested the 11'th of the month above
             * we can simply test the next six days and we are assured
             * that we have hit every day in the week.
             */
            tm.tm_mday = 11 + i;
            tm.tm_wday = i;

            strftime( (char*)dt_buf, sizeof(dt_buf), fmt, &tm );

            len = strlen( dt_buf );

            max_len = max( len, max_len );
        }
    }

    DBG(sqsh_debug(DEBUG_DISPLAY, 
                   "dsp_datetime_len: %s = %d chars\n", 
                   (type == CS_DATETIME_TYPE)?"CS_DATETIME":"CS_SMALLDATETIME",
                   max_len);)

	switch(type) {
	  case CS_DATETIME_TYPE:
        sg_datetime_len = max_len;
		break;
	  case CS_DATETIME4_TYPE:
		sg_datetime4_len = max_len;
		break;
#if defined(CS_DATE_TYPE)
	  case CS_DATE_TYPE:
		sg_date_len = max_len;
		break;
#endif
#if defined(CS_TIME_TYPE)
	  case CS_TIME_TYPE:
		sg_time_len = max_len;
		break;
#endif
    }

/*	fprintf(stderr, "type = %d, len = %d\n", type, max_len); */

    return (CS_INT)max_len;
}

/*
 * dsp_datetime_conv():
 *
 * Converts the date, dt, in the format of dt_fmt into the buffer buf
 * of length, len according to the conversion style as defined in
 * dsp_datetime_fmt().  This function returns CS_SUCCEED up success or
 * CS_FAIL upon failure (duh!).
 */
CS_RETCODE dsp_datetime_conv( ctx, dt_fmt, dt, buf, len )
    CS_CONTEXT  *ctx;     /* Context */
    CS_DATAFMT  *dt_fmt;  /* Date format */
    CS_VOID     *dt;      /* Pointer to date */
    CS_CHAR     *buf;     /* Buffer */
    CS_INT       len;     /* Length */
{
    struct tm   tm;
    char       *fmt;
    CS_DATEREC  dr;
    CS_DATAFMT  cs_fmt;

    /*
     * If the user has not specified a format, then we let ct-lib
     * do its thing.  This seems to be OK for most platforms.
     */
    if (*sg_datetime_fmt == '\0' ||
#if defined(CS_DATE_TYPE)
		(dt_fmt->datatype == CS_DATE_TYPE || dt_fmt->datatype == CS_TIME_TYPE)
#else
		0
#endif
		)
    {
        cs_fmt.datatype  = CS_CHAR_TYPE;
        cs_fmt.locale    = NULL;
        cs_fmt.format    = CS_FMT_NULLTERM;
        cs_fmt.maxlength = len;
        cs_fmt.scale     = 0;
        cs_fmt.precision = 0;

        if (cs_convert( ctx,                   /* Context */
                        dt_fmt,                /* Date format */
                        dt,                    /* Date */
                        &cs_fmt,               /* String format */
                        (CS_VOID*)buf,         /* String */
                        (CS_INT*)NULL ) != CS_SUCCEED)
        {
            fprintf( stderr, 
                "dsp_datetime_conv: cs_convert(DATETIME->CHAR) failed\n" );
            return CS_FAIL;
        }

        return CS_SUCCEED;
    }

    /*
     * Nope, the user has supplied us with a format to be used to
     * display the date-time, so we need to break into the date
     * into its component chunks.
     */
    cs_dt_crack( ctx, dt_fmt->datatype, (CS_VOID*)dt, (CS_VOID*)&dr );

    /*
     * The OS expects the date/time to be in its own format, so
     * we need to re-organize the ct-lib structure a little bit.
     */
    tm.tm_sec   = dr.datesecond;
    tm.tm_min   = dr.dateminute;
    tm.tm_hour  = dr.datehour;
    tm.tm_mday  = dr.datedmonth;
    tm.tm_mon   = dr.datemonth;
    tm.tm_year  = dr.dateyear - 1900;
    tm.tm_wday  = dr.datedweek;
    tm.tm_yday  = 0;
    tm.tm_isdst = -1;

    /*
     * Take the existing format and strip it down according to the
     * type of date that we are processing and replace the ms
     * field if it exists.
     */
    fmt = dsp_datetime_strip( dt_fmt->datatype, sg_datetime_fmt, 
                              (int)dr.datemsecond );

    /*
     * According to the strftime(3C) documentation, there is no real
     * way to determine the success or failure of this function
     * call, so we have to assume that there are no errors.
     */
    strftime( buf, len, fmt, &tm );

    return CS_SUCCEED;
}

/*
 * dsp_money_len():
 *
 * Since different locales display money values differently (such
 * as with and without commas), this function is used to calculate
 * the maximum number of characters required to represent a given
 * money value.  This is achieved by converting a string representing
 * the largest possible money value (according to the SQL Server
 * documentation) to a native money value, then converting that
 * value back to a string in the format descibed by the locale
 * of the context.
 */
CS_INT dsp_money_len( ctxt )
    CS_CONTEXT   *ctxt;
{
    CS_INT       len;
    CS_DATAFMT   mon_fmt;
    CS_MONEY     mon;

    mon_fmt.datatype  = CS_MONEY_TYPE;
    mon_fmt.maxlength = sizeof(CS_MONEY);
    mon_fmt.locale    = NULL;
    mon_fmt.format    = CS_FMT_UNUSED;
    mon_fmt.scale     = 0;
    mon_fmt.precision = 0;

    /*
     * According to Sybase SQL Server reference manual, this is
     * the largest money value we can represent (well, actually
     * the smallest, but it should be largest in terms of total
     * characters displayed).
     */
    len = dsp_type_len( ctxt, (CS_CHAR*)"-922337203685477.5808",
                              &mon_fmt, (CS_VOID*)&mon );
    
    if (len < 19)
    {
        len = 19;
    }
    
    DBG(sqsh_debug(DEBUG_DISPLAY, "dsp_money_len: CS_MONEY = %d chars\n",
                        len);)

    return len;
}

/*
 * dsp_money4_len()
 *
 * Same as dsp_money_len(), but this function only applies to short-
 * money values.
 */
CS_INT dsp_money4_len( ctxt )
    CS_CONTEXT   *ctxt;
{
    CS_INT       len;
    CS_DATAFMT   mon_fmt;
    CS_MONEY4    mon;

    mon_fmt.datatype  = CS_MONEY4_TYPE;
    mon_fmt.maxlength = sizeof(CS_MONEY4);
    mon_fmt.locale    = NULL;
    mon_fmt.format    = CS_FMT_UNUSED;
    mon_fmt.scale     = 0;
    mon_fmt.precision = 0;

    /*
     * According to Sybase SQL Server reference manual, this is
     * the largest shortmoney value we can represent.
     */
    len = dsp_type_len( ctxt, (CS_CHAR*)"-214748.3648",
                              &mon_fmt, (CS_VOID*)&mon );

    if (len < 10)
    {
        len = 10;
    }

    DBG(sqsh_debug(DEBUG_DISPLAY, "dsp_money4_len: CS_MONEY4 = %d chars\n",
                        len);)

    return len;
}

/*
 * dsp_type_len():
 *
 * Given a dat_str string representing a data described by dat_fmt,
 * dsp_type_len() converts this string into the data type described
 * (dat), then back into a string as defined by the locale of the
 * context, returning the number of characters that were required
 * to do the final conversion.
 */
static CS_INT dsp_type_len( ctxt, dat_str, dat_fmt, dat )
    CS_CONTEXT   *ctxt;
    CS_CHAR      *dat_str;
    CS_DATAFMT   *dat_fmt;
    CS_VOID      *dat;
{
    CS_CHAR     dat_buf[257];  /* We should never have a string longer */
    CS_DATAFMT  str_fmt;       /* Format of string buffer */

    str_fmt.datatype  = CS_CHAR_TYPE;
    str_fmt.locale    = NULL;
    str_fmt.format    = CS_FMT_NULLTERM;
    str_fmt.maxlength = strlen(dat_str);
    str_fmt.scale     = 0;
    str_fmt.precision = 0;

    /*-- Convert data to string --*/
    if (cs_convert( ctxt,                      /* Context */
                    &str_fmt,                  /* Source format */
                    (CS_VOID*)dat_str,         /* Source data */
                    dat_fmt,                   /* Destination format */
                    (CS_VOID*)dat,             /* Destination data */
                    (CS_INT*)NULL ) != CS_SUCCEED)
    {
        fprintf( stderr, "dsp_type_len: cs_convert(CHAR->type #%d) failed\n",
                 (int)dat_fmt->datatype );
        return (CS_INT)0;
    }

    /*-- Convert the data back into a string --*/
    str_fmt.maxlength = sizeof(dat_buf);

    if (cs_convert( ctxt,                      /* Context */
                    dat_fmt,                   /* Source format */
                    (CS_VOID*)dat,             /* Source data */
                    &str_fmt,                  /* Destination format */
                    (CS_VOID*)dat_buf,         /* Destination data */
                    (CS_INT*)NULL ) != CS_SUCCEED)
    {
        fprintf( stderr, "dsp_type_len: cs_convert(type #%d->CHAR) failed\n",
                 (int)str_fmt.datatype );
        return (CS_INT)0;
    }

    return strlen(dat_buf);
}

/*
 * dsp_datetime_strip():
 *
 * Takes a datetime format string (as defined for the dsp_datetime_fmt()
 * function) and strips it down according to the type of date and
 * replaces the ms portion of the date.  The results are returned as
 * a string.
 */
static char* dsp_datetime_strip( type, fmt, ms )
    CS_INT         type;
    char          *fmt;
    int            ms;
{
    static   char new_fmt[64];
    char         *cp;

    /*
     * Now, we want to keep date format that the caller specified,
     * but we want to strip out the milisecond (%u) and the 
     * delimeter for small and regular datetimes ([]).
     */
    for (cp = new_fmt; *fmt != '\0'; ++fmt)
    {
        switch (*fmt)
        {
            case '%':
                if (*(fmt + 1) == 'u')
                {
                    fmt += 2;
                    sprintf( cp, "%03d", ms );
                    cp += 3;
                }
                else
                {
                    *cp++ = *fmt;
                }

                break;

            case '[':
                if (type == CS_DATETIME4_TYPE)
                {
                    while (*fmt != '\0' && *fmt != ']')
                    {
                        ++fmt;
                    }
                }
                break;

            case ']':
                break;
            
            default:
                *cp++ = *fmt;
        }
    }
    *cp = '\0';

    return new_fmt;
}
