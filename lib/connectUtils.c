
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include "cserver.h"
#include "timeout.h"

extern char * getuser();

/*---------------------------------------------------------------------------
 * recvfromReply - receive a reply structure from a specified host
 *---------------------------------------------------------------------------
 */
static int
sget( from, dlen, to, max )
	char ** from;
	int * dlen;
	char * to;
	int max;
{
	int slen;
	
	for( slen = 0; ((*dlen ) > 0) && (slen < max); slen++ ) {
		char ch = **from;
		
		(*dlen)--;
		(*from)++;
		
		*to++ = ch;
		
		if( ch == '\0' ) {
			return( 1 );
		}
	}
	return( 0 );
}

int
recvfromReply( s, from, timeout, reply )
     int s;
     struct sockaddr_in * from;
     int timeout;
     struct reply * reply;
{
	int status;
	struct sockaddr_in dummy;
	int fromlen = sizeof( struct sockaddr_in );
	int numc;
	int rlen;
	int dlen;
	struct reply replycopy;

	if( from == NULL ) {
		from = & dummy;
	}

	if( ! readDelay( s, timeout ) ) {
		return( -1 );
	}
	
	if( ( status = recvfrom( s, (char *) & replycopy,
				 sizeof( struct reply ),
				 0, (struct sockaddr *) from,
				 & fromlen ) ) <= 0 ) {
		fprintf( stderr, "can't receive reply: %s\n",
			strerror( errno ) );
		return( -1 );
	}

	dlen = status - ( sizeof( struct reply ) - MAXDETAILS );
	
	if( dlen < 0 ) {
		fprintf( stderr, "reply wrong size\n" );
		return( -1 );
	}

	if( replycopy.version != CURVERSION ) {
		fprintf( stderr, "wrong version" );
		return( -1 );
	}
		
	replycopy.hostname[ MAXHOSTNAME - 1 ] = '\0';
	
	replycopy.numconnections[ MAXNCONNECTIONS - 1 ] = '\0';
	
	bcopy( & replycopy, reply, sizeof( struct reply ) - MAXDETAILS );
	
	numc = atoi( replycopy.numconnections );

	if( numc <= 0 ) {
		if( dlen == MAXDETAILS ) {
			dlen--;
		}
		replycopy.details[ dlen ] = '\0';
		bcopy( replycopy.details, reply->details, dlen + 1 );
	}
	else {
		char * from = replycopy.details;
		struct statusreplyData * to =
		    (struct statusreplyData *) reply->details;
		int i;
		int active;

		for( i = 0; i < numc; i++, to++ ) {
			if( ! sget( & from, & dlen, to->conname,
				    MAXCONNECTIONNAME ) ) {
				fprintf( stderr, "reply wrong size\n" );
				return( -1 );
			}
			if( ! sget( & from, & dlen, to->conclass,
				    MAXCLASSNAME ) ) {
				fprintf( stderr, "reply wrong size\n" );
				return( -1 );
			}

			if( dlen <= 0 ) {
				fprintf( stderr, "reply wrong size\n" );
				return( -1 );
			}

			active = *from++;
			dlen--;

			if( ! active ) {
				to->active = 0;
				to->user[0] = '\0';
				to->contime[0] = '\0';
				continue;
			}

			to->active = 1;
			
			if( ! sget( & from, & dlen, to->user,
				    MAXUSERNAME ) ) {
				fprintf( stderr, "reply wrong size\n" );
				return( -1 );
			}
			
			if( ! sget( & from, & dlen, to->contime,
				    MAXTIMELENGTH ) ) {
				fprintf( stderr, "reply wrong size\n" );
				return( -1 );
			}
		}
	}

	return( sizeof( struct reply ) );
}

int
recvReply( s, timeout, reply )
     int s;
     int timeout;
     struct reply * reply;
{
	return( recvfromReply( s, NULL, timeout, reply ) );
}

/*---------------------------------------------------------------------------
 * sendtoRequest - send a request struct in a UDP packet to a specific host
 *---------------------------------------------------------------------------
 */
int
sendtoRequest( s, to, req )
    int s;
    struct sockaddr_in * to;
    struct request * req;
{
	if( sendto( s, (char *) req, sizeof( struct request ), 0,
		    (struct sockaddr *)to, sizeof( struct sockaddr ) ) <= 0 ) {
		fprintf( stderr, "can't sendto request: %s\n",
		     strerror( errno ) );
		return( -1 );
	}
	return( sizeof( struct request ) );
}

int
sendRequest( s, req )
    int s;
    struct request * req;
{
	if( send( s, (char *) req, sizeof( struct request ), 0 ) <= 0 ) {
		fprintf( stderr, "can't send request: %s\n",
			strerror( errno ) );
		return( -1 );
	}
	return( sizeof( struct request ) );
}

initRequest( req, code, connection, class )
     struct request * req;
     char code;
     char * connection;
     char * class;
{
	if( connection == NULL ) {
		connection = "";
	}
	if( class == NULL ) {
		class = "";
	}
	
	req->version = CURVERSION;
	
	req->code = code;
	
	strncpy( req->user, getuser(), MAXUSERNAME );
	
	req->user[ MAXUSERNAME - 1 ] = '\0';
	
	strncpy( req->conname, connection, MAXCONNECTIONNAME - 1 );
	
	req->conname[ MAXCONNECTIONNAME - 1 ] = '\0';
	
	strncpy( req->conclass, class, MAXCLASSNAME - 1 );
	
	req->conclass[ MAXCLASSNAME - 1 ] = '\0';
}

initReply( reply, code, host, numc )
     struct reply * reply;
     char code;
     char * host;
     int numc;
{
	if( host == NULL ) {
		host = "";
	}
	
	reply->version = CURVERSION;
	
	reply->code = code;
	
	strncpy( reply->hostname, host, MAXHOSTNAME );
	
	reply->hostname[ MAXHOSTNAME - 1 ] = '\0';
	
	sprintf( reply->numconnections, "%d", numc );
	
	reply->numconnections[ MAXNCONNECTIONS - 1 ] = '\0';
}

/*----------------------------------------------------------------------
 * reqGeneric - send a generic request to remote connection
 *----------------------------------------------------------------------
 */
reqGeneric( connection, class, host, code, reply )
     char * connection;
     char * class;
     char * host;
     int code;
     struct reply * reply;
{
	struct request req;
	struct reply rep;
	
	static int sock = -1;
	static char sockhost[ MAXHOSTNAME ];

	if( reply == NULL ) {
		reply = & rep;
	}
	
	initRequest( & req, code, connection, class );

	if( sock >= 0 && strequ( sockhost, host ) ) {
		/* do nothing */
	}
	else {
		if( sock >= 0 ) {
			close( sock );
		}
		
		strncpy( sockhost, host, MAXHOSTNAME );
		sockhost[ MAXHOSTNAME - 1 ] = '\0';
		if( ( sock = connectUDP( host, CS_PORT ) ) < 0 ) {
			return( -1 );
		}
	}
	if( sendRequest( sock, & req ) < 0 ) {
		return( -1 );
	}
	if( recvReply( sock, STIMEOUT, reply ) < 0 ) {
		return( -1 );
	}

	if( reply->code != RESP_OK ) {
		fprintf( stderr, "connecton to '%s', host '%s' failed: %s\n",
			connection, host, reply->details );
		return( -1 );
	}
	return( 0 );
}
