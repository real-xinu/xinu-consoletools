/* 
 * cs-status.c - prints both server and connection status
 */

#include <stdio.h>

#include "cserver.h"

extern char * getdfltClass();

/* classes to ignore when -c ALL is specified */
static char * class_ignore_list_ALL[] = { "DOWNLOAD", "POWERCYCLE", NULL };

/* classes to ignore for all other specified classes */
static char * class_ignore_list[] = { "DOWNLOAD", "POWERCYCLE", "tquark", "quark-l", NULL };


static char class[ MAXCLASSNAME ];
static char host[ MAXHOSTNAME ];
static char connection[ MAXCONNECTIONNAME ];

main( argc, argv )
    int argc;
    char * argv[];
{
	int sock;
	int fflag = 0;
	int bflag = 0;
	int i;
	char** ignore_list = class_ignore_list;

	host[ 0 ] = '\0';
	host[ MAXHOSTNAME - 1 ] = '\0';
	connection[ 0 ] = '\0';
	connection[ MAXCONNECTIONNAME - 1 ] = '\0';
	class[ 0 ] = '\0';
	class[ MAXCLASSNAME - 1 ] = '\0';

	strncpy( host, DEFAULTSERVERNAME, MAXHOSTNAME - 1 );

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
		else if( strequ( argv[ i ], "-a") ) {
			host[ 0 ] = '\0';
			host[ MAXHOSTNAME - 1 ] = '\0';
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

	if (class[0] == '\0') {
	    strncpy( class, getdfltClass( class, connection ), MAXCLASSNAME );
	}
	if ( strequ(class, "all") ) {
	    class[ 0 ] = '\0';
	    class[ MAXCLASSNAME - 1 ] = '\0';
	}
	if ( strequ(class, "ALL") ) {
	    class[ 0 ] = '\0';
	    class[ MAXCLASSNAME - 1 ] = '\0';
	    ignore_list = class_ignore_list_ALL;
	}
	if( ( sock = statusrequest( connection, 
				    class,
				    host ) ) < 0 ) 
	  {
	    exit( 1 );
	  }
	while( 1 ) {
		struct reply reply;
		if( statusrecv( sock, & reply ) < 0 ) {
			break;
		}
		printstatus( & reply, fflag, bflag, ignore_list );
	}
	exit( 0 );
}

printusage( sb )
     char * sb;
{
	fprintf( stderr, "usage: %s [-b] [-f] [-a] [-c class] [-s server] [connection]\n", sb );
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
printstatus( reply, fflag, bflag, ignore_list )
     struct reply * reply;
     int fflag;
     int bflag;
     char** ignore_list;
{
	struct statusreplyData * stat;
	int i;
	int numc;
	int igidx;
	int ignore_entry;
	
	if( fflag ) {
		printName( reply->hostname );

		if( bflag ) {
			printf( "\n" );
		}
	}

	numc = atoi( reply->numconnections );
	stat = (struct statusreplyData *) ( reply->details );
	for( i = 0; i < numc; i++, stat++ ) {

		if( ignore_list ) {
			igidx = 0;
			ignore_entry = 0;
			while(ignore_list[igidx] != NULL) {
				if( strequ ( stat->conclass, ignore_list[igidx] ) ) {
					ignore_entry = 1;
					break;
				}
				igidx++;
			}
			if( ignore_entry ) {
				continue;
			}
		}
		
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
