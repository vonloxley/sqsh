/*
 * sqsh_env.c - Manipulate "smart" environment variables
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
#include "sqsh_config.h"
#include "sqsh_error.h"
#include "sqsh_env.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_env.c,v 1.1.1.1 2001/10/23 20:31:06 gray Exp $";
USE(RCS_Id)
#endif /* !defined(lint) */

/*-- Prototypes --*/
static int    env_hval       _ANSI_ARGS(( env_t*, char* ));
static int    env_nhval      _ANSI_ARGS(( env_t*, char*, int ));
static var_t* var_create     _ANSI_ARGS(( char*, char* ));
static int    var_new_value  _ANSI_ARGS(( var_t*, char* ));
static void   var_destroy    _ANSI_ARGS(( var_t* ));

/*
 * env_create():
 *
 * Allocates an environment structure within which you may then
 * store variable name/value pairs.  Returns a new env upon success,
 * NULL upon failure.
 */
env_t* env_create( hsize )
	int     hsize;
{
	env_t  *e;
	int     i;

	/*-- Check our parameters --*/
	if (hsize < 1)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	/*-- Attempt to allocate an environment --*/
	if ((e = (env_t*)malloc(sizeof(env_t))) == NULL)
	{
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return False;
	}

	/*-- Create the hash table --*/
	if ((e->env_htable = (var_t**)malloc(sizeof(var_t*)*hsize)) == NULL)
	{
		free( e ); 
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return False;
	}

	for (i = 0; i < hsize; i++)
	{
		e->env_htable[i] = NULL;
	}

	e->env_hsize = hsize;
	e->env_save  = NULL;

	/*-- Return success --*/
	sqsh_set_error( SQSH_E_NONE, NULL );
	return e;
}

/*
 * env_set_valid():
 *
 * If var_name hasn't been created before within e, then a new
 * variable is created, using set_func and get_func for its future
 * validation checks.  If the variable already exists, then the value
 * of the variable is replaced with value, and eh validation checks
 * are replaced.  Note, this function does not call any existing
 * validation checks.
 */
int env_set_valid( e, var_name, value, set_func, get_func )
	env_t  *e;
	char   *var_name;  /* name of variable to set */
	char   *value;     /* value to initialize variable to */
	env_f  *set_func;  /* set validation function */
	env_f  *get_func;  /* get validation function */
{
	var_t   *var;
	int     hval;

	/*-- Always check your arguments --*/
	if (e == NULL || var_name == NULL)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	/*-- Attempt to look up the variable --*/
	hval = env_hval( e, var_name );
	for (var = e->env_htable[hval]; 
	     var != NULL && strcmp(var_name, var->var_name) != 0;
	     var = var->var_nxt);

	/*
	 * If the variable already exists, then replace its existing
	 * value and validation functions with the values passed in
	 */
	if (var != NULL)
	{
		if ((var_new_value( var, value )) == False)
		{
			sqsh_set_error( SQSH_E_NOMEM, NULL );
			return False;
		}
	}
	else
	{
		if ((var = var_create( var_name, value )) == NULL)
		{
			sqsh_set_error( SQSH_E_NOMEM, NULL );
			return False;
		}

		/*-- Insert the variable into the hash table --*/
		var->var_nxt = e->env_htable[hval];
		e->env_htable[hval] = var;
	}

	var->var_setf = set_func;
	var->var_getf = get_func;

	sqsh_set_error( SQSH_E_NONE, NULL );
	return True;
}

/*
 * env_remove():
 *
 * Remove and entry from the environment.
 */
