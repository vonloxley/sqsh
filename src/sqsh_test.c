#include <stdio.h>
#include "sqsh_config.h"
#include "sqsh_varbuf.h"
#include "sqsh_filter.h"

main( argc, argv )
	int    argc;
	char  *argv[];
{
	FILE      *file;
	char      line[1024];
	varbuf_t  *inbuf;
	varbuf_t  *outbuf;

	inbuf = varbuf_create( 1024 );

	if (inbuf == NULL)
	{
		fprintf( stderr, "varbuf_create: %s\n", sqsh_get_errstr() );
		exit(1);
	}

	outbuf = varbuf_create( 1024 );

	if (outbuf == NULL)
	{
		fprintf( stderr, "varbuf_create: %s\n", sqsh_get_errstr() );
		exit(1);
	}

	if (argc == 2)
	{
		file = fopen( argv[1], "r" );
		if (file == NULL)
		{
			fprintf( stderr, "fopen: Unable to open %s\n", argv[1] );
			exit(1);
		}
	}
	else
		file = stdin;

	while (fgets( line, sizeof(line), file ) != NULL)
	{
		varbuf_strcat( inbuf, line );
	}
	fclose( file );

	if (sqsh_filter( varbuf_getstr(inbuf),
	                 varbuf_getlen(inbuf),
	                 "m4 -", outbuf ) == False)
	{
		fprintf( stderr, "sqsh_filter: %s\n", sqsh_get_errstr() );
		exit(1);
	}

	fputs( varbuf_getstr( outbuf ), stdout );
	fflush(stdout);
	exit(0);
}

