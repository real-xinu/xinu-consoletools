#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "cserver.h"

extern char * getenv();

/*---------------------------------------------------------------------------
 * getMinutes -
 *---------------------------------------------------------------------------
 */
static int
getMinutes( time )
     char * time;
{
	int hours, minutes, seconds;
	
	sscanf( time, "%d:%d:%d", & hours, & minutes, & seconds );
	if( hours > 0 ) {
		minutes = minutes + ( hours * 60 );
	}
	return( minutes );
}

/*---------------------------------------------------------------------------
 * reqTCPConnection - test if the correct connection and is available
 *---------------------------------------------------------------------------
 */
static int
reqTCPConnection( connection, class, host, steal, stat )
     char * connection;
     char * class;
     char * host;
     int steal;
     struct statusreplyData * stat;
{
	struct reply reply;
	
	if( ( ! steal && stat->active ) || ( steal && ! stat->active ) ) {
		return( -1 );
	}
	
	if( steal ) {
#ifdef RESERVETIME
		int rv;
		
		if( getMinutes( stat->contime ) < RESERVETIME  ) {
			return( -1 );
		}
		
		/* connection time exceeded */
		if( reqGeneric( stat->conname, stat->conclass, host,
				REQ_BREAKCONNECTION, NULL ) < 0 ) {
			fprintf( stderr, "error: could not break connection\n" );
			return( -1 );
		}
#else
		return( -1 );
#endif
	}
	
	if( reqGeneric( stat->conname, stat->conclass, host,
			REQ_MAKECONNECTION, & reply ) == 0 ) {
		if( connection != NULL ) {
			strcpy( connection, stat->conname );
		}
		if( class != NULL ) {
			strcpy( class, stat->conclass );
		}
		return( connectTCP( host, atoi( reply.details ) ) );
	}
	return( -1 );
}

/*---------------------------------------------------------------------------
 * getTCPConnection - 
 *---------------------------------------------------------------------------
 */
static int
getTCPConnection( connection, class, host, steal, replies, cnt )
     char * connection;
     char * class;
     char * host;
     int steal;
     struct reply * replies[];
     int cnt;
{
	int i;
	
	for( i = 0; i < cnt; i++ ) {
		struct statusreplyData * stat = (struct statusreplyData *)
		    ( replies[ i ]->details );
		char * h = replies[ i ]->hostname;
		int j = 0;
		int numc = atoi( replies[ i ]->numconnections );
		
		for( ; j < numc; j++, stat++ ) {
			int fd = reqTCPConnection( connection, class,
						   replies[ i ]->hostname,
						   steal, stat );

			if( fd >= 0 ) {
				if( host != NULL ) {
					strcpy( host, replies[ i ]->hostname );
				}
				return( fd );
			}
		}
	}
	return( -1 );
}

/*---------------------------------------------------------------------------
 * getdfltClass
 *---------------------------------------------------------------------------
 */
static  char *
getdfltClass( class, connection )
     char * class, * connection;
{
	char * sb;

	if( ( class != NULL ) && ( *class != '\0' ) ) {
		return( class );
	}
	if( ( connection != NULL ) && ( *connection != '\0' ) ) {
		return( "" );
	}
#ifdef ENVCLASSNAME
	if( ( sb = getenv( ENVCLASSNAME ) ) != NULL ) {
		if( *sb == '\0' ) {
			return( DEFAULTCLASSNAME );
		}
		return( sb );
	}
#endif
	return( DEFAULTCLASSNAME );
}

static int
totalAnswers( replies, cnt )
     struct reply * replies[];
     int cnt;
{
	int i = 0;
	int total = 0;
	
	for( ; i < cnt; i++ ) {
		struct statusreplyData * stat = (struct statusreplyData *)
		    ( replies[ i ]->details );
		total += atoi( replies[ i ]->numconnections );
	}
	return( total );
}

/*---------------------------------------------------------------------------
 * makeConnection - make a tcp connection to the specific connection
 *---------------------------------------------------------------------------
 */
int
makeConnection( connection, class, host )
     char * connection;
     char * class;
     char * host;
{
	int fd;
	int cnt;
	struct reply * replies[ MAXSERVERS ];
	char dclass[ MAXCLASSNAME ];
	
	if( ( host != NULL ) && ( *host != '\0' ) &&
	    ( connection != NULL ) && ( *connection != '\0' ) &&
	    ( class != NULL ) && ( *class != '\0' ) ) {
		struct reply reply;
		if( reqGeneric( connection, class, host, REQ_MAKECONNECTION,
				& reply ) == 0 ) {
			return( connectTCP( host, atoi( reply.details ) ) );
		}
		fprintf( stderr, "error: connection not available\n" );
		return( -1 );
	}
	else {
		if( class == NULL ) {
			strncpy( dclass, getdfltClass( class, connection ),
				 MAXCLASSNAME );
			class = dclass;
		}
		else {
			strncpy( class, getdfltClass( class, connection ),
				 MAXCLASSNAME );
		}
		class[ MAXCLASSNAME - 1 ] = '\0';
	
		cnt = obtainStatus( connection, class, host, replies,
				    MAXSERVERS );

		if( totalAnswers( replies, cnt ) <= 0 ) {
			fprintf( stderr, "error: connection not found\n" );
			return( -1 );
		}
		
		if( ( fd = getTCPConnection( connection, class, host, 0,
					     replies, cnt ) ) >= 0 ) {
			return( fd );
		}
		if( ( fd = getTCPConnection( connection, class, host, 1,
					     replies, cnt ) ) >= 0 ) {
			return( fd );
		}
		
		fprintf( stderr, "error: connection not available\n" );
		return( -1 );
	}
}
