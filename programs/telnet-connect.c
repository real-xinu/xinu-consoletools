/* Copyright (c) 1990, 1991, 1992
   Vincent F. Russo
   Patrick A. Muckelbauer
   All rights reserved. 

This work was developed by the author(s) at Purdue University
during 1990-1992 under the direction of Professor Vincent Russo.
 
 Redistribution and use in source and binary forms are permitted provided that
 this entire copyright notice is duplicated in all such copies.  No charge,
 other than an "at-cost" distribution fee, may be charged for copies,
 derivations, or distributions of this material without the express written
 consent of the copyright holders. Neither the name of the University, nor the
 name of the author may be used to endorse or promote products derived from
 this material without specific prior written permission.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
MERCHANTIBILITY AND FITNESS FOR ANY PARTICULAR PURPOSE. */

#include <stdio.h>
#include <signal.h>

#ifdef HPPA

#define	IAC	255		/* interpret as command: */
#define	DONT	254		/* you are not to use option */
#define	DO	253		/* please, you use option */
#define	WONT	252		/* I won't use option */
#define	WILL	251		/* I will use option */
#define	BREAK	243		/* break */

/* telnet options */
#define TELOPT_BINARY	0	/* 8-bit data path */
#define	TELOPT_TTYPE	24	/* terminal type */

#else

#include <arpa/telnet.h>

#endif

static unsigned char noterm[] = { IAC, WONT, TELOPT_TTYPE };

static unsigned char dobinary[] = { IAC, DO, TELOPT_BINARY,
				    IAC, WILL, TELOPT_BINARY };

static unsigned char dobreak[] = { IAC, BREAK };

static int sock = -1;

static int flushdata();

static char * prog = 0;

static void
quit( i )
    int i;
{
	restoretty( fileno( stdin ) );
	if( sock >= 0 ) {
		shutdown( sock, 2 );
	}
	exit( 0 );
}

static void
sendBreak( i )
    int i;
{
	if( sock >= 0 ) {
		write( sock, dobreak, sizeof( dobreak ) );
	}
}


static
setSignals()
{
#ifdef SOLARIS
	{
		struct sigaction sa;
		
		sigemptyset( &(sa.sa_mask) );
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = quit;
		(void) sigaction( SIGQUIT, & sa, 0 );
		(void) sigaction( SIGINT, & sa, 0 );
		sa.sa_handler = SIG_IGN;
		(void) sigaction( SIGTSTP, & sa, 0 );
		(void) sigaction( SIGPIPE, & sa, 0 );
		sa.sa_handler = sendBreak;
		(void) sigaction( SIGUSR1, & sa, 0 );
	}
#else
	signal( SIGQUIT, quit );
	signal( SIGINT, quit );
	signal( SIGTSTP, SIG_IGN );
	signal( SIGPIPE, SIG_IGN );
	signal( SIGUSR1, sendBreak );
#endif
}

static void
printusage( sb )
     char * sb;
{
	fprintf( stderr, "usage: %s [-t] [-b] [-f] host port\n", sb );
	exit( 1 );
}

#define FLUSH_TIMEOUT 2

static int transparent = 0;

/*---------------------------------------------------------------------------
 * telnet-connect - connect to telnet server in binary mode
 *---------------------------------------------------------------------------
 */
