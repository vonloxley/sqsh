#!./sqsh -i

\func -x test_loop
	\echo shit
\done

test_loop ${1}

\echo $?