int env_remove( e, var_name, flags )
	env_t   *e;
	char    *var_name;
	int      flags;
{
	var_t   *v, *p;
	int      hval;

	/*-- Always check your arguments --*/
	if (e == NULL || var_name == NULL)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	/*-- Attempt to look up the variable --*/
	hval = env_hval( e, var_name );
	for (p = NULL, v = e->env_htable[hval]; 
	     v != NULL && strcmp(var_name, v->var_name) != 0;
	     p = v, v = v->var_nxt);
	
	if (v != NULL)
	{
		if (p != NULL)
		{
			p->var_nxt = v->var_nxt;
		}
		else
		{
			e->env_htable[hval] = v->var_nxt;
		}

		/*
		 * If we have save-point enabled, then keep this node around
		 * for future use.
		 */
		if (e->env_save != NULL && (flags & ENV_F_TRAN) != 0)
		{
			v->var_sptype = ENV_SP_REMOVE;
			v->var_nxt    = e->env_save;
			e->env_save   = v;
		}
		else
		{
			var_destroy( v );
		}

		sqsh_set_error( SQSH_E_NONE, NULL );
		return True;
	}

	sqsh_set_error( SQSH_E_EXIST, NULL );
	return False;
}

/*
 * env_set():
 *
 * If the variable given by var_name already exists within e, then its
 * value is replaced by value, following the success of any validation or
 * filtering checks that may have been created with env_set_valid(), above.
 * If the variable does not exist, then it is created with no validation
 * checks set. 
 */
int env_set( e, var_name, value )
	env_t   *e;
	char    *var_name;
	char    *value;
{
	return env_put( e, var_name, value, 0 );
}

/*
 * env_put():
 *
 * Same as env_set(), above, with the addition that flags may
 * be passed in to affect the behavior.
 */
int env_put( e, var_name, value, flags )
	env_t    *e;
	char     *var_name;
	char     *value;
	int       flags;
{
	var_t   *v;
	var_t   *new_v;
	int      hval;

	/*-- Always check your arguments --*/
	if (e == NULL || var_name == NULL)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	/*-- Attempt to look up the variable --*/
	hval = env_hval( e, var_name );
	for (v = e->env_htable[hval]; 
	     v != NULL && strcmp(var_name, v->var_name) != 0;
	     v = v->var_nxt);

	/* 
	 * If the variable already exists, then replace its existing
	 * value, otherwise, create it.
	 */
	if (v != NULL)
	{
		/*
		 * The variable already exists, so check to see if it has a set
		 * validation/filter function.  If it does then call it, it
		 * is the responsibility of the set_func to call env_set_error(),
		 * to report which error ocurred.
		 */
		if (v->var_setf != NULL )
		{
			/*
			 * Just in case the set function doesn't call sqsh_set_error()
			 * we call it ourselves with an empty error string.
			 */
			sqsh_set_error( SQSH_E_INVAL, NULL );
			if ((v->var_setf( e, var_name, &value )) == False)
			{
				return False;
			}
		}

		/*
		 * If a save-point currently exists, then save away the current
		 * value prior to replacing it.
		 */
		if (e->env_save != NULL && (flags & ENV_F_TRAN) != 0)
		{
			new_v = var_create( v->var_name, v->var_value );

			if (new_v == NULL)
			{
				return False;
			}


			new_v->var_sptype = ENV_SP_CHANGE;
			new_v->var_nxt    = e->env_save;
			e->env_save       = new_v;
		}

		/*
		 * Attempt to replace the existing value with the (possibly)
		 * validated/filtered result.
		 */
		if ((var_new_value( v, value )) == False)
		{
			sqsh_set_error( SQSH_E_NOMEM, NULL );
			return False;
		}

	}
	else
	{
		/*
		 * The variable doesn't exist, so create it and stick it
		 * into the hash table.
		 */
		if ((v = var_create( var_name, value )) == NULL)
		{
			sqsh_set_error( SQSH_E_NOMEM, NULL );
			return False;
		}

		/*-- Insert the variable into the hash table --*/
		v->var_nxt = e->env_htable[hval];
		e->env_htable[hval] = v;
	}

	sqsh_set_error( SQSH_E_NONE, NULL );
	return True;
}

