#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "cserver.h"

/*---------------------------------------------------------------------------
 * breakConnection - 
 *---------------------------------------------------------------------------
 */
int
breakConnection( connection, class, host )
     char * connection;
     char * class;
     char * host;
{
	if( connection == NULL || connection[ 0 ] == '\0' ) {
		fprintf( stderr, "error: no connection specified\n" );
		return( -1 );
	}
	if( host == NULL || host[ 0 ] == '\0' ) {
		struct reply * replies[ MAXSERVERS ];
		int numservers = obtainStatus( connection, class, NULL,
					      replies, MAXSERVERS );
		int i;

		for( i = 0; i < numservers; i++ ) {
			int numc = atoi( replies[ i ]->numconnections );
			
			if( numc > 0 ) {
				return( reqGeneric( connection, class,
						    replies[ i ]->hostname,
						    REQ_BREAKCONNECTION,
						    NULL ) );
			}
		}
		fprintf( stderr, "error: no connection found\n" );
		return( -1 );
	}
	return( reqGeneric( connection, class, host, REQ_BREAKCONNECTION,
			   NULL ) );
}
