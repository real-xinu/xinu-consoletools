/*
 * cs-command.c - send command to server
 */

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "cserver.h"
#include "timeout.h"

main ( argc, argv )
     int argc;
     char *argv[];
{
	int i;
	struct request req;
	struct reply reply;
	char code = -1;

	initRequest( & req, code, NULL, NULL );

	for( i = 1; i < argc; i++ ) {
		if( strequ( argv[ i ], "-h" ) ) {
			printusage( argv[ 0 ] );
		}
		else if( strequ( argv[ i ], "-k" ) ) {
			code = REQ_QUIT;
		}
		else if( strequ( argv[ i ], "-r" ) ) {
			code = REQ_REBOOT;
		}
		else if( strequ( argv[ i ], "+v" ) ) {
			code = REQ_VERBOSE_ON;
		}
		else if( strequ( argv[ i ], "-v" ) ) {
			code = REQ_VERBOSE_OFF;
		}
		else if( *argv[ i ] == '-' ) {
			fprintf( stderr, "unknown options '%s'\n", argv[ i ] );
			printusage( argv[ 0 ] );
		}
		else if( strequ( "ALL", argv[ i ] ) ) {
			int sock;

			if( code < 0 ) {
				printusage( argv[ 0 ] );
			}
			req.code = code;
			
			/***** set up broadcast connection *****/
			sock = socket( AF_INET, SOCK_DGRAM, 0 );
			
			if( bcastUDP( sock, & req, sizeof( struct request ),
				      CS_PORT ) < 0 ) {
				continue;
			}
			
			while( 1 ) {
				if( recvReply( sock, STIMEOUT, & reply )
				    < 0 ) {
					break;
				}
				printf( "server %s: '%s'\n",
				       reply.hostname, reply.details );
			}
			continue;
		}
		else {
			int sock;
			char * host = argv[ i ];

			if( *host == '/' ) {
				host++;
			}

			if( code < 0 ) {
				printusage( argv[ 0 ] );
			}
			req.code = code;
			
			if( ( sock = connectUDP( host, CS_PORT ) ) < 0 ) {
				continue;
			}
			if( sendRequest( sock, & req ) < 0 ) {
				close( sock );
				continue;
			}
			if( recvReply( sock, STIMEOUT, & reply ) > 0 ) {
				printf( "server %s: '%s'\n",
				       reply.hostname, reply.details );
			}
			close( sock );
		}
	}
	exit( 0 );
}

printusage( sb )
     char * sb;
{
	fprintf( stderr, "usage: %s [ [-k -r -v +v] [hosts | ALL]* ]*\n", sb );
	exit( 1 );
}
