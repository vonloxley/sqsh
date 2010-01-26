/*
 * sqsh_job.c - Functions for launching commands
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
#include <sys/param.h>
#include <ctype.h>
#include <setjmp.h>
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_fd.h"
#include "sqsh_tok.h"
#include "sqsh_cmd.h"
#include "sqsh_global.h"
#include "sqsh_fork.h"
#include "sqsh_expand.h"
#include "sqsh_strchr.h"
#include "sqsh_alias.h"
#include "sqsh_getopt.h"
#include "sqsh_sig.h"
#include "sqsh_job.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_job.c,v 1.2 2009/04/14 10:46:27 mwesdorp Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Local Prototypes --*/
static job_t*     job_alloc         _ANSI_ARGS((jobset_t*));
static int        job_free          _ANSI_ARGS((job_t*));
static job_id_t   jobset_wait_all   _ANSI_ARGS((jobset_t*, int*, int));
static int        jobset_parse      _ANSI_ARGS((jobset_t*, job_t*, char*, 
                                                varbuf_t*, int));
static job_t*     jobset_get        _ANSI_ARGS((jobset_t*, job_id_t));
static void       jobset_run_sigint _ANSI_ARGS((int, void*));
static int        jobset_get_cmd    _ANSI_ARGS((jobset_t*, char*, cmd_t**));
static int        jobset_global_init _ANSI_ARGS((void));

/*-- Status Globals --*/
static JMP_BUF sg_jmp_buf ;           /* Where to go on SIGINT */

/*
 * sg_cmd_buf: Variable length buffer to be used to expand the
 *             current command to be executed.
 */
static varbuf_t *sg_cmd_buf = NULL ;    /* Expanded command buffer */

/*
 * sg_alias_buf: Variable length buffer to be used when performing
 *               alias expansion (prior to command expansion).
 */
static varbuf_t *sg_alias_buf = NULL ;  /* Expanded alias buffer */

/*
 * sg_while_buf: Variable length buffer used to hold the (mostly)
 *               unparsed command line for a \while statement.
 */
static varbuf_t *sg_while_buf = NULL ;  /* Expanded alias buffer */

/*
 * jobset_create():
 *
 * Creates and attaches a new jobset structure to the current context.
 * Upon success, True is return, otherwise False is returned.
 */
