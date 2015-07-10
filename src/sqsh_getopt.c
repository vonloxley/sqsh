/*
 * sqsh_getopt.c - Reusable version of getopt(3c)
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
#include "sqsh_varbuf.h"
#include "sqsh_global.h"
#include "sqsh_env.h"
#include "sqsh_getopt.h"

/*-- Current Version --*/
#if !defined(lint) && !defined(__LINT__)
static char RCS_Id[] = "$Id: sqsh_getopt.c,v 1.4 2013/02/24 22:21:10 mwesdorp Exp $" ;
USE(RCS_Id)
#endif /* !defined(lint) */


/*
 * The following variables don't following my usual naming convention
 * for global variables, primarily because I wanted them to look
 * like the standard getopt variable names.
 */
int   sqsh_optind ;
char *sqsh_optarg ;

static  char **sg_argv  = NULL ;     /* Current argv we are using */
static  char  *sg_flags = NULL ;
static  int    sg_nargs;             /* Number of arguments left to process */
static  int    sg_argc ;             /* Current end of argv */

/*
 * The is_flag() macro is used to test a string to see if it is
 * potentially a flag (that is, it is of the format -- or -[a-zA-Z0-9] or -\250).
 */
#define is_flag(s) (*(s) == '-' && (isalnum((int)*((s)+1)) || *((s)+1) == '-' || *((s)+1) == '\250'))

/*-- Prototypes --*/
static int sqsh_move    _ANSI_ARGS(( int, char**, int ));
static int sqsh_nextopt _ANSI_ARGS(( int, char** )) ;

int sqsh_getopt( argc, argv, opt_flags )
	int      argc;
	char    *argv[];
	char    *opt_flags;
{
	char   *opt;
	int     flag;         /* Which flag we have hit */
	int     ignore;
	int     nskip;
	int     i;

	/*
	 * If any one of argc, argv, or opt_flags is different from what
	 * we saw the last time we were run, then we have a new set of
	 * options and should start over again.
	 */
	if( sg_argv != argv || sg_argc != argc || 0 != strcmp( sg_flags, opt_flags) ) {
		sqsh_optind  = 1 ;           /* Skip argv[0] */
		sg_argv      = argv ;
		sg_argc      = argc ;
		sg_flags     = opt_flags ;
		sg_nargs     = argc ;
	}

	/*
	 * While there are arguments left to process...
	 */
	while (sqsh_optind < sg_nargs) {


		/*
		 * If the option doesn't start with a '-', then it isn't
		 * an option, so we move it to the end of our array.
		 */
		if (!is_flag(argv[sqsh_optind])) {

			/*
			 * This moves the entry located at sqsh_optind to the end
			 * of our array of options.  We don't need to increment
			 * sqsh_optind, because we have effectively already done
			 * that.
			 */
			sqsh_move( argc, argv, sqsh_optind );
			--sg_nargs;
			continue;

		}

		/*
		 * If we have reached this point, then we have an option to
		 * process.  Note that thanks to the new '-' modifier in the
		 * opt_flags list, we may still not really be interested in this
		 * flag.
		 */
		flag = argv[sqsh_optind][1];

		if ((opt = strchr( opt_flags, flag )) == NULL) {
			/*
			 * If we don't have an entry for this option, then it is
			 * considered illegal and must be severely mistreated.
			 */
			sqsh_set_error( SQSH_E_BADPARAM, "illegal option -%c", flag ) ;
			++sqsh_optind;
			return '?';
		}

		/*
		 * Now, figure out which options are available for this particular
		 * flag.  Currently there are three supported: A ':' indicates that
		 * the flag requires an argument, a ';' indicates that it may have
		 * an optional argument, and a '-' indicates that we are to ignore
		 * the current argument.  Note that the '-' may be used in combination
		 * with the other two.
		 */
		++opt;
		if (*opt == '-') {
			ignore = True;
			++opt;
		} else {
			ignore = False;
		}

		/*
		 * Check to see if we may need an agument for this particular
		 * flag.
		 */
		if (*opt == ';' || *opt == ':') {
			if (*(opt + 1) == '-')
				ignore = True;
			
			/*
			 * If there is more text left on this option (for example, 
			 * if we have "-foo", then "oo" is the argument to -f. This
			 * means that we need to skip the current argument to get to
			 * the next potential flag.
			 */
			if (argv[sqsh_optind][2] != '\0') {

				sqsh_optarg = &argv[sqsh_optind][2];
				nskip = 1;

			} else if (sqsh_optind < (argc - 1) && !is_flag(argv[sqsh_optind+1])) {
				/*
				 * Otherwise, if there is a next argument on the command line
				 * and it does not begin with a '-', then it is the argument
				 * to our option. If this is the case, then we must skip two
				 * ahead to get to the next flag.
				 */
				sqsh_optarg = argv[sqsh_optind+1];
				nskip = 2;
			} else {
				/*
				 * Oops, it looks like this flag does not have an argument
				 * at all.  But, that may actually be OK.
				 */
				sqsh_optarg = NULL;
				nskip = 1;
			}

			/*
			 * If the option modifier indicates that we have a required 
			 * argument and there isn't one there, then generate an error
			 * message.
			 */
			if (*opt == ':' && sqsh_optarg == NULL) {
				sqsh_set_error( SQSH_E_BADPARAM, 
									 "option requires an argument -%c", flag ) ;
				++sqsh_optind;
				return '?';
			}

		} else {

			/*
			 * We have an option that does not take an argument, so we
			 * want to make sure that one wasn't supplied.
			 */
			if (argv[sqsh_optind][2] != '\0') {
				sqsh_set_error( SQSH_E_BADPARAM,
				                "option does not take an argument -%c", flag );
				++sqsh_optind;
				return '?';
			}

			sqsh_optarg = NULL;
			nskip = 1;

		}


		/*
		 * At this point, sqsh_optind points to the next possible
		 * argument, flag contains the current command line flag,
		 * and sqsh_optarg contains the argument to the flag.  Now,
		 * if we aren't supposed to ignore it, return the flag.
		 */
		if (ignore == False) {
			sqsh_optind += nskip;
			return flag;
		}

		/*
		 * If we are to ignore this flag, then we need to do two things:
		 * first, we need to move both the flag and, potentially, its 
		 * argument onto the end of our array of arguments, then treat
		 * it as if those two arguments don't exist (by reducing our count
		 * of the total number of arguments on the array).
		 */
		sg_nargs -= nskip;
		for (i = 0; i < nskip; i++)
			sqsh_move( argc, argv, sqsh_optind );

	}

	sqsh_optarg = NULL;
	sqsh_set_error( SQSH_E_NONE, NULL );
	return EOF;
}

