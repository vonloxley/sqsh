/*
** sqsh_ctx.c - Manipulate a context of execution.
**
** Copyright (C) 1995, 1996 by Scott C. Gray
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, write to the Free Software
** Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
**
** You may contact the author :
**   e-mail:  gray@voicenet.com
**            grays@xtend-tech.com
**            gray@xenotropic.com
*/
#include <stdio.h>
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_ctx.h"

/*
** sqsh_ctx_t: This data structure is used to hold the context of
**    a sqsh block. 
*/
typedef struct sqsh_ctx_st
{
	int                 ctx_argc;      /* Argument count to current context */
	char              **ctx_argv;      /* Array of arguments */
	CS_CONNECTION      *ctx_conn;      /* Connection to the database */
	dsp_desc_t         *ctx_cols;      /* Columns referenced from \do loop */
	varbuf_t           *ctx_sqlbuf;    /* Current SQL command */
	struct sqsh_ctx_st *ctx_prev;      /* Previous context */
}
sqsh_ctx_t;

/*
** sg_cur_ctx: Pointer to the current execution context.
*/
static sqsh_ctx_t *sg_cur_ctx;

/*
** sqsh_ctx_push():
**
** Allocates a new execution context.
*/
int sqsh_ctx_push()
{
	sqsh_ctx_t   *ctx;

	ctx = (sqsh_ctx_t*)malloc(sizeof(sqsh_ctx_t));

	if (ctx == NULL)
	{
		sqsh_set_error( SQSH_E_NOMEM, 
			"sqsh_ctx_push: Memory allocation failure\n" );
		return(False);
	}

	ctx->ctx_argc   = -1;
	ctx->ctx_argv   = NULL;
	ctx->ctx_conn   = NULL;
	ctx->ctx_cols   = NULL;
	ctx->ctx_sqlbuf = NULL;
	ctx->ctx_prev   = sg_cur_ctx;

	sg_cur_ctx = ctx;
	return(True);
}

/*
** sqsh_ctx_pop():
**
** Discard the current context and return to the previous one.
** This will automatically close any connection and the
** current column descriptions and sqlbuffer for the
** context.
*/
int sqsh_ctx_pop()
{
	sqsh_ctx_t  *ctx;
	CS_INT       conn_status;

	if (sg_cur_ctx == NULL)
	{
		fprintf( stderr,
			"sqsh_ctx_pop: No more contexts left!\n" );
		return(False);
	}

	ctx = sg_cur_ctx;
	sg_cur_ctx = sg_cur_ctx->ctx_prev;

	if (ctx->ctx_conn != NULL)
	{
		if (ct_con_props( ctx->ctx_conn, CS_GET, CS_CON_STATUS,
			(CS_VOID*)&conn_status, CS_UNUSED, (CS_INT*)NULL ) != CS_SUCCEED)
		{
			conn_status = CS_CONSTAT_CONNECTED;
		}

		if ((conn_status & CS_CONSTAT_CONNECTED) 
			== CS_CONSTAT_CONNECTED)
		{
			ct_close( ctx->ctx_conn, CS_FORCE_CLOSE );
		}

		ct_con_drop( ctx->ctx_conn );
	}

	if (ctx->ctx_cols != NULL)
	{
		dsp_desc_destroy( ctx->ctx_cols );
	}

	if (ctx->ctx_sqlbuf != NULL)
	{
		varbuf_destroy( ctx->ctx_sqlbuf );
	}

	free( ctx );
	return(True);
}

/*
** sqsh_ctx_set_args():
**
** Set an argument set for the current context.  Note that
** the argv[] array will not be freed when the context 
** returns. This is the duty of the function that allocated
** the context.
*/
int sqsh_ctx_set_args (argc, argv)
	int   argc;
	char *argv[];
{
	if (sg_cur_ctx == NULL)
	{
		fprintf( stderr, 
			"sqsh_ctx_set_args: There is no available execution context!\n" );
		return(False);
	}

	sg_cur_ctx->ctx_argc = argc;
	sg_cur_ctx->ctx_argv = argv;
	return(True);
}

