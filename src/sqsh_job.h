/*
 * sqsh_job.h - Functions for launching commands
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
#ifndef sqsh_job_h_included
#define sqsh_job_h_included
#include "sqsh_args.h"
#include "sqsh_sigcld.h"

#define JOBSET_HSIZE     47

/*
 * A job_t represents a handle (or command id) on a particular instance
 * of a running command, this is the only real interface into a jobset_t,
 * which is an opaque member of a ctxt_t.
 */
typedef int job_id_t ;

/*
 * The following flags are used for two purposes: They are used within
 * job->job_flags to represent info/status for the command, and some
 * of them are valid flags for job_run() to specify certain properties
 * for a command.
 */
#define JOB_BG           (1<<0)  /* Command is running in background */  
#define JOB_CHLD         (1<<1)  /* Background command is in child process */
#define JOB_DONE         (1<<2)  /* Command has completed execution */
#define JOB_DEFER        (1<<3)  /* Job's output has been defered to file */
#define JOB_PIPE         (1<<4)  /* Command's stdout is to a pipeline */

/*
 * Possible methods for waiting for execution of a background job
 * to complete.
 */
#define JOB_BLOCK        1
#define JOB_NONBLOCK     2

/*
 * This structure contains the internal representation for a job_t,
 * including the varbuf_t on which the command is to act, and the
 * set of arguments supplied to the command.
 */
typedef struct job_st {
	job_id_t  job_id ;          /* This is the unique identifier for the job */
	int       job_flags ;       /* Set of flags, above */
	args_t   *job_args ;        /* Arguments to command */
	time_t    job_start ;       /* Time at which job began */
	time_t    job_end ;         /* Time at which job completed */
	char     *job_output ;      /* If job_flags & JOB_DEFER, then this will */
	                            /* contain the name of the file holding the */
	                            /* deferred output */
	int       job_status ;      /* If job_flags & JOB_DONE, then this */
	                            /* will contain the exit status of the child */
	                            /* process */
	pid_t     job_pid ;         /* If job_flags & JOB_BG then this will */
										 /* contain the Unix process id of child */
	struct job_st *job_nxt ;    /* Pointers for the hash table */
} job_t ;

typedef struct jobset_st {
   int         js_hsize ;      /* Size of the command hash table */
	job_t     **js_jobs ;       /* Hash table of commands */
	sigcld_t   *js_sigcld ;     /* Handle on sigcld events */
} jobset_t ;

/*-- Public Prototypes --*/
jobset_t*  jobset_create     _ANSI_ARGS(( int )) ;
job_id_t   jobset_run        _ANSI_ARGS(( jobset_t*, char*, int* )) ;
int        jobset_is_cmd     _ANSI_ARGS(( jobset_t*, char* )) ;
job_id_t   jobset_wait       _ANSI_ARGS(( jobset_t*, job_id_t, int*, int )) ;
int        jobset_end        _ANSI_ARGS(( jobset_t*, job_id_t )) ;
int        jobset_clear      _ANSI_ARGS(( jobset_t* )) ;
int        jobset_get_status _ANSI_ARGS(( jobset_t*, job_id_t )) ;
char*      jobset_get_defer  _ANSI_ARGS(( jobset_t*, job_id_t )) ;
pid_t      jobset_get_pid    _ANSI_ARGS(( jobset_t*, job_id_t )) ;
int        jobset_is_done    _ANSI_ARGS(( jobset_t*, job_id_t )) ;
int        jobset_destroy    _ANSI_ARGS(( jobset_t* )) ;

#endif /* sqsh_job_included */
