#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <netdb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifndef linux
#include <sys/sockio.h>
#endif

#include <sys/ioctl.h>
#include <sys/file.h>

#include <netinet/in.h>

#include <net/if.h>

struct	in_addr inet_makeaddr();

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#ifndef linux
extern char * sys_errlist[];
#endif

extern u_long inet_addr();

/*----------------------------------------------------------------------------
 * readTCP - read tcp connect and write to stdout
 *----------------------------------------------------------------------------
 */
int
readTCP( devtcp )
     int devtcp;
{
	char buff[ 1024 ];/* big buffer to read into and out of  */
	int len;
	
	for(;;) {
		if( ( len = read( devtcp, buff, sizeof( buff ) ) ) <= 0 ) {
			if( len < 0 ) {
				fprintf( stderr, "bad read from tcp connection\n" );
				return( -1 );
			}
			break;
		}
		write( 1, buff, len );
	}
	return( 0 );
}

/*----------------------------------------------------------------------------
 * connectUDP - connect to a specified UDP service on a specified host
 *----------------------------------------------------------------------------
 */
int
connectUDP( host, service )
     char *host;	     /* name of host to which connection is desired  */
     int service;	    /* service associated with desired port	 */
{
	return( connectsock( host, service, "udp" ) );
}

/*----------------------------------------------------------------------------
 * connectTCP - connect to a specified TCP service on a specified host
 *----------------------------------------------------------------------------
 */
int
connectTCP( host, service )
     char *host;	     /* name of host to which connection is desired  */
     int service;	    /* service associated with desired port	 */
{
	return( connectsock( host, service, "tcp" ) );
}

/*----------------------------------------------------------------------------
 * passiveUDP - create a passive socket for use in a UDP server
 *----------------------------------------------------------------------------
 */
int
passiveUDP( service )
     int service;	    /* service associated with desired port	 */

{
   return( passivesock( service, "udp", 0 ) );
}

/*----------------------------------------------------------------------------
 * passiveTCP - create a passive socket for use in a TCP server
 *----------------------------------------------------------------------------
 */
int
passiveTCP( service, qlen )
     int service;	    /* service associated with desired port	 */
     int  qlen;		     /* maximum length of the server request queue   */
{
   return( passivesock( service, "tcp", qlen ) );
}

/*---------------------------------------------------------------------------
 * acceptTCP() -- return a socket to a newly accepted connection
 *---------------------------------------------------------------------------
 */
int
acceptTCP( s )
     int s;			   /* passive socket descriptor	 	     */
{
	int nc;			   /* the new socket descriptor 	     */
	struct sockaddr_in sin;    /* an Internet endpoint address	   */
	int len;		   /* Internet address length 		     */

	len = sizeof( struct sockaddr_in ); 
	if( ( nc = accept( s, (struct sockaddr *) & sin, & len ) ) < 0 ) {
		fprintf( stderr, "can't accept new connection: %s\n",
		    sys_errlist[ errno ] );
		return( -1 );
	}
	return( nc );
}

/*---------------------------------------------------------------------------
 * sockgetstr - get str from socket - return NULL if error
 *---------------------------------------------------------------------------
 */
char *
sockgetstr( s, str, timeout )
     int s;
     char *str;
     int timeout;
{
	char *sb;
	
	sb = str;
	do {
		if( ! readDelay( s, timeout ) ) {
			return( NULL );
		}
		if( recv( s, sb, 1, 0 ) < 1 ) {
			return( NULL );
		}
	} while( *sb++ != '\0' );
	return( str );
}

/*----------------------------------------------------------------------------
 * connectsock - allocate & connect a socket using TCP or UDP
 *----------------------------------------------------------------------------
 */
