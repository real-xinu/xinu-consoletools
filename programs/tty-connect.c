#include <stdio.h>
#include <errno.h>
#include <signal.h>

#ifdef DEC

#include <fcntl.h>

#else

#include <sys/fcntl.h>

#endif

#include <ctype.h>

static int flushdata();

static char * prog = 0;
static int g_dev = -1;

static void
quit( i )
    int i;
{
	restoretty( fileno( stdin ) );
	exit( 0 );
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
		sa.sa_handler = SIG_DFL;
		(void) sigaction( SIGALRM, & sa, 0 );
	}
#else
	signal( SIGQUIT, quit );
	signal( SIGINT, quit );
	signal( SIGTSTP, SIG_IGN );
	signal( SIGPIPE, SIG_IGN );
#endif
}

static void
printusage( sb )
     char * sb;
{
	fprintf( stderr, "usage: %s [-t] [-b] [-f] [-r baudrate] tty\n", sb );
	exit( 1 );
}

#define FLUSH_TIMEOUT 2

static int transparent = 0;

/*---------------------------------------------------------------------------
 * tty-connect - connect to tty at specified baudrate
 *---------------------------------------------------------------------------
 */
main( argc, argv )
     int argc;
     char **argv;
{
	char * ttyname = NULL;
	char * ttyrate = "9600";
	int i;
	int flushc;
	int sbrk;

	prog = argv[0];
	flushc = 0;
	sbrk = 0;

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
		else if( strcmp( argv[ i ], "-r" ) == 0 ) {
			i++;
			if( i >= argc ) {
				fprintf( stderr,
					"error: missing baudrate\n" );
				printusage( prog );
			}
			ttyrate = argv[ i ];
		}
		else if( argv[ i ][ 0 ] == '-' ) {
			fprintf( stderr, "error: unexpected argument '%s'\n",
				 argv[ i ] );
			printusage( prog );
		}
		else {
			ttyname = argv[ i ];
		}
	}
	if( ttyname == NULL ) {
		fprintf( stderr, "missing terminal name\n" );
		printusage( prog );
	}

	setSignals();

	if( isatty( fileno( stdin ) ) ) {
		if( cbreakmode( fileno( stdin ) ) != 0 ) {
			fprintf( stderr, "%s: cannot set terminal modes\n",
				 argv[ 0 ] );
			exit( 1 );
		}
	}

	if( ( g_dev = OpenTTYLine( ttyname, ttyrate ) ) < 0 ) {
		fprintf( stderr, "%s: cannot set tty line\n", argv[ 0 ] );
		exit( 1 );
	}

	if( sbrk ) {
		SendTTYBreak( g_dev );
		sleep( 1 );
	}
	else {
		connectto( g_dev, flushdata, prog, flushc, 0 );
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
	if( transparent ) {
		/* writing transparently */
		
		return( write( devout, buf, len ) );
	}
	else if( devout == fileno( stdout ) ) {
		/* writing to stdout */
		
		return( write( devout, buf, len ) );
	}
	else {
		/* writing to tty - process escaped commands */
		
		static int esc = 0;
	
		int e = 0;

		PutCharStart( devout ); 

		while( len-- > 0 ) {
		
			char ch = *buf++;

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
					SendTTYBreak( devout );
					e++;
					continue;
				}
				/* fall through and output character */
			}
			
			if( ! PutChar( ch ) ) {
				return( PutCharEnd() + e );
			}
		}
		return( PutCharEnd() + e );
	}
}
