/*
 * dsp_xm.c - Display results in a Motif window.
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
 *
 * Note: The Motif code was supplied by John Griffin 
 *       <belved!JOHNG@uunet.uu.net> and has been modified
 *       slightly by Scott C. Gray.
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
static char RCS_Id[] = "$Id: dsp_x.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

#if !defined(USE_X11)

int dsp_x( output, cmd, flags, dsp_func )
	dsp_out_t   *output;
	CS_COMMAND  *cmd;
	int          flags;
	dsp_t       *dsp_func;
{
	fprintf( stderr, "dsp_x: X11 support not compiled into sqsh\n" );
	return DSP_SUCCEED;
}

#else

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

/*
 * cb_data_t:
 *
 * This mostly useless data structure is used to pass around important
 * information between callbacks.  Mainly, this is information about
 * objects whose state can change according to callbacks.
 */
typedef struct cb_data_st {
	int        c_fd;              /* File descriptor for communications */
	int        c_lines;           /* Number of lines displayed */
	int        c_lastpos;
	Widget     c_text;            /* Text widget */
	Widget     c_cmd;
	Widget     c_lbl;
	XtInputId  c_id;              /* Input id */
} cb_data_t;

/*-- Prototypes --*/
static int dsp_x_init _ANSI_ARGS(( int, int, int ));

/*
 * dsp_x:
 *
 * Display routine to display the current result into an x window. This
 * function is interesting in that it creates a pipe-line between itself
 * and the dsp_horiz display style.
 */
int dsp_x( output, cmd, flags, dsp_func )
	dsp_out_t   *output;
	CS_COMMAND  *cmd;
	int          flags;
	dsp_t       *dsp_func;
{
	int    fd_pair[2];               /* Pair of pipes to communicate */
	int    ret;                      /* Return value from display method */
	int    argc;                     /* Required by XtInitialize */
	char  *argv[1];                  /* Required by XtInitialize */
	int    width;                    /* Width of screen */
	int    height;                   /* Height of screen */
	char   number[20];               /* Holder for number */
	char  *cp;
	int    orig_fd;
	int    fd;
	int    orig_width;
	int    i;

	/*
	* First, we would like to figure out how large our X window
	* really is.
	*/
	orig_width = g_dsp_props.p_width;
	if (*g_dsp_props.p_xgeom == '\0')
	{
		width  = 80;
		height = 25;
	}
	else
	{
		i = 0;
		for (cp = g_dsp_props.p_xgeom; *cp != '\0' && isdigit(*cp); ++cp)
		{
			number[i++] = *cp;
		}
		number[i] = '\0';

		width = atoi(number);

		if (*cp != '\0')
		{
			height = atoi( cp + 1 );
		}
		else
		{
			height = 25;
		}

		if (width < 30)
		{
			width = 80;
		}

		if (height < 10)
		{
			height = 25;
		}
	}

	DBG(sqsh_debug(DEBUG_DISPLAY,"dsp_x: width = %d, height = %d\n",
		width, height);)

	/*-- Create a pair of pipes to talk through --*/
	if (pipe(fd_pair) == -1)
	{
		fprintf( stderr, "dsp_x: Unable to create communications pipe\n" );
		return DSP_FAIL;
	}

	switch (fork())
	{
		case   -1:          /* Error condition */
			fprintf( stderr, "dsp_x: Unable to create child process: %s\n",
				strerror(errno)  );
			close( fd_pair[0] );
			close( fd_pair[1] );
			return DSP_FAIL;

		case    0:           /* Child process */
			/*
			 * Keep a handle on the file descriptor to be used to communicate
			 * with our parent (sqsh).  And close the other file descriptor
			 * that we no longer need.
			 */
			fd = fd_pair[0];
			close( fd_pair[1] );
			break;

		default:             /* Parent process */

			/*
			 * Close child's socket.
			 */
			close( fd_pair[0] );

			/*
			 * Since we will be redirecting the file descriptor for
			 * output, we want to make sure it has flushed its contents
			 * before we begin.
			 */
			dsp_fflush( output );

			/*
			 * Because we are going to replace the stdout for this process
			 * we want to make sure we can restore it before we return.
			 */
			orig_fd = dup( output->o_fd );

			/*
			 * Now attach the remaining file descriptor to our output for
			 * the life of the selected display method.
			 */
			dup2( fd_pair[1], output->o_fd );
			close( fd_pair[1] );

			/*
			 * Allow the display method to do whatever it does, however
			 * while it runs, the output will be piped into our child
			 * process that should be displaying it into an X window.
			 */
			g_dsp_props.p_width=2040;
			ret = dsp_func( output, cmd, flags );
			g_dsp_props.p_width=orig_width;

			/*
			 * Now restore the file descriptors back to their original
			 * state.  After that, since output has most likely reached
			 * EOF, we need to clear this error condition before returning,
			 * otherwise OS's, like SunOS, will not be able to use it.
			 */
			dup2( orig_fd, output->o_fd );
			close( orig_fd );

			return ret;
	}

	dsp_x_init( fd, width, height );
	exit(0);
}

