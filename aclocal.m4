dnl #   AC_VOLATILE (Scott C. Gray gray@voicenet.com)
dnl #
dnl #   Determines whether or not a compiler supports the volatile
dnl #   keyword.  Defines HAVE_VOLATILE
dnl #
AC_DEFUN(AC_VOLATILE,
[
	AC_MSG_CHECKING(whether ${CC-cc} supports volatile keyword)
	AC_CACHE_VAL(ac_cv_have_volatile,
	[
		AC_TRY_COMPILE(
			[#include <stdio.h>],
			[volatile int a ; a = 10 ;],
			ac_cv_have_volatile=yes, ac_cv_have_volatile=no )
	])
	AC_MSG_RESULT($ac_cv_have_volatile)

	if (test "$ac_cv_have_volatile" = "yes"); then
		AC_DEFINE(HAVE_VOLATILE, [1], [Define to 1 if your compiler supports the volatile keyword])
	fi
])

AC_DEFUN(AC_SIGSETJMP,
[
	AC_MSG_CHECKING(for sigsetjmp)
	AC_CACHE_VAL(ac_cv_have_sigsetjmp,
	[
		AC_TRY_COMPILE([
#include <stdio.h>
#include <setjmp.h>
		],[
			sigjmp_buf  jb;

			sigsetjmp(jb, 1);
		],[
			ac_cv_have_sigsetjmp=yes
		],[
			ac_cv_have_sigsetjmp=no
		])
	])

	AC_MSG_RESULT($ac_cv_have_sigsetjmp)

	if (test "$ac_cv_have_sigsetjmp" = "yes"); then
		AC_DEFINE(HAVE_SIGSETJMP, [1], [Define to 1 if your environment provides sigsetjpm()])
	fi
])

AC_DEFUN(AC_POSIX_SIGNALS,
[
	AC_MSG_CHECKING(for POSIX signal handling)
	AC_CACHE_VAL(ac_cv_have_posix_sigs,
	[
		AC_TRY_LINK(
		[
#include <stdio.h>
#include <signal.h>
   	],[
      		struct sigaction sa;
      		sigaction( SIGINT, NULL, &sa );
   	],[
				ac_cv_have_posix_sigs="yes"
		],[
				ac_cv_have_posix_sigs="no"
		])
   ])

	if test "$ac_cv_have_posix_sigs" = "yes"; then
		AC_DEFINE(HAVE_POSIX_SIGNALS, [1], [Define to 1 if your environment supprts POSIX signals])
	fi
	AC_MSG_RESULT($ac_cv_have_posix_sigs)
])

#
# AC_FIND_HEADER([hdr],[path],[action_found],[action_notfound])
#
AC_DEFUN([AC_FIND_HDR], [
	AC_MSG_CHECKING(for $1)
	ac_cache_var=ac_cv_hdr_`echo $1 | sed -e 's,[[/.]],_,g'`

	AC_CACHE_VAL($ac_cache_var,
		[
			SEARCH_PATH="/usr/include:/usr/local/include:${HOME}/include:${HOME}/usr/include"
			include_dir=""

			if test "$2" != ""; then
				SEARCH_PATH="$2:${SEARCH_PATH}"
			fi

			IFS="${IFS=  }"; ac_save_ifs="$IFS"; IFS="${IFS}:"
			for dir in ${SEARCH_PATH}; do
				if test -f ${dir}/$1; then
					include_dir=${dir}
					break;
				fi
			done
			IFS="$ac_save_ifs"

			eval ${ac_cache_var}=${include_dir}
		])

	include_dir=${ac_cache_var}
	include_dir=`eval echo '$'${include_dir}`

	if test "${include_dir}" != ""; then
		AC_MSG_RESULT($include_dir)
		$3
	else
		AC_MSG_RESULT(not found)
		$4
	fi
])

#
# AC_FIND_LIB([lib_name],[path],[action_found],[action_notfound])
#
AC_DEFUN(AC_FIND_LIB, [
	AC_MSG_CHECKING(for lib$1)
	AC_CACHE_VAL(ac_cv_lib$1,
		[
			if test "$ac_cv_bit_mode" = "64"; then
				SEARCH_PATH="/lib64:/usr/lib64:/usr/local/lib64:${HOME}/lib64:${HOME}/usr/lib64:/lib:/usr/lib:/usr/local/lib:${HOME}/lib:${HOME}/usr/lib"
			else
				SEARCH_PATH="/lib:/usr/lib:/usr/local/lib:${HOME}/lib:${HOME}/usr/lib"
			fi
			libdir=""

			if test "${LD_LIBRARY_PATH}" != ""; then
				SEARCH_PATH="${LD_LIBRARY_PATH}:${SEARCH_PATH}"
			fi

			if test "$2" != ""; then
				SEARCH_PATH="$2:${SEARCH_PATH}"
			fi

			IFS="${IFS=  }"; ac_save_ifs="$IFS"; IFS="${IFS}:"
			for dir in ${SEARCH_PATH}; do
				for lib in ${dir}/lib$1.*; do
					if test -f "${lib}"; then
						libdir=${dir}
						break;
					fi
				done

				if test "${libdir}" != ""; then
					break;
				fi
			done
			IFS="$ac_save_ifs"

			ac_cv_lib$1="${libdir}"
		])

	libdir=$ac_cv_lib$1

	if test "${libdir}" != ""; then
		AC_MSG_RESULT($libdir)
		$3
	else
		AC_MSG_RESULT(not found)
		$4
	fi
])

dnl #   AC_BIT_MODE  (M Wesdorp)
dnl #
dnl #   Determines whether we are compiling and running code in 32 or 64 bit mode
dnl #
AC_DEFUN(AC_BIT_MODE,
[
	AC_MSG_CHECKING(Compiling 32/64 bit mode)
	AC_CACHE_VAL(ac_cv_bit_mode,
	[
		AC_TRY_RUN(
		[
main() {
#if defined (__LP64__) || defined (__LP64) || defined (_LP64) || defined (__arch64__)
    exit (0);
#else
    exit (1);
#endif
}
		], ac_cv_bit_mode=64, ac_cv_bit_mode=32)
	])

	AC_MSG_RESULT($ac_cv_bit_mode)

	if (test "$ac_cv_bit_mode" = "64"); then
		CPPFLAGS="-DSYB_LP64 $CPPFLAGS"
	fi
	AC_SUBST(CPPFLAGS)
])

dnl #   AC_SYSV_SIGNALS  (Scott C. Gray gray@voicenet.com)
dnl #
dnl #   Determines behaviour of signals, if they need to be reinstalled
dnl #   immediately after calling.
dnl #
AC_DEFUN(AC_SYSV_SIGNALS,
[
	AC_MSG_CHECKING(signal behaviour)
	AC_CACHE_VAL(ac_cv_signal_behaviour,
	[
		AC_TRY_RUN(
		[
/*-- Exit 0 if BSD signal, 1 if SYSV signal --*/
#include <signal.h>
static int sg_sig_count = 0 ;
void sig_handler() { ++sg_sig_count ; }
main() {
#ifdef SIGCHLD
#define _SIG_ SIGCHLD
#else
#define _SIG_ SIGCLD
#endif
    signal( _SIG_, sig_handler ) ;
    kill( getpid(), _SIG_ ) ;
    kill( getpid(), _SIG_ ) ;
    exit( sg_sig_count != 2 ) ;
}
		], ac_cv_signal_behaviour=BSD, ac_cv_signal_behaviour=SYSV)
	])

	AC_MSG_RESULT($ac_cv_signal_behaviour)

	if (test "$ac_cv_signal_behaviour" = "SYSV"); then
		AC_DEFINE(SYSV_SIGNALS, [1], [Define to 1 if your environment supports SysV signals])
	fi
])

AC_DEFUN([AC_SYBASE_ASE], [

	#
	#  Is $SYBASE defined?
	#
	AC_MSG_CHECKING(Open Client installation)
		if [[ "$SYBASE" = "" ]]; then
			AC_MSG_RESULT(no)
			AC_MSG_ERROR([Unable to locate Sybase installation. Check your SYBASE environment variable setting.])
		fi

		SYBASE_OCOS=
		for subdir in $SYBASE $SYBASE/OCS $SYBASE/OCS-[[0-9]]*
		do
			if [[ -d $subdir/include ]]; then
				SYBASE_OCOS=$subdir
				break
			fi
		done

		if [[ "$SYBASE_OCOS" = "" ]]; then
			AC_MSG_RESULT(fail)
			AC_MSG_ERROR([Sybase include files not found in \$SYBASE. Check your SYBASE environment variable setting.])
		fi

	AC_MSG_RESULT($SYBASE_OCOS)

	AC_MSG_CHECKING(Open Client libraries)

		#
		# SYBASE_VERSION=`strings $SYBASE_OCOS/lib/lib*ct*.a 2>/dev/null | \
		#	cut -c1-80 | fgrep 'Sybase Client-Library' | cut -d '/' -f 2 | uniq`
		#
		SYBASE_VERSION=`$SYBASE_OCOS/bin/isql -v 2>/dev/null | cut -c1-80 | grep -e '[[SAP|Sybase]] CTISQL Utility' | \
			cut -d '/' -f 2 | cut -d . -f 1`

		if [[ "$SYBASE_VERSION" = "" ]]; then
		#
		# Assume this is a FreeTDS build
		#
			SYBASE_VERSION="FreeTDS"
			if [[ "$ac_cv_bit_mode" = "64" -a -f $SYBASE_OCOS/lib64/libct.so ]]; then
				SYBASE_LIBDIR="$SYBASE_OCOS/lib64"
			else
				SYBASE_LIBDIR="$SYBASE_OCOS/lib"
			fi
			if [[ ! -f $SYBASE_LIBDIR/libct.so ]]; then
				AC_MSG_RESULT(fail)
				AC_MSG_ERROR([No properly installed FreeTDS or Sybase environment found in ${SYBASE_OCOS}.])
			fi
			case "${host_os}" in
	                        *cygwin)
					SYBASE_LIBS="-lct"
					CPPFLAGS="-D_MSC_VER=800 $CPPFLAGS"
					with_static="no"
					;;
				*)
					if [[ "$with_static" = "yes" -a -f $SYBASE_LIBDIR/libct.a ]]; then
						SYBASE_LIBS="$SYBASE_LIBDIR/libct.a"
					else
						SYBASE_LIBS="-lct"
					fi
					;;
			esac
		else
		#
		# Assume this is a build using Sybase OpenClient libraries
		#
			if [[ "$with_devlib" = "yes" -a -d $SYBASE_OCOS/devlib ]]; then
				SYBASE_LIBDIR="$SYBASE_OCOS/devlib"
			else
				SYBASE_LIBDIR="$SYBASE_OCOS/lib"
			fi

			SYBASE_LIBS=

			case "${host_os}" in
				*cygwin)
					if [[ $SYBASE_VERSION -ge 15 -a "$ac_cv_bit_mode" = "64" ]]; then
						SYBASE_LIBS="-L../cygwin -lsybcs64 -lsybct64 -lsybblk64"
					else if [[ $SYBASE_VERSION -ge 15 -a "$ac_cv_bit_mode" = "32" ]]; then
						SYBASE_LIBS="-L../cygwin -lsybcs -lsybct -lsybblk"
					else
						SYBASE_LIBS="-L../cygwin -lcs -lct -lblk"
					fi
					fi
					CPPFLAGS="-D_MSC_VER=800 $CPPFLAGS"
					with_static="no"
					;;
				*)
					if [[ "$ac_cv_bit_mode" = "32" ]]; then
						if      [[ $SYBASE_VERSION -eq 10 ]]; then
							libtst="blk comn cs ct intl sybtcl tcl unic insck tli"
						else if [[ $SYBASE_VERSION -lt 15 ]]; then
							libtst="blk comn cs ct intl sybtcl tcl unic"
						else if [[ $SYBASE_VERSION -ge 15 ]]; then
							libtst="sybblk sybcomn sybcs sybct sybintl sybtcl sybunic"
						fi
						fi
						fi
					else
						if      [[ $SYBASE_VERSION -eq 10 ]]; then
							libtst="blk64 comn64 cs64 ct64 intl64 sybtcl64 tcl64 unic64 insck64 tli64"
						else if [[ $SYBASE_VERSION -lt 15 ]]; then
							libtst="blk64 comn64 cs64 ct64 intl64 sybtcl64 tcl64 unic64"
						else if [[ $SYBASE_VERSION -ge 15 ]]; then
							libtst="sybblk64 sybcomn64 sybcs64 sybct64 sybintl64 sybtcl64 sybunic64"
						fi
						fi
						fi
					fi

					for i in $libtst
					do
						x=
						if [[ -f $SYBASE_LIBDIR/lib${i}.a -o -f $SYBASE_LIBDIR/lib${i}.so ]]; then
							if [[ "$with_static" = "yes" -a -f $SYBASE_LIBDIR/lib${i}.a ]]; then
								x="$SYBASE_LIBDIR/lib${i}.a"
							else
								x="-l${i}"
							fi
						fi
						if test -n $x; then
							SYBASE_LIBS="$SYBASE_LIBS $x"
						fi
					done
					;;
			esac
		fi

	AC_MSG_RESULT($SYBASE_LIBS)

	AC_MSG_CHECKING(Open Client OS libraries)
		case "${host_os}" in
			linux*)
				SYBASE_OS="-ldl -lm";;
			irix*)
				SYBASE_OS="-lnsl -lm";;
			ncr*)
				SYBASE_OS="-ldl -lm";;
			sunos*)
				SYBASE_OS="-lm";;
			solaris*)
				SYBASE_OS="-lnsl -ldl -lm";;
			osf1*)
				SYBASE_OS="-lsdna -ldnet_stub -lm";;
			ultrix*)
				SYBASE_OS="-lsdna -ldnet_stub -lm";;
			hpux*)
				SYBASE_OS="-lcl -lm -lsec -lBSD";;
			dgux*)
				SYBASE_OS="-lm -ldl -ldgc";;
			aix*)
				SYBASE_OS="-lm";;
			*cygwin)
				SYBASE_OS="";;
			*)
				SYBASE_OS="-lm -ldl";;
		esac

	AC_MSG_RESULT($SYBASE_OS)

	SYBASE_INCDIR="-I${SYBASE_OCOS}/include"
	SYBASE_LIBDIR="-L${SYBASE_LIBDIR}"

	AC_SUBST(CPPFLAGS)
	AC_SUBST(SYBASE)
	AC_SUBST(SYBASE_VERSION)
	AC_SUBST(SYBASE_INCDIR)
	AC_SUBST(SYBASE_LIBDIR)
	AC_SUBST(SYBASE_LIBS)
	AC_SUBST(SYBASE_OCOS)
	AC_SUBST(SYBASE_OS)
])

