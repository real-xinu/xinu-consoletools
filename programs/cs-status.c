/* 
 * cs-status.c - prints both server and connection status
 */

#include <stdio.h>

#include "cserver.h"

main( argc, argv )
    int argc;
    char * argv[];
{
	int sock;
	int fflag = 0;
	int bflag = 0;
	char class[ MAXCLASSNAME ];
	char host[ MAXHOSTNAME ];
	char connection[ MAXCONNECTIONNAME ];
	int i;

	host[ 0 ] = '\0';
	host[ MAXHOSTNAME ] = '\0';
	connection[ 0 ] = '\0';
	connection[ MAXCONNECTIONNAME - 1 ] = '\0';
	class[ 0 ] = '\0';
	class[ MAXCLASSNAME - 1 ] = '\0';

	for( i = 1; i < argc; i++ ) {
		if( strequ( argv[ i ], "-h" ) ) {
			printusage( argv[ 0 ] );
		}
		else if( strequ( argv[ i ], "-b" ) ) {
			bflag = 1;
		}
		else if( strequ( argv[ i ], "-f" ) ) {
			fflag = 1;
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
	
	if( ! fflag && ! bflag ) {
		fflag = bflag = 1;
	}

	if( ( sock = statusrequest( connection, 
				    getdfltClass( class, connection ), 
				    host ) ) < 0 ) 
	  {
	    exit( 1 );
	  }
	while( 1 ) {
		struct reply reply;
		if( statusrecv( sock, & reply ) < 0 ) {
			break;
		}
		printstatus( & reply, fflag, bflag );
	}
	exit( 0 );
}

printusage( sb )
     char * sb;
{
	fprintf( stderr, "usage: %s [-b] [-f] [-c class] [-s server] [connection]\n", sb );
	fprintf( stderr,
		 "cs-status v2.0 \"Millenium Edition\"\n");
	exit( 1 );
}

/*---------------------------------------------------------------------------
 * printName - print the name of the frontend (ignoring DOMAINNAME)
 *---------------------------------------------------------------------------
 */
static void
printName( sb )
     char * sb;
{
#ifdef DOMAINNAME
	int len = strlen( sb );
	int dlen = strlen( DOMAINNAME );
	char * end = sb + len;

	if( ( len > dlen ) && ( *(end - dlen - 1) == '.' ) &&
	    ( strcmp( end - dlen, DOMAINNAME ) == 0 ) ) {
		*(end - dlen - 1) = '\0';
	}
#endif
	printf( "%s:", sb );
}

/*
 *---------------------------------------------------------------------------
 * printstatus - print the status message recieved
 *---------------------------------------------------------------------------
 */
printstatus( reply, fflag, bflag )
     struct reply * reply;
     int fflag;
     int bflag;
{
	struct statusreplyData * stat;
	int i;
	int numc;
	
	if( fflag ) {
		printName( reply->hostname );

		if( bflag ) {
			printf( "\n" );
		}
	}

	numc = atoi( reply->numconnections );
	stat = (struct statusreplyData *) ( reply->details );
	for( i = 0; i < numc; i++, stat++ ) {
		if( bflag ) {
			if( fflag ) {
				printf( "    " );
			}
			printf( "%-16s %-16s user= %-16s time= %s\n",
			       stat->conname, stat->conclass,
			       stat->user, stat->contime );
		}
		else if( fflag ) {
			printf( " %s", stat->conname );
		}
	}
	if( fflag && ! bflag ) {
		printf( "\n" );
	}
}