/*
 * env_nget():
 *
 * Retrieves the value of variable var_name from e.  If value is a NULL
 * pointer, or points to a NULL pointer, then env_get() becomes an existence
 * check for variable var_name, returning 1 if it exists, 0 if it
 * doesn't exist or there was an error.  If value is a valid pointer, then,
 * if a 'get' validation function exists for the variable, then it is called.
 * Upon success, 1 is returned with value containing the value of var_name,
 * otherwise 0 is returned if the variable doesn't exist, or a -1
 * is retuend if the validation function failed, or some other error condition
 * ocurred.
 */
int env_nget( e, var_name, value, n )
	env_t   *e;
	char    *var_name;
	char    **value;
	int     n;
{
	var_t   *v;
	int     hval;
	char   *cptr;

	/*-- Always check your arguments --*/
	if (e == NULL || var_name == NULL)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return -1;
	}

	/*
	 * If n is a negative value then perform a regular strcmp
	 * (i.e. ignoring the length of the string).
	 */
	if (n < 0)
	{
		/*-- Attempt to look up the variable --*/
		hval = env_hval( e, var_name );

		for (v = e->env_htable[hval]; 
			  v != NULL && strcmp(var_name, v->var_name) != 0;
			  v = v->var_nxt);
	}
	else
	{
		/*-- Attempt to look up the variable --*/
		hval = env_nhval( e, var_name, n );

		for (v = e->env_htable[hval]; 
			  v != NULL && 
			  (strncmp(var_name, v->var_name, n) != 0 || v->var_name[n] != 0);
			  v = v->var_nxt);
	}

#if defined(DEBUG)
	if (v == NULL) 
	{
		sqsh_debug( DEBUG_ENV, "env_nget: Miss on %s, checking environment.\n",
		            var_name  );
	}
	else
	{
		sqsh_debug( DEBUG_ENV, "env_nget: Hit on %s.\n", var_name );
	}
#endif /* DEBUG */

	/*
	 * If they attempt to retrieve a variable that doesn't exist, then
	 * we check the real environment for the variable. Failing that
	 * we return an error.
	 */
	if (v == NULL) 
	{
		/*
		 * If n is a negative number then we know that var_name is
		 * NULL terminated, so we can go ahead and call getenv.
		 */
		if (n < 0)
		{
			*value = getenv( var_name );
		}
		else 
		{

			/*
			 * If we are only interested in part of a string, then
			 * we need to need to create a temporary buffer in which
			 * to place the partial string to pass it to getenv.
			 */
			if (n >= 0) 
			{
				if ((cptr = sqsh_strndup( var_name, n )) == NULL)
				{
					sqsh_set_error( SQSH_E_NOMEM, NULL );
					return -1;
				}
				*value = getenv( cptr );
				free( cptr );

			} 
			else 
			{
				*value = getenv( var_name );
			}
		}

		if (*value != NULL)
		{
			sqsh_set_error( SQSH_E_NONE, NULL );
			return 1;
		}

		sqsh_set_error( SQSH_E_EXIST, NULL );
		return 0;
	}

	/*
	 * If value was a NULL pointer then this function becomes an
	 * existance check for the variable, so we can return a True.
	 */
	if (value == NULL)
	{
		sqsh_set_error( SQSH_E_NONE, NULL );
		return 1;
	}

	/*
	 * Set value to contain the real, unfiltered, value of the
	 * variable.
	 */
	*value = v->var_value;

	/*
	 * If there is a validation/filter function specified for the
	 * variable, then call it.
	 */
	if (v->var_getf != NULL) 
	{
		sqsh_set_error( SQSH_E_INVAL, NULL );

		if (n >= 0)
		{
			cptr = sqsh_strndup( var_name, n );

			if (cptr == NULL)
			{
				sqsh_set_error( SQSH_E_NOMEM, NULL );
				return -1;
			}

			if ((v->var_getf( e, cptr, value )) == False) 
			{
				free( cptr );
				return -1;
			}
			free( cptr );
		}
		else
		{
			if ((v->var_getf( e, var_name, value )) == False) 
			{
				return -1;
			}
		}
	}

	sqsh_set_error( SQSH_E_NONE, NULL );
	return 1;
}