int
connectsock( host, service, protocol )
     char *host;	     /* name of host to which connection is desired  */
     int   service;	  /* service associated with desired port	 */
     char *protocol;	 /* name of protocol to use ("tcp" or "udp" )    */
{
	struct hostent    *phe;    /* pointer to host information entry      */
	struct protoent   *ppe;    /* pointer to protocol information entry  */
	struct sockaddr_in sin;    /* an Internet endpoint address	   */
	int	       s, type; /* socket descriptor and socket type      */

	bzero( (char *)&sin , sizeof( sin ) );
	sin.sin_family = AF_INET;
	sin.sin_port = htons( (u_short) service );
		
	/* Map host name to IP address, allowing for dotted decimal */
	if( phe = gethostbyname( host ) ) {
		bcopy( phe->h_addr, (char *)&sin.sin_addr, phe->h_length );
	}
	else if( ( sin.sin_addr.s_addr = inet_addr( host ) ) == INADDR_NONE ) {
		fprintf( stderr,"connectsock(%s, %d, %s): can't get host entry\n", host, service, protocol );
		return( -1 );
	}

	/* Map protocol name to port number */
	if( ( ppe = getprotobyname( protocol ) ) == 0 ) {
		fprintf( stderr,"connectsock(%s, %d, %s): can't get protocol entry\n", host, service, protocol );
		return( -1 );
	}

	/* Use protocol to choose a socket type */
	if( strcmp( protocol, "udp" ) == 0 ) {
		type = SOCK_DGRAM;
	}
	else {
		type = SOCK_STREAM;
	}

	/* Allocate a socket */
	if( ( s = socket( PF_INET, type, ppe->p_proto ) ) < 0 ) {
		fprintf( stderr,"connectsock(%s, %d, %s): can't get create socket: %s\n", host, service, protocol, sys_errlist[ errno ] );
		return( -1 );
	}

	/* Connect the socket */
	if( connect( s, (struct sockaddr *)&sin, sizeof( sin ) ) < 0 ) {
		fprintf( stderr,"connectsock(%s, %d, %s): can't connect to port: %s\n", host, service, protocol, sys_errlist[ errno ] );
		return( -1 );
	}
	return( s );
}


/*----------------------------------------------------------------------------
 * passivesock - allocate & bind a server socket using TCP or UDP
 *----------------------------------------------------------------------------
 */
int
passivesock( service, protocol, qlen )
     int   service;	  /* service associated with desired port	 */
     char *protocol;	 /* name of protocol to use ("tcp" or "udp" )    */
     int   qlen;
{
	struct protoent   *ppe;    /* pointer to protocol information entry  */
	struct sockaddr_in sin;    /* an Internet endpoint address	   */
	int	       s, type; /* socket descriptor and socket type      */

	bzero( (char *)&sin , sizeof( sin ) );
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons( (u_short) service );

	/* Map protocol name to port number */
	if( ( ppe = getprotobyname( protocol ) ) == 0 ) {
		fprintf( stderr, "passivesock(%d, %s, %d): can't get protocol entry\n", service, protocol, qlen );
		return( -1 );
	}

	/* Use protocol to choose a socket type */
	if( strcmp( protocol, "udp" ) == 0 ) {
		type = SOCK_DGRAM;
	}
	else {
		type = SOCK_STREAM;
	}

	/* Allocate a socket */
	s = socket( PF_INET, type, ppe->p_proto );
	if( s < 0 ) {
		fprintf( stderr, "passivesock(%d, %s, %d): can't create socket: %s\n", service, protocol, qlen, sys_errlist[ errno ] );
		return( -1 );
	}

	/* Bind the socket */
	if( bind( s, (struct sockaddr *)&sin, sizeof( sin ) ) < 0 ) {
		fprintf( stderr, "passivesock(%d, %s, %d): can't bind to port: %s\n", service, protocol, qlen, sys_errlist[ errno ] );
		return( -1 );
	}
	if( type == SOCK_STREAM && ( listen( s, qlen ) < 0 ) ) {
		fprintf( stderr, "passivesock(%d, %s, %d): can't listen on port: %s\n", service, protocol, qlen, sys_errlist[ errno ] );
		return( -1 );
	}
	return( s );
}

/*---------------------------------------------------------------------------
 * bnets - get all the broadcast networks
 *---------------------------------------------------------------------------
 */
