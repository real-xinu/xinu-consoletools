/*
 * connectto.c - connect to the specified device
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>

typedef int (*FUNCT)();

#define MY_BUFSIZE 4096

static int
flushdata( devin, devout, funct, prog )
     int devin;
     int devout;
     FUNCT funct;
     char * prog;
{
	unsigned char buf[ MY_BUFSIZE ];
	int wlen;
	int rlen;

	if( ( rlen = read( devin, buf, sizeof( buf ) ) ) <= 0 ) {
#ifdef DEBUG
		fprintf( stderr, "%s: bad return from read(%d) = %d, errno = %d\n", prog, devin, rlen, errno );
#endif
		return( rlen );
	}
	if( ( wlen = funct( devout, buf, rlen ) ) != rlen ) {
		if( wlen > 0 && wlen < rlen ) {
#ifdef DEBUG
			fprintf( stderr, "%s: short write(%d)\n", prog, devout );
#endif
			return( -1 );
		}
		if( wlen <= 0 ) {
#ifdef DEBUG
			fprintf( stderr, "%s: bad return from write(%d) = %d, errno = %d\n", prog, devout, wlen, errno );
#endif
		}
		return( wlen );
	}
	return( 1 );
}

/*---------------------------------------------------------------------------
 * connectto - Take standard input and send it to connection process on
 *		frontend 
 *---------------------------------------------------------------------------
 */
void
connectto( dev, funct, prog, flushc, sdown )
     int dev;
     FUNCT funct;
     char * prog;
     int flushc;
     int sdown;
{
	int devin, devout;
#ifdef linux
	fd_set bmaskRead;
#else
	struct fd_set bmaskRead;
#endif
	int i;

	devin = fileno( stdin );
	devout = fileno( stdout );

	while( 1 ) {
		do {
			FD_ZERO( & bmaskRead );
			FD_SET( devin, & bmaskRead );
			FD_SET( dev, & bmaskRead );
		
			i = select( FD_SETSIZE, & bmaskRead, 0, 0, 0 );
		} while( i == -1 && errno == EINTR );
		if( i <= 0 ) {
			fprintf( stderr, "error: bad return from select\n" );
			return;
		}
		if( FD_ISSET( dev, & bmaskRead ) ) {
			if( flushdata( dev, devout, funct, prog ) <= 0 ) {
				return;
			}
		}
		if( FD_ISSET( devin, & bmaskRead ) ) {
			if( flushdata( devin, dev, funct, prog ) <= 0 ) {
				if( flushc == 0 ) {
					return;
				}
				break;
			}
		}
	}

	if( sdown ) {
		shutdown( dev, 1 ); /* no more sends on device */
	}
	
	while( 1 ) {
		if( flushc > 0 ) {
			if( ! readDelay( dev, flushc ) ) {
				break;
			}
		}
		if( flushdata( dev, devout, funct, prog ) <= 0 ) {
			return;
		}
	}
}