/*
 * sqsh_move():
 *
 * Helper function for sqsh_getopt() to move the item located
 * at position idx onto the end of the array argv containing
 * argc entries.
 */
static int sqsh_move( argc, argv, idx )
	int   argc;
	char  *argv[];
	int   idx;
{
	char *cptr;
	int   i;

	/*-- Save pointer to argument --*/
 	cptr = argv[idx] ;

	/*
	 * First, we need to shift the rest of the arguments down, 
	 * removing this argument.
	 */
	for( i = idx; i < argc - 1; i++ )
		argv[i] = argv[i+1] ;

	/*
	 * Now, stick this argument on the end of the argv array.
	 * Since we know that one more argument on the list is not
	 * a flag we can decrement sg_argc as well.
	 */
	argv[argc - 1] = cptr ;
	return 1;
}

/*
 * sqsh_getopt_combined():
 *
 * This function combines sqsh_getopt_env() and sqsh_getopt(), by
 * first traversing the contents of the environment variable var_name
 * followed by the command line options argc and argv, returning
 * results as it goes.
 */
int sqsh_getopt_combined( var_name, argc, argv, opt_flags )
	char  *var_name ;
	int    argc ;
	char **argv ;
	char  *opt_flags ;
{
	static int   var_exhausted = False ;
	static char *old_var_name  = NULL ;
	int    ch ;

	if( var_name != NULL ) {
		/*
		 * If the name of the variable changed then we are probably dealing
		 * with an entirely new set of arguments, so we need to reset
		 * everything.
		 */
		if( var_name != old_var_name ) {
			old_var_name  = var_name ;
			sqsh_getopt_reset() ;
			if( (ch = sqsh_getopt_env( var_name, opt_flags )) == EOF )
				var_exhausted = True ;
			else
				return ch ;
		}

		if( var_exhausted == False ) {
			if( (ch = sqsh_getopt_env( NULL, opt_flags )) == EOF )
				var_exhausted = True ;
			else
				return ch ;
		}
	}

	return sqsh_getopt( argc, argv, opt_flags ) ;
}

/*
 * sqsh_getopt_env():
 *
 * Given the name of an environment variable containing command line
 * options, sqsh_getopt_env() returns each option supplied in opt_flags
 * (of the same format as sqsh_getopt()), setting sqsh_optarg as needed.
 * A subsequent call to sqsh_getopt_env() with a NULL var_name causes
 * the next argument to be returned.
 */
