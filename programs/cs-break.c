/*
 * cs-lock.c - lock and unlock a connectioin
 */

#include <stdio.h>

#include "cserver.h"

main( argc, argv )
     int argc;
     char * argv[];
{
	char class[ MAXCLASSNAME ];
	char host[ MAXHOSTNAME ];
	char connection[ MAXCONNECTIONNAME ];
	int i;

	host[ 0 ] = '\0';
	host[ MAXHOSTNAME - 1 ] = '\0';
	connection[ 0 ] = '\0';
	connection[ MAXCONNECTIONNAME - 1 ] = '\0';
	class[ 0 ] = '\0';
	class[ MAXCLASSNAME - 1 ] = '\0';
	for( i = 1; i < argc; i++ ) {
		if( strequ( argv[ i ], "-h" ) ) {
			printusage( argv[ 0 ] );
		}
		else if( strequ( argv[ i ], "-c" ) ) {
			i++;
			if( i >= argc ) {
				fprintf( stderr,
					"error: missing class name\n" );
				printusage( argv[ 0 ] );
			}
			strncpy( class, argv[ i ], MAXCLASSNAME - 1 );
		}
		else if( strequ( argv[ i ], "-s" ) ) {
			i++;
			if( i >= argc ) {
				fprintf( stderr,
					"error: missing server name\n" );
				printusage( argv[ 0 ] );
			}
			strncpy( host, argv[ i ], MAXHOSTNAME - 1 );
		}
		else if( argv[ i ][ 0 ] == '-' ) {
			fprintf( stderr, "error: unexpected argument '%s'\n",
				   argv[ i ] );
			printusage( argv[ 0 ] );
		}
		else {
			strncpy( connection, argv[ i ], MAXCONNECTIONNAME-1 );
		}
	}

	if( connection[ 0 ] == '\0' ) {
		fprintf( stderr, "error: no connection specified\n" );
		printusage( argv[ 0 ] );
	}
	
	if( breakConnection( connection, class, host ) < 0 ) {
		exit( 1 );
	}
	printf( "broke connection '%s'\n", connection );
	exit( 0 );
}

printusage( sb )
     char * sb;
{
	fprintf( stderr, "usage: %s [-s server] [-c class] connection\n", sb );
	exit( 1 );
}


