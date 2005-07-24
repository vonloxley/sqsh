/*
 * dsp.h - Result display header file.
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
#ifndef dsp_h_included
#define dsp_h_included

/*
 * The following codes are value return values for a result set
 * display routine.
 */
#define DSP_SUCCEED         1   /* Succesfully processed results */
#define DSP_INTERRUPTED     2   /* Interrupt encountered during processing */
#define DSP_FAIL            3   /* Another type of failure encountered */

/*-- Justification Types --*/
#define DSP_JUST_RIGHT      1   /* Justify column to right */
#define DSP_JUST_LEFT       2   /* Justify column to left */

/*-- Process Flags --*/
#define DSP_PROC_HEADER  (1<<0)   /* Column header has been displayed */
#define DSP_PROC_SEP     (1<<1)   /* Column separator has been displayed */
#define DSP_PROC_DATA    (1<<2)   /* Column data has been displayed */

/*
 * dsp_col_t: This data structure is used internally to describe
 * a single column of data, including the format for the data
 * coming from the server, as well as the whether or not the
 * data is NULL.
 */
typedef struct _dsp_col_t {
	CS_INT      c_colid;           /* Column number */
	CS_DATAFMT  c_format;          /* Display format for column */
	CS_SMALLINT c_nullind;         /* Null indicator for column */
	CS_BOOL     c_is_native;       /* True of False if data is bound native */
	CS_VOID    *c_native;          /* Place to hold native format */
	CS_INT      c_native_len;      /* Length of native data */
	CS_CHAR    *c_data;            /* Character string representation of data */
	CS_INT      c_maxlength;       /* Maximum number of characters in data */
	CS_INT      c_justification;   /* Type of justification to use */

	CS_INT      c_column_id;       /* Compute column, source column id */
	CS_INT      c_aggregate_op;    /* Compute column, aggregate operator */

	/*
	 * The following is a cheezy little flag placed here for use by
	 * dsp_horiz.c to indicate if the compute column has been processed
	 * yet...icky.  It is guarenteed to be set to False when a compute
	 * set is created.
	 */
	CS_INT      c_processed;       /* See processed flags, above */

	/*
	 * The following is another cheesy field used by dsp_pretty to keep
	 * track of its progress when displaying a single line of a long
	 * column.
	 */
	CS_CHAR    *c_ptr;

	/*
	 * Once again, for dsp_pretty, this is used to hold the width
	 * of the column to be displayed.
	 */
	CS_INT      c_width;

} dsp_col_t;

/*
 * dsp_desc_t: Represents a single result set, including the type
 * of result set, the number of columns in it, and descriptions
 * for each column.
 */
typedef struct _dsp_desc_t {
	CS_INT       d_type;            /* Type of result set */
	CS_INT       d_ncols;           /* Number of columns in result set */
	dsp_col_t   *d_cols;            /* Array of column info */

	CS_INT       d_bylist_size;     /* Compute results, length of bylist */
	CS_SMALLINT *d_bylist;          /* Compute results, bylist */
} dsp_desc_t;


/*
 * The following flags are accepted by all or most display functions
 * to alter their default behaviours.  It is not expected that every
 * display function know how to interpret every flag.
 */
#define DSP_F_NOHEADERS     (1<<0)  /* Suppress headers */
#define DSP_F_NOFOOTERS     (1<<1)  /* Suppress footers */
#define DSP_F_NOTHING       (1<<3)  /* Everything */
#define DSP_F_X             (1<<4)  /* Send output to X Window */

/*
 * sg_dsp_interrupted:  This variable (defined in dsp.c) is set to True
 *        when an interrupt is encountered while a display routine is
 *        doing its thing.  Each display routine is expected to poll this
 *        variable from time to time.
 */
extern int g_dsp_interrupted;

/*-- Types of actions for dsp_prop() --*/
#define DSP_GET           1
#define DSP_SET           2

/*-- Types of properties for dsp_prop() --*/
#define DSP_WIDTH         1
#define DSP_COLSEP        2
#define DSP_LINESEP       3
#define DSP_XGEOM         4
#define DSP_STYLE         5
#define DSP_DATEFMT       6
#define DSP_FLOAT_SCALE   7
#define DSP_FLOAT_PREC    8
#define DSP_REAL_SCALE    9
#define DSP_REAL_PREC    10
#define DSP_COLWIDTH     11
#define DSP_OUTPUTPARMS  12
#define DSP_BCP_COLSEP   13
#define DSP_BCP_ROWSEP   14
#define DSP_BCP_TRIM     15
#define DSP_MAXLEN       16
#define DSP_CSV_COLSEP   17
#define DSP_CSV_ROWSEP   18
#define DSP_VALID_PROP(p) ((p) >= DSP_WIDTH && (p) <= DSP_CSV_ROWSEP)


/*-- Length for dsp_prop() --*/
#define DSP_NULLTERM     -1
#define DSP_UNUSED       -2

/*-- Different display styles for DSP_STYLE property --*/
#define DSP_BCP           1
#define DSP_HORIZ         2
#define DSP_VERT          3
#define DSP_HTML          4
#define DSP_NONE          5
#define DSP_META          6
#define DSP_PRETTY        7
#define DSP_CSV           8
#define DSP_VALID_STYLE(s) ((s) >= DSP_BCP && (s) <= DSP_CSV)