int env_print( e )
	env_t   *e;
{
	int      i;
	var_t   *v;

	if (e == NULL)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	for (i = 0; i < e->env_hsize; i++)
	{
		for (v = e->env_htable[i]; v != NULL; v = v->var_nxt)
		{
			printf("%s = %s\n",
				v->var_name != NULL  ? v->var_name  : "NULL",
				v->var_value != NULL ? v->var_value : "NULL" );
		}
	}

	sqsh_set_error( SQSH_E_NONE, NULL );
	return True;
}

/*
 * env_tran():
 *
 * Create a transaction "save-point" in the environment. After
 * calling env_tran(), subsequnt calls to env_put() or env_remove()
 * with a flag of ENV_F_TRAN will cause the resulting change to
 * be logged.  A call to env_rollback() will restore all logged
 * changes to be reversed, and a call to env_commit() ends the
 * transaction, leaving changes in place.
 */
int env_tran( e )
	env_t  *e;
{
	var_t  *v;

	/*
	 * Create an empty variable structure to be placed upon
	 * our save stack.  This structure will mark our save-
	 * point.
	 */
	v = var_create( NULL, NULL );

	if (v == NULL)
	{
		return False;
	}

	DBG(sqsh_debug( DEBUG_ENV, "env_tran: Establishing save-point\n" );)

	/*
	 * Push it onto the top of our save stack.
	 */
	v->var_sptype = ENV_SP_START;
	v->var_nxt    = e->env_save;
	e->env_save   = v;

	return True;
}

int env_rollback( e )
	env_t   *e;
{
	var_t   *v;

	/*
	 * Now, blast through our save stack, reseting each variable
	 * to its original state.
	 */
	while (e->env_save != NULL &&
			 e->env_save->var_sptype != ENV_SP_START)
	{
		v = e->env_save;
		e->env_save = e->env_save->var_nxt;

		switch (v->var_sptype)
		{
			case ENV_SP_NEW:
				DBG(sqsh_debug( DEBUG_ENV, 
					"env_rollback: Removing '%s'\n", v->var_name );)
				env_remove( e, v->var_name, 0 );
				break;
			
			case ENV_SP_CHANGE:
				DBG(sqsh_debug( DEBUG_ENV, 
					"env_rollback: Restoring '%s' to '%s'\n", 
						v->var_name, v->var_value );)
				env_put( e, v->var_name, v->var_value, 0 );
				break;
			
			case ENV_SP_REMOVE:
				DBG(sqsh_debug( DEBUG_ENV, 
					"env_rollback: Adding '%s'\n", v->var_name );)

				env_set_valid( e, v->var_name, v->var_value, 
				               v->var_setf, v->var_getf );
				env_put( e, v->var_name, v->var_value, 0 );
				break;
			
			default:
				break;
		}

		var_destroy( v );
	}

	if (e->env_save != NULL)
	{
		v = e->env_save;
		e->env_save = e->env_save->var_nxt;
		var_destroy( v );
	}
	DBG(sqsh_debug( DEBUG_ENV, "env_save: Save-point restored\n" );)

	return True;
}

int env_commit( e )
	env_t   *e;
{
	var_t   *v;

	while (e->env_save != NULL &&
			 e->env_save->var_sptype != ENV_SP_START)
	{
		v = e->env_save;
		e->env_save = v->var_nxt;
		var_destroy( v );
	}

	if (e->env_save != NULL)
	{
		v = e->env_save;
		e->env_save = v->var_nxt;
		var_destroy( v );
	}
	else
	{
		e->env_save = NULL;
	}
	DBG(sqsh_debug( DEBUG_ENV, "env_commit: Save-point restored\n" );)

	return True;
}

/*
 * env_destroy():
 *
 * Destroys an environment structure.
 */