main( argc, argv )
     int argc;
     char * argv[];
{
	int i;
	int flushc;
	int sbrk;
	char * host;
	char * port;

	prog = argv[0];
	flushc = 0;
	sbrk = 0;
	host = 0;
	port = 0;

	for( i = 1; i < argc; i++ ) {
		if( strcmp( argv[ i ], "-t" ) == 0 ) {
			transparent = 1;
		}
		else if( strcmp( argv[ i ], "-b" ) == 0 ) {
			sbrk = 1;
		}
		else if( strcmp( argv[ i ], "-f" ) == 0 ) {
			flushc = FLUSH_TIMEOUT;
		}
		else if( argv[ i ][ 0 ] == '-' ) {
			fprintf( stderr, "error: unexpected argument '%s'\n",
				 argv[ i ] );
			printusage( prog );
		}
		else if( (i + 1) >= argc ) {
			fprintf( stderr, "error: missing argument\n" );
			printusage( prog );
		}
		else {
			host = argv[ i ];
			port = argv[ i + 1 ];
			i++;
		}
	}

	setSignals();

	if( ( sock = connectTCP( host, atoi( port ) ) ) < 0 ) {
		exit( 1 );
	}

	if( isatty( fileno( stdin ) ) ) {
		if( cbreakmode( fileno( stdin ) ) != 0 ) {
			fprintf( stderr, "%s: cannot set terminal modes\n",
				 prog );
			exit( 1 );
		}
	}

	write( sock, (char *) noterm, sizeof( noterm ) );

	write( sock, (char *) dobinary, sizeof( dobinary ) );

	if( sbrk ) {
		sendBreak( 0 );
		sleep( 1 );
	}
	else {
		connectto( sock, flushdata, prog, flushc, 0 );
	}

	quit( 0 );
}

#define PC_LEN 1024

static int pc_dev = -1;
static int pc_rv = 0;
static int pc_len = 0;
static char pc_buf[ PC_LEN ];

static void
PutCharStart( dev )
     int dev;
{
	pc_dev = dev;
	pc_rv = 0;
	pc_len = 0;
}

static int
PutCharFlush()
{
	if( pc_len > 0 ) {
		int rv = write( pc_dev, pc_buf, pc_len );
		if( rv <= 0 ) {
			pc_rv -= pc_len;
			return( 0 );
		}
		else if( rv < pc_len ) {
			pc_rv -= (pc_len - rv);
			return( 0 );
		}
	}
	pc_len = 0;
	return( 1 );
}

static int
PutChar( ch )
     char ch;
{
	if( pc_len >= PC_LEN ) {
		if( ! PutCharFlush() ) {
			return( 0 );
		}
	}
	pc_buf[ pc_len ] = ch;
	pc_len++;
	pc_rv++;
	return( 1 );
}

static int
PutCharEnd()
{
	int rv;
	
	PutCharFlush();

	rv = pc_rv;
	
	pc_dev = -1;
	pc_rv = 0;
	
	return( rv );
}

static int
flushdata( devout, buf, len )
     int devout;
     char * buf;
     int len;
{
	if( devout == fileno( stdout ) ) {
		/* writing to stdout */
		
		static int state = 0;

		int e = 0;

		PutCharStart( devout );

		while( len-- > 0 ) {
			char ch = *buf++;
			    
			if( state == 0 ) {
				if( ((unsigned char) ch) == IAC ) {
					state = 1;
					e++;
				}
				else {
					if( ! PutChar( ch ) ) {
						return( PutCharEnd() + e );
					}
				}
			}
			else if( state == 1 ) {
				switch( ((unsigned char) ch) ) {
				case IAC:
					if( ! PutChar( ch ) ) {
						return( PutCharEnd() + e );
					}
				default:
					state = 0;
					e++;
					break;
				case DO:
				case DONT:
				case WILL:
				case WONT:
					state = 2;
					e++;
					break;
				}
			}
			else if ( state == 2 ) {
				/* do nothing with option */
				state = 0;
				e++;
			}
		}
		
		return( PutCharEnd() + e );
	}
	else {
		/* writing to network */

		static int esc = 0;
		
		int e = 0;

		PutCharStart( devout );

		while( len-- > 0 ) {
			char ch = *buf++;

			if( ! transparent ) {
				/* process escaped commands */
				
				if( ! esc ) {
					if( ch == '\\' ) {
						esc = 1;
						e++;
						continue;
					}
				}
				else if( esc ) {
					esc = 0;
					if( ch == 'b' ) {
						sendBreak( 0 );
						e++;
						continue;
					}
					/* fall through and output character */
				}
			}
			
			if( ! PutChar( ch ) ) {
				return( PutCharEnd() + e );
			}
			if( ((unsigned char ) ch) == IAC ) {
				e--;
				if( ! PutChar( ch ) ) {
					return( PutCharEnd() + e );
				}
			}
		}
		return( PutCharEnd() + e );
	}
}