int sqsh_getopt_env( var_name, opt_flags )
	char *var_name ;
	char *opt_flags ;
{
	static char     *var_value = NULL ;
	static varbuf_t *arg_buf   = NULL ;
	char            *opt_ptr ;
	int              flag ;
	int              ret_value ;

	/*
	 * The first time through this function the user must pass a
	 * variable name in to be parsed.  So, we check to see if the
	 * variable exists and is non-empty.
	 */
	if( var_name != NULL ) {
		env_get( g_env, var_name, &var_value ) ;
		if( var_value == NULL || *var_value == '\0' )
			goto leave_eof ;
	}

	/*
	 * We need to create a buffer in which to store the argument to
	 * a given parameter.  This buffer should only exist until we
	 * reach EOF.
	 */
	if( arg_buf == NULL ) {
		if( (arg_buf = varbuf_create( 64 )) == NULL )
			goto leave_err ;
	}
	varbuf_clear( arg_buf ) ;

	/*-- Skip leading white-space --*/
	for( ; *var_value != '\0' && isspace((int)*var_value); ++var_value ) ;

	/*-- Are we at the end? --*/
	if( *var_value == '\0' )
		goto leave_eof ;

	/*-- Arguments must begin with a '-' --*/
	if( *var_value != '-' ) {
		sqsh_set_error( SQSH_E_BADPARAM, "Options must begin with '-' (found: %s)",
			       	var_value ) ;
		goto leave_err ;
	}

	/*-- Flag is character following the '-' --*/
	flag       = *(var_value + 1) ;

	/*-- Skip to character following the flag --*/
	var_value += 2 ;

	/*-- Check for invalid argument --*/
	if( flag == '\0' || (opt_ptr = strchr( opt_flags, flag )) == NULL ) {
		sqsh_set_error( SQSH_E_BADPARAM, "illegal option -%c", flag ) ;
		goto leave_err ;
	}

	/*-- Skip white space --*/
	for( ; *var_value != '\0' && isspace((int)*var_value); ++var_value ) ;

	/*
	 * If this is an option that allows an argument, then we need
	 * to try to pull out the argument part.
	 */
	if( *(opt_ptr+1) == ':' || *(opt_ptr+1) == ';' ) {
		/*
		 * Check to see if the next thing on the line is either EOF
		 * or another argument.
		 */
		if( *var_value == '\0' || is_flag(var_value) ) {
			/*
			 * If this option requires an argument and there isn't one
			 * available, then spew an error message.
			 */
			if( *(opt_ptr+1) == ':' ) {
				sqsh_set_error( SQSH_E_BADPARAM, "option requires an argument -%c",
									 flag ) ;
				goto leave_err ;
			}

			/*
			 * This was an optional argument, and it didn't exist, so
			 * let the caller know.
			 */
			sqsh_optarg = NULL ;
			return flag ;
		}

		/*
		 * There was an argument to the option, so copy from the current
		 * position to either the next white space or EOF into our arg_buf.
		 */
		while( *var_value != '\0' && !(isspace((int)*var_value)) ) {
			if( varbuf_charcat( arg_buf, *var_value ) == -1 )
				goto leave_err ;
			++var_value ;
		}

		sqsh_optarg = varbuf_getstr( arg_buf ) ;
		return flag ;
	}

	/*
	 * The option doesn't take an argument, so simply return.
	 */
	sqsh_optarg = NULL ;
	return flag ;

leave_err:
	ret_value = '?' ;
	goto clean_up ;

	/*
	 * Clean up following an EOF.  This consists of destroying the
	 * argument buffer, setting the var_value to NULL, and returning.
	 */
leave_eof:
	ret_value = EOF ;

clean_up:
	if( arg_buf != NULL ) {
		varbuf_destroy( arg_buf ) ;
		arg_buf = NULL ;
	}
	var_value = NULL ;
	return ret_value ;
}

/*
 * sqsh_getopt_reset():
 *
 * Forces sqsh_getopt() to start from the beginning of the argument
 * list.
 */
int sqsh_getopt_reset()
{
	sg_argv = NULL ;	
	return True ;
}

/*
 * sqsh-3.0
 * Process long options that are specfied as --xxxxx yyyyy
 * or --xxxxx=yyyyy
 */
int sqsh_getopt_long ( argc, argv, option, slt )
    int              argc ;
    char           **argv ;
    char            *option;
    sqsh_longopt_t   slt[];
{
    int    i;
    char *ch;


    /*
     * The long option maybe specified as --option=argument
    */
    if ( (ch = strchr( option, '=' )) != NULL )
        *ch = '\0';

    for ( i = 0; slt[i].name != NULL; ++i )
    {
        if ( strcasecmp (option, slt[i].name) == 0 )
        {
            if ( slt[i].reqarg == 1 )
            {
               if (ch != NULL)
                   sqsh_optarg = ch + 1;
               else
                   if ( sqsh_nextopt ( argc, argv ) == False )
                       return '?';
            }
            return slt[i].shortopt;
        }
    }
    return '?';
}        

/*
 * sqsh-3.0
 * Set sqsh_optarg to next option in the arglist.
 * Return False if there are no more options available or if the next option is the
 * password option -\250.
*/
static int sqsh_nextopt ( argc, argv )
    int    argc ;
    char **argv ;
{

    if (sqsh_optind < sg_nargs)
    {
        if (argv[sqsh_optind][0] == '-' && argv[sqsh_optind][1] == '\250' )
            return False;
        sqsh_optarg = argv[sqsh_optind++];
    }
    else
        return False;

    return True ;
}