int env_destroy( e )
	env_t  *e;
{
	int    i;
	var_t *v, *v_nxt;

	if (e == NULL)
	{
		sqsh_set_error( SQSH_E_BADPARAM, NULL );
		return False;
	}

	if (e->env_htable != NULL)
	{
		for (i = 0; i < e->env_hsize; i++) 
		{
			v = e->env_htable[i];

			while (v != NULL) 
			{
				v_nxt = v->var_nxt;
				var_destroy( v );
				v = v_nxt;
			}
		}

		free( e->env_htable );
	}

	if (e->env_save != NULL)
	{
		v = e->env_save;
		while (v != NULL)
		{
			v_nxt = v->var_nxt;
			var_destroy( v );
			v = v_nxt;
		}
	}

	free( e );

	sqsh_set_error( SQSH_E_NONE, NULL );
	return True;
}

/*
 * env_hval():
 *
 * Calculates a bucket hash value for key within hash table e.  This
 * algorithm (and the prime numbers involved) were swiped from
 * dynamic-hash.c, which swiped it from "CACM April 1988 pp 446-457,
 * by Per-Ake Larson, coded by ejp@ausmelb.oz.
 */
static int env_hval( e, key )
	env_t   *e;
	char    *key;
{
	unsigned  char *k    = (unsigned char*) key;
	unsigned  long  hval = 0;

	while (*k)
	{
		hval = hval * 37 ^ (*k++ - ' ');
	}
	return (hval % 1048583) % e->env_hsize;
}

/*
 * env_nhval():
 *
 * Same as above, but key isn't assumed to be NULL terminated.
 */
static int env_nhval( e, key, n )
	env_t   *e;
	char    *key;
	int      n;
{
	unsigned  char *k    = (unsigned char*) key;
	unsigned  long  hval = 0;

	for (; n > 0; --n)
	{
		hval = hval * 37 ^ (*k++ - ' ');
	}

	return (hval % 1048583) % e->env_hsize;
}

/*
 * var_create():
 *
 * Internal helper function to create an initialize a var_t structure
 * with name and value (value may be NULL).  Returns NULL on memory failure,
 * otherwise a new var is returned.
 */
static var_t* var_create( var_name, value )
	char   *var_name;
	char   *value;
{
	var_t  *v;

	if ((v = (var_t*)malloc(sizeof(var_t))) == NULL)
	{
		sqsh_set_error( SQSH_E_NOMEM, NULL );
		return NULL;
	}

	if (var_name != NULL)
	{
		if ((v->var_name = sqsh_strdup( var_name )) == NULL)
		{
			free( v );
			sqsh_set_error( SQSH_E_NOMEM, NULL );
			return NULL;
		}
	}
	else
	{
		v->var_name = NULL;
	}

	if (value != NULL)
	{
		if ((v->var_value = sqsh_strdup( value )) == NULL)
		{
			if (v->var_name != NULL)
			{
				free( v->var_name );
			}
			free( v );
			sqsh_set_error( SQSH_E_NOMEM, NULL );
			return NULL;
		}
	}
	else
	{
		v->var_value = NULL;
	}

	v->var_sptype = ENV_SP_NONE;
	v->var_setf   = NULL;
	v->var_getf   = NULL;

	return v;
}

/*
 * var_new_value():
 *
 * Replaces the current value of v with value.
 */
static int var_new_value( v, value )
	var_t  *v;
	char   *value;
{
	if (value == NULL)
	{
		if (v->var_value != NULL)
		{
			free( v->var_value );
		}
		v->var_value = NULL;
		return True;
	}

	if (v->var_value == NULL || strlen(value) > strlen(v->var_value))
	{
		if (v->var_value != NULL)
		{
			free( v->var_value );
		}

		if ((v->var_value = sqsh_strdup( value )) == NULL)
		{
			return False;
		}
	}
	else
	{
		strcpy( v->var_value, value );
	}

	return True;
}

static void var_destroy( v )
	var_t *v;
{
	if (v == NULL)
	{
		return;
	}

	if (v->var_name != NULL)
	{
		free( v->var_name );
	}

	if (v->var_value != NULL)
	{
		free( v->var_value );	
	}

	free( v );
}
