#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "cserver.h"
#include "timeout.h"

#define BIGBUFF		40000

extern char *xmalloc();
extern char * getenv();
extern char * index();
extern unsigned long inet_addr();

/*---------------------------------------------------------------------------
 * getservers - get the servers from the environment variable
 *---------------------------------------------------------------------------
 */
static int
getservers( servers, host )
     char * servers[];
     char * host;
{
	char * sb;
	char * copy;
	char * p;
	int i;

	if( host != NULL && *host != '\0' ) {
		servers[ 0 ] = host;
		return( 1 );
	}

#ifdef ENVSERVERS
	if( ( sb = getenv( ENVSERVERS ) ) == NULL ) {
		return( 0 );
	}
	
	while( *sb == ' ' ) {
		sb++;
	}
	
	copy = xmalloc( strlen( sb ) + 1 );
	strcpy( copy, sb );
	for( i = 0, sb = copy;  sb != NULL && *sb != '\0'; i++ ) {
		if( ( sb = index( copy, ' ' ) ) != NULL ) {
			*sb++ = '\0';
			while( *sb == ' ' ) {
				sb++;
			}
			servers[ i ] = copy;
		}
		else {
			servers[ i ] = copy;
		}
		copy = sb;
		if( i >= ( MAXSERVERS - 1 ) ) {
			break;
		}
	}
	servers[ i ] = NULL;
	return( i );
#else
	return( 0 );
#endif
}

/*---------------------------------------------------------------------------
 * fakemulticast - Fake a multicast by sending to the given list of bed servers
 *---------------------------------------------------------------------------
 */
static int
fakemulticast( servers, numservers, sock, req )
     char * servers[];
     int numservers;
     int sock;
     struct request * req;
{
	struct sockaddr_in sa;
	struct hostent * phent;
	int i;
	int bcast;
	unsigned long addr;
	
	bcast = 0;
	for( i = 0; i < numservers; i++ ) {
		if( ! strcmp( "*", servers[ i ] ) ) {
			bcast = 1;
			continue;
		}
		if( strlen( servers[ i ] ) == 0 ) {
			continue;
		}
		sa.sin_family = AF_INET;
		sa.sin_port   = htons( CS_PORT );
		if( inet_aton( servers[ i ], &sa.sin_addr ) > 0 ) {
			/* null */
		}
		else if( ( phent = gethostbyname( servers[ i ] ) ) != NULL ) {
			bcopy( phent->h_addr, (char *) & sa.sin_addr,
			      phent->h_length );
		}
		else {
			fprintf( stderr, "warning: unknown host '%s'\n",
				servers[ i ] );
			continue;
		}
		
		(void) sendtoRequest( sock, & sa, req );
	}
	return( bcast );
}

/*---------------------------------------------------------------------------
 * statusrequest - send a status request message to all frontends
 *---------------------------------------------------------------------------
 */
int
statusrequest( connection, class, host )
     char * connection;
     char * class;
     char * host;
{
	int sock;
	int status;
	struct sockaddr_in sa;
	struct request req;
	int maxbuff, lmb;		/* used to increase socket buf	*/
	char * servers[ MAXSERVERS ];	/* names of servers		*/
	int numservers;			/* number of servers		*/
	int bcast;

	initRequest( & req, REQ_STATUS, connection, class );
	
	if( ( sock = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
		fprintf( stderr, "error: socket failed\n" );
		return( -1 );
	}
	
	sa.sin_family = AF_INET;
	sa.sin_port   = htons( 0 );
	sa.sin_addr.s_addr = INADDR_ANY;
	if( bind( sock, (struct sockaddr *) & sa,
		  sizeof( struct sockaddr_in ) ) < 0 ) {
		fprintf( stderr, "error: bind failed\n" );
		return( -1 );
	}
	
#ifndef HPPA

	maxbuff = BIGBUFF;
	lmb = sizeof( maxbuff );
	if( setsockopt( sock, SOL_SOCKET, SO_RCVBUF,
			(char *) & maxbuff, lmb ) != 0 ) {
		fprintf( stderr, "error: setsockopt falied\n" );
		return( -1 );
	}

#endif
		
	bcast = 0;
	numservers = 0;
	servers[ 0 ] = NULL;
	numservers = getservers( servers, host );
	if( numservers != 0 ) {
		bcast = fakemulticast( servers, numservers, sock, & req );
	}
	else {
		bcast = 1;
	}
	if( bcast > 0 ) {
		status = bcastUDP( sock, (char *) & req,
				   sizeof( struct request), CS_PORT );
		if( status < 0 ) {
			fprintf( stderr,
				"error: broadcasting status request failed\n");
			return( -1 );
		}
	}
	return( sock );
}

/*---------------------------------------------------------------------------
 * statusrecv - receive one status reply and return it
 *---------------------------------------------------------------------------
 */
int
statusrecv( sock, reply )
     int sock;
     struct reply * reply;
{
	if( recvReply( sock , XSTIMEOUT, reply ) < 0 ) {
		return( -1 );
	}
	return( 0 );
}

/*----------------------------------------------------------------------
 * obtainStatus - obtain status info from all servers
 *----------------------------------------------------------------------
 */
int
obtainStatus( connection, class, host, stats, cnt )
     char * connection;
     char * class;
     char * host;
     struct reply * stats[];
     int cnt;
{
	int i;
	int sock;
	struct reply * reply;
	
	if( ( sock = statusrequest( connection, class, host ) ) < 0 ) {
		exit( 1 );
	}
	for( i = 0; i < cnt; i++ ) {
		reply = (struct reply *) xmalloc( sizeof( struct reply ) );
		if( statusrecv( sock, reply ) < 0 ) {
			break;
		}
		stats[ i ] = reply;
	}
	close( sock );
	return( i );
}