/********************************************************************
** 
**                              X11/MOTIF
** 
********************************************************************/

#if defined(USE_MOTIF)

/*-- Include the X headers --*/
#include <Xm/Command.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PanedW.h>
#include <Xm/MainW.h>

/*-- Prototypes --*/
static void dsp_x_input_cb( XtPointer, int*, XtInputId* );
static void dsp_x_ok_cb ( Widget, XtPointer, XtPointer );

static int dsp_x_init( fd, width, height )
	int fd;
	int width;
	int height;
{
	int    i;
	int    nlines;

	/*-- Widgets that make up our window --*/
	Widget       w_top;              /* Top level widget */
	Widget       w_main;             /* Top level widget */
	Widget       w_res_form;         /* Form constraing */
	Widget       w_res;              /* Text Widget */
	Widget       w_sql_form;
	Widget       w_sql_text;
	Widget       w_paned;
	Widget       w_btn_form;
	Widget       w_btn_ok;
	XmString     s_ok;
	XmString     s_cancel;
	cb_data_t    cd;                 /* Data for callbacks */
	XtInputId    id;                 /* Id of input source */
	int          text_width;
	int          orig_width;
	int          argc;
	char        *argv[1];
	char        *cp;

	/*
	 * At this point we are in the child process, the rest is pretty
	 * easy.  We simply open a window and simply place every line we
	 * receive from the parent through the fd file descriptor
	 * into a window.
	 */
	argc    = 0;
	argv[0] = NULL;

	/*
	 * Calculate the number of lines in the SQL Text buffer.
	 */
	nlines = 0;
	for (cp = varbuf_getstr(g_sqlbuf); *cp != '\0'; ++cp)
	{
		if (*cp == '\n')
		{
			++nlines;
		}
	}

	w_top = XtInitialize( "sqsh", "Sqsh", NULL, 0, &argc, argv );

	w_main = 
		XtVaCreateManagedWidget("main", xmMainWindowWidgetClass, w_top,
			NULL);

	w_paned = 
		XtVaCreateManagedWidget("pane", xmPanedWindowWidgetClass, w_main, 
			XmNallowResize,     TRUE,
			NULL);

	w_sql_form = 
		XtVaCreateManagedWidget("sql_form", xmScrolledWindowWidgetClass, w_paned, 
			XmNallowResize,     TRUE,
			NULL);

	w_sql_text = 
		XtVaCreateManagedWidget( "sql_text", xmTextWidgetClass, w_sql_form,
	 		XmNscrollingPolicy, XmAUTOMATIC,
	 		XmNeditMode,        XmMULTI_LINE_EDIT,
	 		XmNeditable,        0,
	 		XmNcolumns,         width,
	 		XmNrows,            nlines,
	 		XmNresizeWidth,     1,
	 		NULL);

	/*
	 * Populate the sql text box with the sql command being
	 * processed.
	 */
	XmTextSetString(w_sql_text, varbuf_getstr(g_sqlbuf) );

	w_res_form = 
		XtVaCreateManagedWidget("w_res_form", xmScrolledWindowWidgetClass, 
			w_paned, 
	 		NULL);

	w_res = 
		XtVaCreateManagedWidget( "text", xmTextWidgetClass, w_res_form,
			XmNscrollingPolicy, XmAUTOMATIC,
			XmNeditMode,        XmMULTI_LINE_EDIT,
			XmNeditable,        0,
			XmNrows,            height,
			XmNresizeWidth,     1,
			XmNcolumns,         width,
	 		NULL);

	w_btn_form = 
		XtVaCreateManagedWidget("btn_form", xmFormWidgetClass, w_paned, 
			XmNrows,            1,
			XmNcolumns,         1,
			XmNresize,          0,
			NULL);
	
	s_ok = XmStringCreateLocalized( "Done" );

	w_btn_ok = 
		XtVaCreateManagedWidget( "ok_button", xmPushButtonWidgetClass, w_btn_form,
			XmNlabelString,     s_ok,
			XmNleftAttachment,  XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			NULL );

	XtAddCallback( w_btn_ok, XmNactivateCallback, dsp_x_ok_cb, &cd ); 

	id = XtAddInput( fd, (XtPointer)XtInputReadMask, 
		(XtInputCallbackProc)dsp_x_input_cb, (XtPointer)&cd );

	/*
	 * We have already attached a cd_data_t to all of our callbacks, so
	 * now we need to populate it with enough info to get work done.
	 */
	cd.c_fd      = fd;
	cd.c_lines   = 0;
	cd.c_lastpos = 0;
	cd.c_text    = w_res;
	cd.c_id      = id;

	XtRealizeWidget( w_top );
	XtMainLoop();

	exit(0);
}

