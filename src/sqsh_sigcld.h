/*
 * sqsh_sigcld.h - Queueing and retrieving of SIGCHLD events
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
#ifndef sqsh_sigcld_h_included
#define sqsh_sigcld_h_included

/*
 * A single instance of the following data structure may be referenced
 * by more than one list of pending signals.
 */
typedef struct sigpid_st {
	pid_t         sp_pid ;      /* Process we are watching */
	int           sp_signal ;   /* True/False got signal */
	int           sp_status ;   /* If sp_signal True, this is exit status */
} sigpid_t ;

/*
 * The following data structure is used both internally and externally
 * to link a single instance of a sigpid_t into muliple lists.
 */
typedef struct signode_st {
	sigpid_t          *sn_node ; /* The sigpid_t to be stuck in list */
	int                sn_ref ;  /* Number of references to this pid */
	struct signode_st *sn_nxt ;  /* The link in the list */
} signode_t ;

/*
 * A sigcld_t is the external caller's reference to his/her set of pid's
 * of interest.  It merely contains pointers to all of the sigpid_t's
 * in the master list of watched pid's.
 */
typedef struct sigcld_st {
	signode_t        *s_pids ;  /* List of pid's */
} sigcld_t ;

/*
 * Types of blocking that may be performed.
 */
#define SIGCLD_BLOCK             0
#define SIGCLD_NONBLOCK          1

/*-- Prototypes --*/
sigcld_t*  sigcld_create  _ANSI_ARGS(( void )) ;
int        sigcld_block   _ANSI_ARGS(( void )) ;
int        sigcld_unblock _ANSI_ARGS(( void )) ;
int        sigcld_watch   _ANSI_ARGS(( sigcld_t*, pid_t )) ;
int        sigcld_unwatch _ANSI_ARGS(( sigcld_t*, pid_t )) ;
pid_t      sigcld_wait    _ANSI_ARGS(( sigcld_t*, pid_t, int*, int )) ;
int        sigcld_destroy _ANSI_ARGS(( sigcld_t* )) ;

#endif /* sqsh_sigcld_h_included */