/*
** sqsh_ctx_get_args():
**
** Fetch the current argument list.
*/
int sqsh_ctx_get_args (argc, argv)
	int    *argc;
	char ***argv;
{
	sqsh_ctx_t  *ctx;

	for (ctx = sg_cur_ctx; ctx != NULL; ctx = ctx->ctx_prev)
	{
		if (ctx->ctx_argv != NULL && ctx->ctx_argc >= 0)
		{
			*argc = ctx->ctx_argc;
			*argv = ctx->ctx_argv;
			return(True);
		}
	}

	*argc = -1;
	*argv = NULL;
	return(False);
}

/*
** sqsh_ctx_set_conn():
**
** Attaches a connection to the current context.  This connection
** will automatically be closed and destroyed when the current
** context is dropped.
*/
int sqsh_ctx_set_conn (conn)
	CS_CONNECTION *conn;
{
	if (sg_cur_ctx == NULL)
	{
		fprintf( stderr, 
			"sqsh_ctx_set_conn: There is no available execution context!\n" );
		return(False);
	}

	if (sg_cur_ctx->ctx_conn != NULL)
	{
		fprintf( stderr,
			"sqsh_ctx_set_conn: There is already an attached connectioN!\n" );
		return(False);
	}

	sg_cur_ctx->ctx_conn = conn;
	return(True);
}

/*
** sqsh_ctx_get_conn():
**
** Fetch the current connection.
*/
int sqsh_ctx_get_conn (conn)
	CS_CONNECTION **conn;
{
	sqsh_ctx_t  *ctx;

	for (ctx = sg_cur_ctx; ctx != NULL; ctx = ctx->ctx_prev)
	{
		if (ctx->ctx_conn != NULL)
		{
			*conn = ctx->ctx_conn;
			return(True);
		}
	}

	*conn = NULL;
	return(False);
}

/*
** sqsh_ctx_set_cols():
**
** Attaches a description of the a result set to the current context.
** This is only utilized by the \do loop right now.
*/
int sqsh_ctx_set_cols (desc)
	dsp_desc_t *desc;
{
	if (sg_cur_ctx == NULL)
	{
		fprintf( stderr, 
			"sqsh_ctx_set_cols: There is no available execution context!\n" );
		return(False);
	}

	if (sg_cur_ctx->ctx_cols != NULL)
	{
		fprintf( stderr,
			"sqsh_ctx_set_cols: There is already a column description in the "
			"current exuection context!\n" );
		return(False);
	}

	sg_cur_ctx->ctx_cols = desc;
	return(True);
}

/*
** sqsh_ctx_get_cols():
**
** Fetch the current column set..
*/
int sqsh_ctx_get_cols (desc)
	dsp_desc_t **desc;
{
	sqsh_ctx_t  *ctx;

	for (ctx = sg_cur_ctx; ctx != NULL; ctx = ctx->ctx_prev)
	{
		if (ctx->ctx_cols != NULL)
		{
			*desc = ctx->ctx_cols;
			return(True);
		}
	}

	*desc = NULL;
	return(False);
}

/*
** sqsh_ctx_set_sqlbuf():
**
** Attach a new SQL buffer to the current context.
*/
int sqsh_ctx_set_sqlbuf (sqlbuf)
	varbuf_t  *sqlbuf;
{
	if (sg_cur_ctx == NULL)
	{
		fprintf( stderr, 
			"sqsh_ctx_set_sqlbuf: There is no available execution context!\n" );
		return(False);
	}

	if (sg_cur_ctx->ctx_sqlbuf != NULL)
	{
		fprintf( stderr,
			"sqsh_ctx_set_cols: There is already a sql buffer in the "
			"current exuection context!\n" );
		return(False);
	}

	sg_cur_ctx->ctx_sqlbuf = sqlbuf;
	return(True);
}

/*
** sqsh_ctx_get_sqlbuf():
**
** Get the current sql buffer.
*/
int sqsh_ctx_get_sqlbuf (sqlbuf)
	varbuf_t  **sqlbuf;
{
	sqsh_ctx_t  *ctx;

	for (ctx = sg_cur_ctx; ctx != NULL; ctx = ctx->ctx_prev)
	{
		if (ctx->ctx_sqlbuf != NULL)
		{
			*sqlbuf = ctx->ctx_sqlbuf;
			return(True);
		}
	}

	*sqlbuf = NULL;
	return(False);

}
