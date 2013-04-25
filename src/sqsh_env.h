#ifndef sqsh_env_h_included
#define sqsh_env_h_included

/*
 * Stub definition.
 */
struct env_st ;

/*
 * The following datatype represents an environment set/retrieve function.
 * These functions may be used to verify or alter environment variables
 * as they are being retrieved or set.
 */
typedef int (env_f) _ANSI_ARGS(( struct env_st*, char*, char** )) ;

/*
 * A representation for a single environment variable.  This contains
 * the name, value and any validation function pointers that may be
 * used when setting or retrieving the variable.
 */
typedef struct var_st {
	int       var_sptype;    /* Savepoint record type */
	char     *var_name ;     /* Name of the variable */
	char     *var_value ;    /* Value of the variable */
	env_f    *var_setf ;     /* Function to be called to when setting var */
	env_f    *var_getf ;     /* Function to be called when getting variable */
	struct var_st *var_nxt ; /* Keep 'em in a list */
} var_t ;

/*
 * The following are possible values for var->var_sptype when
 * a var_t is linked into the savepoint stack. 
 */
#define ENV_SP_NONE       0 /* Not a save-point record */
#define ENV_SP_START      1 /* Beginning of savepoint */
#define ENV_SP_NEW        2 /* New variable added to environment */
#define ENV_SP_CHANGE     3 /* Existing value changed */
#define ENV_SP_REMOVE     4 /* Variable removed from environment */

/*
 * The following flags may be passed to env_put() to affect
 * its behavior.
 */
#define ENV_F_TRAN   (1<<0) /* Keep change in transaction */

/*
 * This structure is used within a ctxt_t to represent a handle on a
 * collection of environment variables.
 */
typedef struct env_st {
	int       env_hsize;     /* Size of the environment hash table */
	var_t   **env_htable;    /* Hash table to contain all variables */
	var_t    *env_save;      /* Save stack to restore previous state */
} env_t ;

/*-- Prototypes --*/
env_t* env_create    _ANSI_ARGS(( int ));
int    env_set_valid _ANSI_ARGS(( env_t*, char*, char*, env_f*, env_f* ));
int    env_set       _ANSI_ARGS(( env_t*, char*, char* ));
int    env_put       _ANSI_ARGS(( env_t*, char*, char*, int ));
int    env_remove    _ANSI_ARGS(( env_t*, char*, int ));
int    env_nget      _ANSI_ARGS(( env_t*, char*, char**, int ));
int    env_tran      _ANSI_ARGS(( env_t* ));
int    env_commit    _ANSI_ARGS(( env_t* ));
int    env_rollback  _ANSI_ARGS(( env_t* ));
int    env_destroy   _ANSI_ARGS(( env_t* ));
/* int env_get       _ANSI_ARGS(( env_t*, char*, char** )); */

#define env_get(e,k,v)  env_nget(e,k,v,-1)

#endif /* sqsh_env_h_included */
