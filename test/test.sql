#!./sqsh -i

\if ls -al 
	\echo hello
\fi

\if test $# -ne 1
	\echo "use: test.sql <digit>"
	\exit
\fi

\if [ $# -eq 1 ]
	\echo "wooo hoo"
\fi

\echo "Testing while loop"

\set x=1
\while test $x -lt ${1}
	\echo $x
	\set x=`expr $x + 1`
\done

\echo "If-Elif-Else logic"

\if test ${1} = 1
	\echo block one
\elif test ${1} = 2
	\echo block two
\elif test ${1} = 3
	\echo block three
\else
	\echo block four
\fi

