#!./sqsh -i

\func -x test_loop

	\if [ $# -ne 1 ]
		\echo "use: test_loop <n>"
		\return 1
	\fi

	\set x=0
	\while [ $x -lt ${1} ]
		\echo $x
		\set x=`expr $x + 1`
	\done

	\return 0
\done

\if test_loop ${1}
	\echo "All is well"
\else
	\echo "Doh!"
\fi