/*
 * dsp_x_input_cb():
 *
 * This function is automatically called whenever input is received
 * from our parent process via a file descriptor.  It is responsible
 * for appending any text received onto the end of the text widget.
 */
static void dsp_x_input_cb (client_data, fd, id)
	XtPointer    client_data;
	int         *fd;
	XtInputId   *id;
{
	static int    len = 0;          /* Total Length of text in widget */
	char          buffer[2048];     /* Read up to 2K of input */
	int           n;
	cb_data_t    *cd;
	char         *cp;
	char          number[20];

	cd = (cb_data_t*)client_data;
	
	/*
	 * Read input from the file descriptor.
	 */
	n = read( *fd, buffer, sizeof(buffer) - 1 );

	/*
	 * If we hit an error or EOF, then close the file descriptor 
	 * and remove it from being an X input source.
	 */
	if (n <= 0)
	{
		close( *fd );
		cd->c_fd = -1;
		XtVaSetValues(cd->c_text, XmNcursorPosition, 0,
		                          XmNtopCharacter,   0, 
		                          NULL);
		XtRemoveInput( cd->c_id );
	}
	else
	{
		buffer[n] = '\0';

		/*
		 * Append the new buffer to what we already have displayed 
		 * in the text window.
		 */
		XmTextReplace( cd->c_text, cd->c_lastpos, cd->c_lastpos + n, buffer );

		cd->c_lastpos += n;
	}
}

static void dsp_x_ok_cb (w, client_data, call_data)
	Widget       w;
	XtPointer    client_data;
	XtPointer    call_data;
{
	exit(0);
}

#else /* USE_MOTIF */

/********************************************************************
** 
**                              X11/ATHENA
** 
********************************************************************/

/*-- Include the X headers --*/
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/TextSrc.h>
#include <X11/Xaw/TextSink.h>
#include <X11/Xaw/AsciiSrc.h>
#include <X11/Xaw/AsciiSink.h>

/*-- Prototypes --*/
static void dsp_x_input_cb( XtPointer, int*, XtInputId* );
static void dsp_x_ok_cb ( Widget, XtPointer, XtPointer );

static int dsp_x_init( fd, width, height )
	int fd;
	int width;
	int height;
{
   Widget       w_top;              /* Top level widget */
   Widget       w_form;             /* Form constraing */
   Widget       w_text;             /* Text Widget */
   Widget       w_cmd;              /* Command widget */
   Widget       w_lbl;              /* Label widget */
   cb_data_t    cd;                 /* Data for callbacks */
   XtInputId    id;                 /* Id of input source */
   int          text_width;
	int          argc;
	char        *argv[1];

   XFontStruct  *font = NULL; /* Font for text widget */

	/*
	 * At this point we are in the child process, the rest is pretty
	 * easy.  We simply open a window and simply place every line we
	 * receive from the parent through the fd file descriptor
	 * into a window.
	 */
	argc    = 0;
	argv[0] = NULL;

	/*-- Intialize our X Session --*/
	w_top = XtInitialize( "sqsh", "Sqsh", NULL, 0, &argc, argv );

	w_form = XtCreateManagedWidget("form", formWidgetClass, w_top, NULL, 0);

	w_text = XtVaCreateManagedWidget( "text", asciiTextWidgetClass, w_form,
		XtNdisplayCaret,      FALSE,
		XtNtop,               XtChainTop,
		XtNvertDistance,      5,
		XtNhorizDistance,     5,
		XtNleft,              XtChainLeft,
		XtNright,             XtChainRight,
		XtNwidth,             200,
		XtNheight,            100,
		XtNscrollHorizontal,  XawtextScrollWhenNeeded,
		XtNscrollVertical,    XawtextScrollWhenNeeded,
		XtNeditType,          XawtextAppend,
		NULL );

	/*-- Now, get the AsciiText's Font structure --*/
	XtVaGetValues( w_text, XtNfont, &font, NULL );

	if (font != NULL)
	{
		text_width = ((font->max_bounds.width + 1) * width) + 5;

		XtVaSetValues( w_text,
			XtNwidth,       text_width,
			XtNheight,      ((font->max_bounds.ascent + 
			                  font->max_bounds.descent) * height)+5,
			NULL );
	}
	else
	{
		/*-- Assume 7 pixel width font --*/
		text_width = (80 * 7) + 5;
	}

	w_lbl = XtVaCreateManagedWidget( "lines", labelWidgetClass, w_form, 
		XtNlabel,            "Lines: 0     ",
		XtNleft,             XtChainLeft,
		XtNbottom,           XtChainBottom,
		XtNjustify,          XtJustifyLeft,
		XtNfromVert,         w_text,
		NULL );

	w_cmd = XtVaCreateManagedWidget( "cancel_button",commandWidgetClass,w_form, 
		XtNlabel,           "Cancel",
		XtNleft,             XtRubber,
		XtNbottom,           XtChainBottom,
		XtNfromVert,         w_text,
		XtNresizable,        FALSE,
		XtNhorizDistance,    ((text_width/2) - 25),
		NULL );

	/*
	 * The command button callback will receive a cb_data_t structure
	 * which will describe enough information to figure out how to
	 * cancel the current result set.
	 */
	XtAddCallback( w_cmd, XtNcallback, dsp_x_ok_cb, &cd ); 

	id = XtAddInput( fd, (XtPointer)XtInputReadMask, 
	                 (XtInputCallbackProc)dsp_x_input_cb, (XtPointer)&cd );

	/*
	 * We have already attached a cd_data_t to all of our callbacks, so
	 * now we need to populate it with enough info to get work done.
	 */
	cd.c_fd    = fd;
	cd.c_lines = 0;
	cd.c_text  = w_text;
	cd.c_cmd   = w_cmd;
	cd.c_lbl   = w_lbl;
	cd.c_id    = id;

	XtRealizeWidget( w_top );
	XtMainLoop();

	exit(0);
}