static int
bnets( addrs, s )
	struct in_addr *addrs;
	int s;
{
	struct ifconf ifc;
	struct ifreq ifreq, *ifr;
	struct sockaddr_in *sin;
	int n, i;
	char buf[ 2000 ];
	
	ifc.ifc_len = sizeof( buf );
	ifc.ifc_buf = buf;
	if( ioctl( s, SIOCGIFCONF, (char *) & ifc ) < 0 ) {
		return( 0 );
	}
	ifr = ifc.ifc_req;
	n = ifc.ifc_len/sizeof( struct ifreq );
	for( i = 0; n > 0; n--, ifr++ ) {
		ifreq = *ifr;
		if( ioctl( s, SIOCGIFFLAGS, (char *) &ifreq ) < 0 ) {
			continue;
		}
		if( ( ifreq.ifr_flags & IFF_BROADCAST ) &&
		    ( ifreq.ifr_flags & IFF_UP ) &&
		    ifr->ifr_addr.sa_family == AF_INET ) {
			sin = (struct sockaddr_in *) & ifr->ifr_addr;
			if( ioctl( s, SIOCGIFBRDADDR, (char *)&ifreq ) < 0 ) {
				addrs[i++] = inet_makeaddr(inet_netof
					(sin->sin_addr.s_addr), INADDR_ANY);
			}
			else {
				addrs[i++] = ((struct sockaddr_in*)
						&ifreq.ifr_addr)->sin_addr;
			}
		}
	}
	return( i );
}

/*---------------------------------------------------------------------------
 * bcastUDP - send a broadcast UDP packet
 *---------------------------------------------------------------------------
 */
int
bcastUDP( s, buf, len , port )
     int s;
     char * buf;
     int len;
     int port;
{
	struct in_addr addrs[ 20 ];
	struct sockaddr_in baddr; /* broadcast and response addresses */
	int nets;
	int i;
	int on = 1;

	/* set socket options so we can broadcast the IP addr back */
	if( setsockopt( s , SOL_SOCKET , SO_BROADCAST ,
			(char *) & on , sizeof( on ) ) < 0 ) {
		fprintf( stderr, "broadcast failed setting socket options: %s\n", sys_errlist[ errno ] );
		return( -1 );
	}
	nets = bnets( addrs, s );
	bzero( (char *)&baddr, sizeof( baddr ) );
	baddr.sin_family = AF_INET;
	baddr.sin_port = htons( port );
#ifdef linux
	baddr.sin_addr.s_addr = htonl( INADDR_ANY );
#else
	baddr.sin_addr.S_un.S_addr = htonl( INADDR_ANY );
#endif
	for( i = 0; i < nets; i++ ) {
		baddr.sin_addr = addrs[ i ];
		if( sendto( s, buf, len, 0, (struct sockaddr *) & baddr,
			    sizeof( struct sockaddr_in ) ) != len ) {
			fprintf( stderr, "broadcast failed: %s\n",
				sys_errlist[ errno ] );
			return( -1 );
		}
	}
	return( i );
}

/*---------------------------------------------------------------------------
 * readDelay - delay until a read is available (or timeout)
 *---------------------------------------------------------------------------
 */
int
readDelay( fd, timeout )
     int fd, timeout;
{
	if( timeout >= 0 ) {
#ifdef linux
		fd_set reads;
#else
		struct fd_set reads;
#endif
		struct timeval tv;
		int rv;

		FD_ZERO( & reads );
		FD_SET( fd, & reads );
		
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		do {
			rv = select( fd + 1, &reads, 0, 0, & tv );
		} while( ( rv == -1 ) && ( errno == EINTR ) );

		if( rv != 1 ) {
			return( 0 );
		}
	}
	return( 1 );
}

/*---------------------------------------------------------------------------
 * writeDelay - delay until a write is available (or timeout)
 *---------------------------------------------------------------------------
 */
int
writeDelay( fd, timeout )
     int fd, timeout;
{
	if( timeout >= 0 ) {
#ifdef linux
		fd_set writes;
#else
		struct fd_set writes;
#endif
		struct timeval tv;
		int rv;

		FD_ZERO( & writes );
		FD_SET( fd, & writes );
		
		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		do {
			rv = select( fd + 1, 0, & writes, 0, & tv );
		} while( ( rv == -1 ) && ( errno == EINTR ) );

		if( rv != 1 ) {
			return( 0 );
		}
	}
	return( 1 );
}