/*-- External Prototypes --*/
int     dsp_cmd      _ANSI_ARGS(( FILE*, CS_COMMAND*, char*, int ));
int     dsp_prop     _ANSI_ARGS(( int, int, void*, int ));

/******************************************************************
 **                     INTERNAL DEFINITIONS                     **
 ******************************************************************/


/*
 * The following data structure is very similar to a FILE*. It
 * is used to provide signal safe buffered I/O.
 */
#define DSP_BUFSIZE  1024
typedef struct dsp_out_st {
	int          o_fd;
	FILE        *o_file;
	int          o_nbuf;
	char         o_buf[DSP_BUFSIZE];
} dsp_out_t;

/*
 * This is used to represent a pointer to a display style function
 * within the internals of the dsp sub-system.
 */
typedef int (dsp_t) _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, int ));

/*-- Limits --*/
#define MAX_SEPLEN   32      /* Longest allowable line or col separator */
#define MAX_XGEOM     9      /* Maximum X geometry NNNNxNNNN */

/*
 * The following data structure is used to represent all of the 
 * available properties within the display sub-system.
 */
typedef struct dsp_prop_st {
	int     p_style;                 /* Current display style */
	int     p_width;                 /* Current width of the screen */
	char    p_colsep[MAX_SEPLEN+1];  /* Current column separator */
	int     p_colsep_len;            /* Display len of the column separator */
	char    p_linesep[MAX_SEPLEN+1]; /* Current Line separator */
	int     p_linesep_len;           /* Display length of the line separator */
	char    p_xgeom[MAX_XGEOM+1];    /* Maximum xgeometry */
	int     p_flt_prec;              /* Total precision for floats */
	int     p_flt_scale;             /* Scale for floats */
	int     p_real_prec;             /* Total precision for reals */
	int     p_real_scale;            /* Scale for reals */
	int     p_colwidth;              /* Max column width in pretty mode */
	int     p_outputparms;           /* Display output parameters? */
	char    p_bcp_colsep[MAX_SEPLEN+1];  /* Current bcp column separator */
	int     p_bcp_colsep_len;        /* Display len of bcp column separator */
	char    p_bcp_rowsep[MAX_SEPLEN+1];  /* Current bcp row separator */
	int     p_bcp_rowsep_len;        /* Display len of bcp row separator */
	int     p_bcp_trim;              /* True/False: Trim spaces in bcp? */
	int     p_maxlen;                /* Maximum column width we will accept */
    char    p_csv_colsep[MAX_SEPLEN+1]; /* Current csv column separator */
    int     p_csv_colsep_len;        /* Display len of csv column separator */
} dsp_prop_t;


/*
 * g_dsp_props: This variable is global to all display functions and
 * will contain the current set of properties as defined by the user
 * (or as defaulted).
 */
extern dsp_prop_t g_dsp_props;
	
/*-- Internal Prototypes --*/
dsp_out_t*  dsp_fopen         _ANSI_ARGS(( FILE* ));
int         dsp_fputc         _ANSI_ARGS(( int, dsp_out_t* ));
int         dsp_fputs         _ANSI_ARGS(( char*, dsp_out_t* ));
int         dsp_fflush        _ANSI_ARGS(( dsp_out_t* ));
int         dsp_fprintf       _ANSI_ARGS(( dsp_out_t*, char*, ... ));
int         dsp_fclose        _ANSI_ARGS(( dsp_out_t* ));
int         dsp_horiz         _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, int ));
int         dsp_meta          _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, int ));
int         dsp_vert          _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, int ));
int         dsp_bcp           _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, int ));
int         dsp_csv           _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, int ));
int         dsp_html          _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, int ));
int         dsp_none          _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, int ));
int         dsp_pretty        _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, int ));
int         dsp_x             _ANSI_ARGS(( dsp_out_t*, CS_COMMAND*, int, dsp_t* ));
int         dsp_datefmt_set   _ANSI_ARGS(( char* ));
char*       dsp_datefmt_get   _ANSI_ARGS(( void ));
dsp_desc_t* dsp_desc_bind     _ANSI_ARGS(( CS_COMMAND*, CS_INT ));
CS_INT      dsp_desc_fetch    _ANSI_ARGS(( CS_COMMAND*, dsp_desc_t* ));
void        dsp_desc_destroy  _ANSI_ARGS(( dsp_desc_t* ));
CS_INT      dsp_datetime_len  _ANSI_ARGS(( CS_CONTEXT*, CS_INT ));
CS_INT      dsp_datetime4_len _ANSI_ARGS(( CS_CONTEXT* ));
CS_INT      dsp_money_len     _ANSI_ARGS(( CS_CONTEXT* ));
CS_INT      dsp_money4_len    _ANSI_ARGS(( CS_CONTEXT* ));
CS_RETCODE  dsp_datetime_conv _ANSI_ARGS(( CS_CONTEXT*, CS_DATAFMT*, CS_VOID*,
                                           CS_CHAR*, CS_INT ));

#if defined(DEBUG)
char*   dsp_result_name _ANSI_ARGS(( CS_INT ));
#endif

#endif