static void dsp_x_ok_cb (w, client_data, call_data)
	Widget       w;
	XtPointer    client_data;
	XtPointer    call_data;
{
	cb_data_t    *cd;
	char          number[20];

	cd = (cb_data_t*)client_data;

	if (cd->c_fd != -1)
	{
		sprintf( number, "Lines: %d", cd->c_lines );
		XtVaSetValues( cd->c_cmd, XtNlabel, "Done", NULL );
		XtVaSetValues( cd->c_text, XtNeditType, XawtextRead, NULL );
		XtVaSetValues( cd->c_lbl, XtNlabel, number, NULL );

		close( cd->c_fd );
		cd->c_fd = -1;
		XtRemoveInput( cd->c_id );
	}
	else
	{
		exit(0);
	}
}

/*
 * dsp_x_input_cb():
 *
 * This function is automatically called whenever input is received
 * from our parent process via a file descriptor.  It is responsible
 * for appending any text received onto the end of the text widget.
 */
static void dsp_x_input_cb (client_data, fd, id)
	XtPointer    client_data;
	int         *fd;
	XtInputId   *id;
{
	static int    len = 0;          /* Total Length of text in widget */
	char          buffer[1024];     /* Read up to 1K of input */
	XawTextBlock  b;
	int           n;
	cb_data_t    *cd;
	char         *cp;
	char          number[20];

	cd = (cb_data_t*)client_data;

	/*-- Read input from file descriptor --*/
	n = read( *fd, buffer, sizeof(buffer) - 1 );

	/*-- On error or EOF close connection --*/
	if (n <= 0)
	{
		sprintf( number, "Lines: %d", cd->c_lines );
		XtVaSetValues( cd->c_cmd, XtNlabel, "Done", NULL );
		XtVaSetValues( cd->c_text, XtNeditType, XawtextRead, NULL );
		XtVaSetValues( cd->c_lbl, XtNlabel, number, NULL );

		close( *fd );
		cd->c_fd = -1;
		XtRemoveInput( cd->c_id );
	}
	else
	{
		/*-- Append data read to the end --*/
		b.firstPos = 0;
		b.length   = n;
		b.ptr      = buffer;
		b.format   = FMT8BIT;

		cp = buffer;
		while ((cp = strchr( cp, '\n' )) != NULL)
		{
			++cd->c_lines;

			if ((cd->c_lines % 100) == 0)
			{
				sprintf( number, "Lines: %d", cd->c_lines );
				XtVaSetValues( cd->c_lbl, XtNlabel, number, NULL );
			}

			cp += 1;
		}

		XawTextReplace( cd->c_text, len, len, &b );

		len += n;
	}
}

#endif /* !USE_MOTIF */
#endif /* USE_X11 */