jobset_t* jobset_create( hsize )
	int hsize;
{
	jobset_t   *js;
	int         i;

	/*-- Always check your parameters --*/
	if( hsize < 1 ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	/*-- Attempt to allocate a new jobset structure --*/
	if( (js = (jobset_t*)malloc( sizeof( jobset_t ) )) == NULL ) {
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return NULL;
	}

	/*-- Allocate the hash table for the job_id's --*/
	if( (js->js_jobs = (job_t**)malloc(sizeof(job_t*)*hsize)) == NULL ){
		free( js );
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return NULL;
	}

	/*
	 * Allocate a sigcld_t.  This will act as a handle on all of the
	 * SIGCHLD events that will be received due to background jobs
	 * completing.
	 * sqsh-2.1.7 - Logical fix: Free js->js_jobs before a free of js.
	 */
	if( (js->js_sigcld = sigcld_create()) == NULL ) {
		free( js->js_jobs );
		free( js );
		sqsh_set_error( sqsh_get_error(), "sigcld_create: %s",
		                sqsh_get_errstr() );
	 	return NULL;
	}

	/*-- Initialize our command structure --*/
	js->js_hsize   = hsize;

	for( i = 0; i < hsize; i++ )
		js->js_jobs[i] = NULL;

	sqsh_set_error( SQSH_E_NONE, NULL );
	return js;
}


/*
 * jobset_is_cmd():
 *
 * Returns a positive value of cmd_line is a command string, 0 if it
 * does not, and a negative value upon error.
 */
int jobset_is_cmd( js, cmd_line )
	jobset_t   *js;
	char       *cmd_line;
{
	cmd_t  *cmd;

	sqsh_set_error( SQSH_E_NONE, NULL );

	/*-- Alias' are assumed to be commands --*/
	if( alias_test( g_alias, cmd_line ) > 0 )
		return 1;
	return jobset_get_cmd( js, cmd_line, &cmd );
}

/*
 * jobset_get_cmd():
 *
 */
static int jobset_get_cmd( js, cmd_line, cmd )
	jobset_t   *js;
	char       *cmd_line;
	cmd_t     **cmd;
{
	tok_t      *tok ;               /* Token returned from sqsh_tok() */

	/*
	 * The jobset_global_init() initializes the sg_cmd_buf global
	 * variable which we will need to expand the command name
	 * into.
	 */
	if( jobset_global_init() == False )
		return -1;

	/*
	 * Now we want to expand just the first syntactical word of
	 * the command line into a temporary buffer.  If there is an
	 * error then we just assume that it is bad quoting or something
	 * like that and therefore this isn't a valid command line.
	 */
	if( sqsh_nexpand( cmd_line, sg_cmd_buf, 0, EXP_WORD ) == False )
		return 0;

	DBG(sqsh_debug( DEBUG_JOB, "jobset_get_cmd: Testing '%s'\n",
	                varbuf_getstr(sg_cmd_buf) );)
	
	/*
	 * Try to grab the first token from the command line.  If there
	 * is any error while tokenizing, then we will assume that this
	 * isn't a command.
	 */
 	if( sqsh_tok( varbuf_getstr( sg_cmd_buf ), &tok, 0 ) == False )
		return 0;

	/*
	 * So, if the first token we retrieve isn't a word (i.e. it is a
	 * pipe, or a redirection or something), or we couldn't find the
	 * command in our global set of commands, then we don't have a
	 * command.
	 */
	if( tok->tok_type != SQSH_T_WORD || 
	    (*cmd = cmdset_get(g_cmdset, sqsh_tok_value(tok))) == NULL )
		return 0;

	return 1;
}

/*
 * jobset_run():
 *
 * Attempts to run the command contained in cmd_line.  If the job is
 * succesfully run and completes (i.e. wasn't a background job), then
 * 0 is returned and exit_status will contained the exit status of the
 * command. If the job was succesfully started and is still running then
 * a valid job_id_t is returned as a handle to the running job.
 * If there was any sort of error then -1 is returned.
 */
job_id_t jobset_run( js, cmd_line, exit_status )
	jobset_t      *js;
	char          *cmd_line;
	int           *exit_status;
{
	cmd_t      *cmd ;                  /* The command being run */
	char        msg[512];
	int         error;
	pid_t       child_pid;
	int         ret;
	int         hval;
	job_t      *job;
	int         tok_flags;

#if defined(USE_AIX_FIX)
	int          old_flags = (int)-1;
#endif /* USE_AIX_FIX */
	varbuf_t    *while_buf;

	/*-- Check the arguments --*/
	if( js == NULL || cmd_line == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return -1;
	}

	/*
	 * We need to initialize a couple of global buffers that
	 * we are going to use, namely sg_cmd_buf.
	 */
	if( jobset_global_init() == False )
		return -1;

	/*
	 * Perform alias expansion.
	 */
	if( alias_expand( g_alias, cmd_line, sg_alias_buf ) > 0 )
		cmd_line = varbuf_getstr( sg_alias_buf );

	/*
	 * Attempt to retrieve the command from the command line.  If it
	 * wasn't a command, then return an error condition.  jobset_get_cmd
	 * should take care of setting the appropriate error message for
	 * us.
	 */
	if( (ret = jobset_get_cmd( js, cmd_line, &cmd )) <= 0 ) {
		if( ret == 0 )
			sqsh_set_error( SQSH_E_EXIST, "Invalid command" );
		return -1;
	}

	/*
	 * Token flags control how the tokenizer behaves.
	 */
	tok_flags = 0;

	/*
	 * When parsing the \if expression, expand '[' and ']' to be
	 * a test statement.
	 */
	if (strcmp( cmd->cmd_name, "\\if" ) == 0)
	{
		tok_flags |= TOK_F_TEST;
	}

	/*
	 * \while is a special case. We need to make sure that
	 * we do *not* expand the command line of variables and
	 * that the command line is passed in as a single string.
	 */
	if (strcmp( cmd->cmd_name, "\\while" ) == 0)
	{
		/*
		 * Leave all quotes and other goodies in-tact.
		 */
		tok_flags |= TOK_F_LEAVEQUOTE|TOK_F_TEST;

		/*
		 * We'll pass this varbuf_t into the tokenizer to fill with
		 * the unparsed command line.
		 */
		while_buf = sg_while_buf;
		varbuf_clear( while_buf );
	}
	else
	{
		/*
		 * Let the parser know that this isn't a \while statement.
		 */
		while_buf = NULL;

		/*
		 * Expand the command line of any tokens, keeping quotes and stripping
		 * escape characters (i.e. removing them as part of the expansion
		 * process).
		 */
		if (sqsh_expand( cmd_line, sg_cmd_buf, EXP_COLUMNS ) == False)
		{
			sqsh_set_error( sqsh_get_error(), "sqsh_expand: %s", 
								 sqsh_get_errstr() );
			return -1;
		}

		/*
		 * The "actual" command line is the version that we just expanded
		 * of any variables.
		 */
		cmd_line = varbuf_getstr( sg_cmd_buf );
	}

	/*
	 * It was a command, so lets allocate a new job structure for
	 * the duration of the command.
	 */
	if ((job = job_alloc(js)) == NULL)
	{
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return -1;
	}

	/*
	 * Well, it looks like we are in for the long hual.  The first
	 * thing we need to do at this point is back up the state of all
	 * of our file descriptors. This allows jobset_parse() to do all
	 * of the opening and closing of file descriptors that it could
	 * possible want to and a single call to sqsh_frestore() will
	 * restore the current state. 
	 */
	if (sqsh_fsave() == -1)
	{
		sqsh_set_error( sqsh_get_error(), "sqsh_fsave: %s", sqsh_get_errstr() );
		job_free( job );
		return -1;
	}

#if defined(USE_AIX_FIX)
	old_flags = stdout->_flag;
#endif /* USE_AIX_FIX */

	/*
	 * Now, let jobset_parse() do its thing and redirect stdout
	 * and the such as necessary.  If it has an error then we just send
	 * it back with an error code.
	 */
	if (jobset_parse( js, job, cmd_line, while_buf, tok_flags ) == False)
	{
		goto jobset_abort;
	}

	/*
	 * If this was a \while statement, then the sole argument to the
	 * function will be the unexpanted command line stripped of
	 * all redirection crap.
	 */
	if (while_buf != NULL)
	{
		if (args_add(job->job_args, "\\while" ) == False ||
			args_add(job->job_args, varbuf_getstr(while_buf)) == False)
		{
			sqsh_set_error( sqsh_get_error(), "args_add: %s", 
				sqsh_get_errstr() );
			goto jobset_abort;
		}
	}

	/*
	 * It parsed OK, so now job->job_args contains the arguments to 
	 * the command and stdout and stderr are redirected as needed.
	 * the only thing left is to fork if necessary and call the
	 * appropriate function.
	 */
	if (job->job_flags & JOB_BG)
	{
		/*
		 * Perform the actual fork().  Sqsh_fork() will take care of
		 * resetting certain global variables in the context of the
		 * child.
		 */
		switch ((child_pid = sqsh_fork()))
		{
			case -1 :  /* Error */
				goto jobset_abort;
			
			/*
			 * Child process.  There really isn't much to do here other
			 * than call the requested cmd and exit with the return value
			 * of that command. The parent will receive the exit status.
			 */
			case  0 :  /* Child process */
				/*
				 * We want the child to process signals a little differently
				 * from the parent.  There is no real need for graceful re-
				 * covery within the child.
				 */
				while (sig_restore() >= 0);
					
				ret = cmd->cmd_ptr( args_argc(job->job_args), 
					args_argv(job->job_args) );
				exit(ret);

			/*
			 * Parent process. All we need to do is record the pid of the
			 * child in the job structure, restore the  file descriptors
			 * and return.
			 */
			default :  /* Parent */
				
				job->job_pid = child_pid;
				sigcld_watch( js->js_sigcld, job->job_pid );
		}

		/*
		 * Restore the file descriptors redirected during jobset_parse()
		 * I should probably do something if this fails, but I can't think
		 * of any way to recover.
		 */
		sqsh_frestore();

		/*
		 * The following is required on those systems in which EOF isn't
		 * checked each time a read operation is attempted on a FILE
		 * structure.  This is in case stdin was temporarily redirected
		 * from a file that has reached EOF.
		 */
		clearerr( stdin );

		/*
		 * Link the new job into our hash table.
		 */
	 	hval = job->job_id % js->js_hsize;
	 	job->job_nxt = js->js_jobs[hval];
	 	js->js_jobs[hval] = job;

		/*-- And return it --*/
		return job->job_id;
	}

	/*
	 * This isn't a background job, so we just need to call the command
	 * and return its exit status. We need to install a SIGINT handle
	 * to capture any ^C's from a user.  In this case we want the
	 * ^C to return immediately before the sqsh_restore() call below.
	 */
	sig_save();

	sig_install( SIGINT, jobset_run_sigint, (void*)NULL, 0 );
	if (SETJMP( sg_jmp_buf ) == 0)
	{
		/*
		 * Since most commands use sqsh_getopt() to parse the command
		 * line options, we want to ensure that sqsh_getopt() will
		 * know that it is receiving a new argv[] and argc so we want
		 * to reset its existing impression of these two arguments.
		 */
		sqsh_getopt_reset();

		/*
		 * If this is a pipe-line then we want to ignore the SIGPIPE.  Let the
		 * command itself set up a handler for it if it wishes.
		 */
		if (job->job_flags & JOB_PIPE)
		{

			sig_install( SIGPIPE, SIG_H_POLL, (void*)NULL, 0 );
			*exit_status = cmd->cmd_ptr( args_argc(job->job_args), 
												  args_argv(job->job_args) );

		}
		else
		{
			*exit_status = cmd->cmd_ptr( args_argc(job->job_args), 
												  args_argv(job->job_args) );
		}
	}

	/*-- Restore signal handlers --*/
	sig_restore();
	
	/*
	 * Restore the file descriptors that were redirected during the
	 * process of parsing the command line.
	 */
	sqsh_frestore();

	/*
	 * As with before, the following is required just in case stdin
	 * hit EOF while redirected from a file or some other file 
	 * descriptor.  On some systems EOF sticks with the FILE structure
	 * until clearerr() is called.
	 */
	clearerr( stdin );

#if defined(USE_AIX_FIX)
	stdout->_flag = old_flags;
#endif /* USE_AIX_FIX */

	/*
	 * Lastly, destroy the job structure that we allocated.  It almost
	 * makes you wonder why it was created at all.
	 */
 	job_free( job );

	/*
	 * Note, reguardless of the exit status of the command, we return 0
	 * (i.e. the command was called succesfully reguarldess of whether
	 * or not the job performed its task. 
	 */
 	sqsh_set_error( SQSH_E_NONE, NULL );
	return 0;

jobset_abort:
	/*
	 * This sucks.  Unfortunately I want to return the error condition
	 * returned by jobset_parse(), but the functions sqsh_frestore() and
	 * job_free() probably set the sqsh error condition, so we need to
	 * save the current error condition prior to calling them.
	 * There must be a better way of doing this.
	 */
	strcpy( msg, sqsh_get_errstr() );
	error = sqsh_get_error();
	sqsh_frestore();
	clearerr( stdin );
#if defined(USE_AIX_FIX)
	if (old_flags != (int)-1)
	{
		stdout->_flag = old_flags;
	}
#endif /* USE_AIX_FIX */
	job_free( job );
	sqsh_set_error( error, msg );
	return -1;
}

/*
 * jobset_run_sigint():
 *
 * This little stub is used by jobset_run() to capture SIGINT from a
 * user and return to the point immedately after the command was executed..
 */
static void jobset_run_sigint( sig, user_data )
	int sig;
	void *user_data;
{
	LONGJMP( sg_jmp_buf, 1 );
}

/*
 * jobset_wait():
 *
 * Waits for job given by job_id to complete execution (if job_id is
 * a negative value then jobset_wait() waits for any pending job to
 * complete).  If block_type is JOBSET_NONBLOCK and the job has not
 * completed then 0 is returned and exit_status contains an undefined
 * value, otherwise, if block_type is JOBSET_BLOCK, the job_id of
 * the completed job is returned with exit_status containing the exit
 * value of the job. If an error condition ocurres, or job_id is not a
 * valid pending job then -1 is returned.
 */
job_id_t jobset_wait( js, job_id, exit_status, block_type )
	jobset_t   *js;
	job_id_t    job_id;
	int        *exit_status;
	int         block_type;
{
	job_t      *j;
	int         wait_type;
	pid_t       pid;

	/*-- Always check your parameters --*/
	if( js == NULL || exit_status == NULL || 
	    (block_type != JOB_BLOCK && block_type != JOB_NONBLOCK) ) {

		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return -1;
	}

	if( job_id < 0 )
		return jobset_wait_all( js, exit_status, block_type );

	/*-- Find out if the job exists --*/
	/* (sqsh-2.1.7 - Bug fix for bucket calculation) --*/
	for( j = js->js_jobs[job_id % js->js_hsize];
		  j != NULL && j->job_id != job_id ; j = j->job_nxt );

	/*-- If we can't, error --*/
	if( j == NULL ) {
		sqsh_set_error( SQSH_E_EXIST, NULL );
		return -1;
	}

	/*
	 * If the job has already been recorded as being complete then
	 * just return the necessary information to the caller.
	 */
	if( j->job_flags & JOB_DONE ) {
		*exit_status = j->job_status;
		return j->job_id;
	}

	/*
	 * The type of sigcld wait that will be performed depends on 
	 * what kind of blocking is requested by the caller.
	 */
	wait_type = (block_type == JOB_BLOCK) ? SIGCLD_BLOCK : SIGCLD_NONBLOCK;

	/*
	 * Go ahead and wait for the completion of the job.
	 */
	pid = sigcld_wait( js->js_sigcld, j->job_pid, exit_status, wait_type );

	/*
	 * If block_type was JOB_NONBLOCK and there were no jobs pending,
	 * then return 0.
	 */
	if( pid == 0 ) {
		sqsh_set_error( SQSH_E_NONE, NULL );
		return 0;
	}

	/*
	 * Propagate any errors back to the caller.
	 * sqsh-2.1.7 - If we missed a SIGCLD signal and the pid is already
	 * finished, then just mark the job as completed and continue
	 * as normal. Then you can use \show to check the deferred output
	 * so far and get the job out of the queue.
	 */
	if( pid == -1 ) {
		sqsh_set_error( sqsh_get_error(), "sigcld_wait: %s",
				sqsh_get_errstr() );
	}

	/*
	 * If we have reached this point, then the job has completed so
	 * it is just a matter of recording the exit status and returning.
	 */
 	j->job_flags  |= JOB_DONE;
	j->job_end     = time(NULL);
 	j->job_status  = *exit_status;

 	sqsh_set_error( SQSH_E_NONE, NULL );
 	return j->job_id;
}

int jobset_end( js, job_id )
	jobset_t   *js;
	job_id_t    job_id;
{
	job_t      *j, *j_prv;
	int         hval;
	int         exit_status;

	/*-- Always check your parameters --*/
	if( js == NULL || job_id <= 0 ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	/*-- Calculate which bucket job_id is in --*/
	hval = job_id % js->js_hsize;

	/*-- Search for the job --*/
	j_prv = NULL;
	for( j = js->js_jobs[hval]; j != NULL && j->job_id != job_id;
	     j = j->job_nxt )
  		j_prv = j;

	if( j == NULL ) {
		sqsh_set_error( SQSH_E_EXIST, "Invalid job_id %d", job_id );
		return False;
	}

	/*
	 * If the job hasn't completed yet, then we need to kill the process
	 * associated with the job and wait for it to complete.
	 */
	if( !(j->job_flags & JOB_DONE) ) {

		/*-- Kill the job --*/
		if( kill( j->job_pid, SIGTERM ) == -1 ) {
			sqsh_set_error( errno, "Unable to SIGTERM pid %d: %s", j->job_pid,
			                strerror( errno ) );
		 	return False;
		}

		/*-- Wait for it to die. --*/
		if( sigcld_wait( js->js_sigcld, j->job_pid, &exit_status,
		                 SIGCLD_BLOCK ) == -1 ) {
	 		sqsh_set_error( sqsh_get_error(), "sigcld_wait: %s",
	 		                sqsh_get_errstr() );
		 	return False;
		}
	}

	/*
	 * Finally unlink it from the list of jobs that are running.
	 */
 	if( j_prv != NULL )
 		j_prv->job_nxt = j->job_nxt;
	else
		js->js_jobs[hval] = j->job_nxt;

 	job_free( j );
 	sqsh_set_error( SQSH_E_NONE, NULL );
 	return True;
}


/*
 * jobset_clear():
 *
 * Clears out all references to currently running jobs without 
 * actually terminating the jobs.
 */
int jobset_clear( js )
	jobset_t    *js;
{
	job_t     *j, *j_nxt;
	int        i;

	/*-- Check your parameters --*/
	if( js == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	/*-- Blast through every bucket in hash table --*/
	for( i = 0; i < js->js_hsize; i++ ) {

		/*-- Blast through every job in bucket --*/
		j = js->js_jobs[i];
		while( j != NULL ) {

			/*-- Keep pointer to next job in bucket --*/
			j_nxt = j->job_nxt;

			/*-- We no longer care when this job terminates --*/
			sigcld_unwatch( js->js_sigcld, j->job_pid );

			/*-- Destroy the job --*/
			job_free( j );

			/*-- Move on --*/
			j = j_nxt;
		}
	}

	sqsh_set_error( SQSH_E_NONE, NULL );
	return True;
}

int jobset_is_done( js, job_id )
	jobset_t   *js;
	job_id_t    job_id;
{
	job_t  *j;

	if( (j = jobset_get( js, job_id )) == NULL )
		return -1;

	sqsh_set_error( SQSH_E_NONE, NULL );

	if( j->job_flags & JOB_DONE )
		return 1;
	return 0;
}

char* jobset_get_defer( js, job_id )
	jobset_t   *js;
	job_id_t    job_id;
{
	job_t  *j;

	if( (j = jobset_get( js, job_id )) == NULL )
		return NULL;

	sqsh_set_error( SQSH_E_NONE, NULL );
	return j->job_output;
}

pid_t jobset_get_pid( js, job_id )
	jobset_t   *js;
	job_id_t    job_id;
{
	job_t  *j;

	if( (j = jobset_get( js, job_id )) == NULL )
		return -1;

	sqsh_set_error( SQSH_E_NONE, NULL );
	return j->job_pid;
}

int jobset_get_status( js, job_id )
	jobset_t   *js;
	job_id_t    job_id;
{
	job_t  *j;

	if( (j = jobset_get( js, job_id )) == NULL )
		return -1;

	sqsh_set_error( SQSH_E_NONE, NULL );
	return j->job_status;
}

/*
 * jobset_destroy():
 *
 * Destroys a jobset structure, sending a SIGTERM signal to all
 * child jobs within it that have not yet completed.  Compare and
 * contrast to jobset_clear().  True is returned upon success, False
 * upon failure.
 */
int jobset_destroy( js )
	jobset_t     *js;
{
	int     i;
	job_t  *j, *j_nxt;
	int     exit_status;

	/*-- Stupid programmers --*/
	if( js == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	for( i = 0; i < js->js_hsize; i++ ) {
		j = js->js_jobs[i];
		while( j != NULL ) {
			j_nxt = j->job_nxt;

			/*
			 * If the job hasn't completed yet, then we need to kill the process
			 * associated with the job and wait for it to complete.
			 */
			if( !(j->job_flags & JOB_DONE) ) {

				/*
				 * Well, I should probably be paying attention to the return
				 * values of the next two calls, but if they fail I really
				 * don't know what to do about it anyway.  Either way the
				 * child process is going down.
				 */
				kill( j->job_pid, SIGTERM );
				sigcld_wait( js->js_sigcld, j->job_pid, &exit_status,
				             SIGCLD_BLOCK );
			}

			job_free( j );
			j = j_nxt;
		}
	}

	free( js->js_jobs );

	/*-- Destroy sigcld context structure --*/
	sigcld_destroy( js->js_sigcld );

	/*-- Finally, free the jobset --*/
	free( js );

	sqsh_set_error( SQSH_E_NONE, NULL );
	return True;
}

/*****************************************************************
 ** INTERNAL FUNTIONS
 *****************************************************************/

/*
 * jobset_wait_all():
 */
static job_id_t jobset_wait_all( js, exit_status, block_type )
	jobset_t   *js;
	int        *exit_status;
	int         block_type;
{
	job_t      *j;
	pid_t       pid;
	int         wait_type;
	int         i;

	/*
	 * The type of sigcld wait that will be performed depends on 
	 * what kind of blocking is requested by the caller.
	 */
	wait_type = (block_type == JOB_BLOCK) ? SIGCLD_BLOCK : SIGCLD_NONBLOCK;

	/*
	 * Go ahead and wait for the completion of the job.
	 */
	pid = sigcld_wait( js->js_sigcld, -1, exit_status, wait_type );

	/*
	 * Propagate any errors back to the caller.
	 */
	if( pid == -1 ) {
		sqsh_set_error( sqsh_get_error(), "sigcld_wait: %s", sqsh_get_errstr() );
		return -1;
	}

	/*
	 * If block_type was JOB_NONBLOCK and there were no jobs pending,
	 * then return 0.
	 */
	if( pid == 0 ) {
 		sqsh_set_error( SQSH_E_NONE, NULL );
		return 0;
	}

	/*
	 * The following two loops rather inneficiently looks up the
	 * job_t structure that represents the completed pid.
	 */
	for( i = 0; i < js->js_hsize; i++ ) {

		for( j = js->js_jobs[i]; j != NULL && j->job_pid != pid;
		     j = j->job_nxt );

	  	if( j != NULL ) {

			/*
			 * If we have reached this point, then the job has completed so
			 * it is just a matter of recording the exit status and returning.
			 */
			j->job_flags  |= JOB_DONE;
			j->job_end     = time(NULL);
			j->job_status  = *exit_status;

			sqsh_set_error( SQSH_E_NONE, NULL );
			return j->job_id;
		}
	}

	/*
	 * If we get here then we have trouble...we have received a sigcld
	 * for a pid that we don't have registered as belonging to a job.
	 * Hmmm...what to do?
	 */
 	sqsh_set_error( SQSH_E_EXIST, "Received SIGCHLD for unknown pid!" );
 	return -1;
}

/*
 * jobset_parse():
 *
 * This giant beastie of a function is responsible for parsing the
 * command line for a job function.  While it proceeds it redirects
 * any file descriptors as requested by the command line.  Upon success
 * various fields of job will be set indicating option and arguments
 * on the command line and true is returned.  Upon failure the status
 * of all file descriptors and the contents of the job structure
 * is undefined, False is returned.
 */
static int jobset_parse( js, job, cmd_line, while_buf, tok_flags )
	jobset_t    *js;
	job_t       *job;
	char        *cmd_line;
	varbuf_t    *while_buf;
	int          tok_flags;
{
	tok_t             *tok ;               /* Token read with sqsh_tok() */
	int                flag ;              /* Flag to be passed to open() */
	char              *defer_bg ;          /* Results from env_get() */
	char              *tmp_dir ;           /* Results from env_get() */
	char               defer_path[SQSH_MAXPATH+1];
	int                defer_fd;
	int                fd;
	char              *bg_ptr   = NULL ; /* Location of & */
	char              *pipe_ptr = NULL ; /* Location of | */
	char              *cp;
	/* sqsh-2.1.6 - New variables */
	varbuf_t	  *exp_buf;


	/*
	 * In a "real" shell both the | pipeline(s) and the & background
	 * characters take presedence during command line parsing.  In
	 * order to simulate this we first must determine if we have either
	 * of these characters in our command line.
	 */
	pipe_ptr = sqsh_strchr( cmd_line, '|' );

	/*
	 * Look for a & that isn't preceded by > (which would be an ocurrance
	 * of the &> token).
	 */
	cp = cmd_line;
	while( (bg_ptr = sqsh_strchr( cp, '&' )) != NULL &&
	       bg_ptr != cmd_line && *(bg_ptr - 1) == '>' )
		cp = bg_ptr + 1;
	
	/*
	 * Now, verify that the background character is the last character
	 * on the line.
	 */
	if( bg_ptr != NULL ) {
		/*-- Skip trailing white space --*/
		for( cp = bg_ptr + 1; *cp != '\0' && isspace((int)*cp); ++cp );

		if( *cp != '\0' ) {
			sqsh_set_error( SQSH_E_SYNTAX, "& must be last token on line" );
			return False;
		}
	}

	/* 
	 * All-righty then.  Now, the first thing we need to do is, if we
	 * are to be run in the background the create the necessary defer
	 * file and attach it to our stdout and stderr.  If, later during
	 * the parsing the user redirects one of these streams it won't
	 * really hurt anything.
	 */
	if( bg_ptr != NULL ) {

		/*-- Mark this as a background job --*/
		job->job_flags |= JOB_BG;

		env_get( g_env, "defer_bg", &defer_bg );

		if( defer_bg != NULL && *defer_bg != '0' ) {

			/*-- Mark this job as having deferred data --*/
			job->job_flags |= JOB_DEFER;

			/*-- Check to see if the user wants to override --*/
			env_get( g_env, "tmp_dir", &tmp_dir );

			if( tmp_dir == NULL || *tmp_dir == '\0' )
				tmp_dir = SQSH_TMP;
			else /* sqsh-2.1.6 feature - Expand tmp_dir variable */
		       	{
				if ((exp_buf = varbuf_create( 512 )) == NULL)
				{
					fprintf( stderr, "sqsh: %s\n", sqsh_get_errstr() );
					sqsh_exit( 255 );
				}
				if (sqsh_expand( tmp_dir, exp_buf, 0 ) == False)
					tmp_dir = SQSH_TMP;
				else
					tmp_dir = varbuf_getstr( exp_buf );
			}

			/*-- Create the defer file --*/
			sprintf( defer_path, "%s/sqsh-dfr.%d-%d", 
			         tmp_dir, (int) getpid(), job->job_id );

			varbuf_destroy( exp_buf ); /* sqsh-2.1.6 feature */
			
			/*-- Let the job structure know where it is --*/
			if( (job->job_output = sqsh_strdup( defer_path )) == NULL ) {
				sqsh_set_error( SQSH_E_NOMEM, NULL );
				return False;
			}

			/*-- Now, open the file --*/
			if( (defer_fd = sqsh_open(defer_path,O_CREAT|O_WRONLY|O_TRUNC,0600)) == -1 ) {
				sqsh_set_error( sqsh_get_error(), "Unable to open %s: %s", 
				                defer_path, sqsh_get_errstr() );
				return False;
			}

			/*
			 * Now, simply attach the new file descriptor onto
			 * stdout and stderr.
			 */
			if( sqsh_dup2( defer_fd, fileno(stdout) ) == -1 ||
			    sqsh_dup2( defer_fd, fileno(stderr) ) == -1 ) {
				sqsh_set_error( sqsh_get_error(), "sqsh_dup2: %s",
									 sqsh_get_errstr() );
				sqsh_close( defer_fd );
				return False;
			}

			/*-- No longer needed --*/
			sqsh_close( defer_fd );
		}
	}

	/*
	 * Ok, now we need to deal with the pipe-line.  If there is one
	 * to be had, we need to crank the sucker up and attach it to
	 * our stdout.  If this is a background process, the pipeline
	 * will inherit our deferred stdout and stderr.
	 */
	if( pipe_ptr != NULL ) {

		/*
		 * Temporarily, if the bg_ptr is pointing to the trailing
		 * '&' character then turn it to a '\0'.  We'll restore it
		 * later.  General note:  Because we know that cmd_line is
		 * actually a varbuf_t (behaps a rash assumption), we also
		 * know that it is alterable.
		 */
		if( bg_ptr != NULL )
			*bg_ptr = '\0';
		
		/*-- Mark this as having a pipe-line --*/
		job->job_flags |= JOB_PIPE;

		/*
		 * Check to see if this is an empty pipe-line.  That is,
		 * if anything actually follow the | character.
		 */
		for( cp = pipe_ptr + 1; *cp != '\0' && isspace((int)*cp); ++cp );
		if( *cp == '\0' ) {
			sqsh_set_error( SQSH_E_SYNTAX, "Nothing following |" );
			return False;
		}
		
		/*
		 * Now, go ahead and crank up the process that we want to
		 * communicate with.
		 */
		if( (fd = sqsh_popen( pipe_ptr+1, "w", NULL, NULL )) == -1 )
		{
			env_set( g_internal_env, "?", "-255" );
			sqsh_set_error(sqsh_get_error(), "sqsh_popen: %s",
			               sqsh_get_errstr());
			return False;
		}

		/*
		 * Now attempt to attach the file descriptor that we just
		 * created to our stdout.
		 */
		if( sqsh_dup2( fd, fileno(stdout) ) == -1 ) {
			sqsh_set_error( sqsh_get_error(), "sqsh_dup2: %s",
								 sqsh_get_errstr() );
			sqsh_close( fd );
			return False;
		}

		/*-- Don't need this any more --*/
		sqsh_close( fd );

		/*
		 * If necessary, restore the background character to where
		 * we found it.  This is supposed to be a non-destructive
		 * function call.
		 */
		if( bg_ptr != NULL )
			*bg_ptr = '&';
	}

	/*
	 * Retrieve the first token from the cmd_line.  This really shouldn't
	 * fail, since we have already done it in jobset_run(), above.
	 */
	if( sqsh_tok( cmd_line, &tok, tok_flags ) == False ) {
		sqsh_set_error( sqsh_get_error(), "sqsh_tok: %s", sqsh_get_errstr() );
		return False;
	}

	/*
	 * We are going to continue looking at tokens until we reach the
	 * end of the command line.  It should be noted that according to
	 * the logic for sqsh_tok(), the SQSH_T_EOF is considered either
	 * when we actually reach the end of the line or we hit a |
	 * or the & (background) character.
	 */
	while( tok->tok_type != SQSH_T_EOF ) {
		
		switch( tok->tok_type ) {

			case SQSH_T_REDIR_OUT :      /* [n]> file, or [n]>> file */

				/*
				 * Attempt to retrieve the name of the file to redirect
				 * the file descriptor to.
				 */
				if( sqsh_tok( NULL, &tok, tok_flags ) == False ) {
					sqsh_set_error( sqsh_get_error(), "sqsh_tok: %s",
					                sqsh_get_errstr() );
					return False;
				}

				/*
				 * If the next token we retrieve isn't the name of a file, then
				 * we have a syntax error.
				 */
				if( tok->tok_type != SQSH_T_WORD ) {
					sqsh_set_error( SQSH_E_SYNTAX, "Expected file following >" );
					return False;
				}

				/*
				 * Figure out which flags we need to supply to open, if the
				 * file is to be appended to, or created/overwritten.
				 */
				if( tok->tok_append )
					flag = O_WRONLY | O_CREAT | O_APPEND;
				else
					flag = O_WRONLY | O_CREAT | O_TRUNC;

				/*-- Now, open the file --*/
				if( (fd = sqsh_open( sqsh_tok_value(tok), flag, 0 )) == -1 ) {
					sqsh_set_error( sqsh_get_error(), "Unable to open %s: %s",
										 sqsh_tok_value(tok), sqsh_get_errstr() );
					return False;
				}

				/*
				 * Now, redirect the file descriptor requested by the user
				 * to the newly opened file and close the file.
				 */
				if( sqsh_dup2( fd, tok->tok_fd_left ) == -1 ) {
					sqsh_set_error( sqsh_get_error(), "sqsh_dup2: %s",
										 sqsh_get_errstr() );
					return False;
				}
				sqsh_close( fd );

				break;

			case SQSH_T_REDIR_IN :  /* '< filename' */
				/*
				 * Attempt to retrieve the name of the file to redirect
				 * in from.
				 */
				if( sqsh_tok( NULL, &tok, tok_flags ) == False ) {
					sqsh_set_error( sqsh_get_error(), "sqsh_tok: %s",
					                sqsh_get_errstr() );
					return False;
				}

				/*
				 * If the next token we retrieve isn't the name of a file, then
				 * we have a syntax error.
				 */
				if( tok->tok_type != SQSH_T_WORD ) {
					sqsh_set_error( SQSH_E_SYNTAX, "Expected file following <" );
					return False;
				}

				/*-- Now, open the file --*/
				if( (fd = sqsh_open( sqsh_tok_value(tok), O_RDONLY, 0 )) == -1 ) {
					sqsh_set_error( sqsh_get_error(), "%s: %s", sqsh_tok_value(tok),
										 sqsh_get_errstr() );
					return False;
				}

				if( (sqsh_dup2( fd, fileno(stdin) )) == -1 ) {
					sqsh_set_error( sqsh_get_error(), "%s: %s", sqsh_tok_value(tok),
										 sqsh_get_errstr() );
					return False;
				}
				sqsh_close( fd );
				break;

			case SQSH_T_REDIR_DUP :   /* '[m]>&[n]' */
				/*
				 * This one is easy.  Simply call sqsh_dup2() to do the
				 * dirty work.
				 */
				if( sqsh_dup2( tok->tok_fd_right, tok->tok_fd_left ) == -1 ) {
					sqsh_set_error( sqsh_get_error(), "sqsh_dup2: %s",
										 sqsh_get_errstr() );
					return False;
				}

				break;

			case SQSH_T_WORD :
				/*
				 * If the token is a generic word on the command line then
				 * we add it to the list of arguments to be passed to the
				 * function.  For a \while loop, we'll just tack it on to
				 * the single parameter.
				 */
				if (while_buf != NULL)
				{
					/*
					 * If this is the first item on the list, then we
					 * don't need to stick a space between the previous
					 * one.
					 */
					if (varbuf_getlen(while_buf) > 0)
					{
						varbuf_charcat( while_buf, ' ' );
					}
					varbuf_strcat( while_buf, sqsh_tok_value(tok) );
				}
				else
				{
					if (args_add(job->job_args, sqsh_tok_value( tok )) == False)
					{
						sqsh_set_error( sqsh_get_error(), "args_add: %s",
											 sqsh_get_errstr() );
						return False;
					}
				}
				break;

			default :
				sqsh_set_error( SQSH_E_SYNTAX, "Unknown token %d", tok->tok_type );
				return False;
		}  /* switch */

		/*
		 * Retrieve the next token from the command line.
		 */
		if( sqsh_tok( NULL, &tok, tok_flags ) == False ) {
			sqsh_set_error( sqsh_get_error(), "sqsh_tok: %s", sqsh_get_errstr() );
			return False;
		}

	} /* while */

	sqsh_set_error( SQSH_E_NONE, NULL );
	return True;
}

static job_t* jobset_get( js, job_id )
	jobset_t   *js;
	job_id_t    job_id;
{
	job_t      *j;
	int         hval;

	/*-- Always check your parameters --*/
	if( js == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return NULL;
	}

	/*-- Calculate which bucket job_id is in --*/
	hval = job_id % js->js_hsize;

	/*-- Search for the job --*/
	for( j = js->js_jobs[job_id % js->js_hsize];
	     j != NULL && j->job_id != job_id; j = j->job_nxt );

	/*-- ESTUPID --*/
  	if( j == NULL ) {
  		sqsh_set_error( SQSH_E_EXIST, NULL );
  		return NULL;
	}

	return j;
}


/*************************************************************/

/*
 * job_alloc():
 *
 * Internal function to allocate and initialize a new job within
 * a job set.  Returns a valid pointer on success and NULL upon
 * a memory allocation failure.
 */
static job_t* job_alloc( js )
	jobset_t  *js;
{
	job_id_t  max_jobid = 0;
	job_t     *new_job, *job;
	int       i;

	/*-- Create the command structure --*/
	if( (new_job = (job_t*)malloc(sizeof( job_t ))) == NULL )
		return NULL;

	/*-- Rather inneficiently calculate the next available job_id --*/
	for( i = 0; i < js->js_hsize; i++ ) {
		for( job = js->js_jobs[i]; job != NULL; job = job->job_nxt )
			max_jobid = max(max_jobid, job->job_id);
	}

	/*-- Create space to hold command-line arguments --*/
	if( (new_job->job_args = args_create( 16 )) == NULL ) {
		free( new_job );
		return NULL;
	}

	/*-- Initialize all fields --*/
	new_job->job_id     = max_jobid + 1;
	new_job->job_flags  = 0;
	new_job->job_start  = time(NULL);
	new_job->job_end    = 0;
	new_job->job_status = 0;
	new_job->job_pid    = 0;
	new_job->job_output = NULL;
	new_job->job_nxt    = NULL;

	return new_job;
}

/*
 * job_free():
 *
 * If command given by job_id exists within the jobset, js, then
 * it is removed and all space allocated to it is reclaimed.  True
 * is returned upon success, False is returned if job_id does not
 * exist within js.
 */
static int job_free( j )
	job_t     *j;
{
	if( j == NULL )
		return False;

	/*
	 * I don't know if this should go here, but.. if the job may
	 * have deferred output and we have the name of the defer
	 * file, then attempt to unlink the file.  Ignore the
	 * results of unlink.
	 */
	if( (j->job_flags & JOB_DEFER) && j->job_output != NULL )
		unlink( j->job_output );

	if( j->job_args != NULL )
		args_destroy( j->job_args );
	if( j->job_output != NULL )
		free( j->job_output );
	free( j );

	return True;
}

/*
 * jobset_global_init():
 *
 * Initialize any global variables that may be required by the various
 * jobset routines..
 */
static int jobset_global_init()
{
	static int init_done = False;

	if( init_done == False ) {
		/*
		 * We need a buffer in which to place the expanded command line
		 * perior to parsing.
		 */
		if( sg_cmd_buf == NULL ) {
			if( (sg_cmd_buf = varbuf_create( 128 )) == NULL ) {
				sqsh_set_error( sqsh_get_error(),"varbuf: %s",sqsh_get_errstr() );
				return False;
			}
		}

		if( sg_alias_buf == NULL ) {
			if( (sg_alias_buf = varbuf_create( 128 )) == NULL ) {
				sqsh_set_error( sqsh_get_error(),"varbuf: %s",sqsh_get_errstr() );
				return False;
			}
		}

		if (sg_while_buf == NULL)
		{
			if ((sg_while_buf = varbuf_create( 128 )) == NULL)
			{
				sqsh_set_error( sqsh_get_error(),"varbuf: %s",sqsh_get_errstr() );
				return False;
			}
		}

		init_done = True;
	}
	return True;
}
