#include <stdio.h>
#include <sgtty.h>
#include <sys/ioctl.h>
#ifndef CBREAK
# ifndef linux
#  include <sys/ttold.h>
#  define CBREAK O_CBREAK
# else
#  include <rpcsvc/rex.h>
#  ifndef tIOC
#   define tIOC    ('t'<<8)
#   define TIOCGETP        (tIOC|8)
#   define TIOCSETP        (tIOC|9)
#   define sg_flags flags
#  endif
# endif
#endif

#ifdef HPPA

#include <termios.h>

#endif

#define initializedTTY()	( g_devsize > 0 )
#define baddevice(dev)		( dev < 0 || dev >= g_devsize )

static struct sgttyb * g_kttymode = NULL; /* keyboard mode upon entry 	*/
static int * g_saved = NULL;
static int * g_mode = NULL;
static int g_devsize = 0;

#define MODE_ORIG	0
#define MODE_RAW	1
#define MODE_CBREAK	2

/*---------------------------------------------------------------------------
 * inittty - initialize global variables
 *---------------------------------------------------------------------------
 */
static void
inittty()
{
	int i;

	if( g_devsize > 0 ) {
		return;
	}

	g_devsize = getdtablesize();
	
	if( g_devsize <= 0 ) {
		return;
	}
	g_saved = (int *) xmalloc( sizeof( int ) * g_devsize );
	g_mode = (int *) xmalloc( sizeof( int ) * g_devsize );
	g_kttymode = (struct sgttyb *) xmalloc( sizeof(struct sgttyb)
						* g_devsize );
	for( i = 0; i < g_devsize; i++ ) {
		g_saved[ i ] = 0;
		g_mode[ i ] = MODE_ORIG;
	}
}

/*---------------------------------------------------------------------------
 * savetty - save the state of the tty
 *---------------------------------------------------------------------------
 */
static int
savetty( dev )
     int dev;
{
	if( baddevice( dev ) ) {
		return( -1 );
	}
	if( g_saved[ dev ] ) {
		return( 0 );
	}
	if( ioctl( dev, TIOCGETP, & g_kttymode[ dev ] ) < 0 )  {
		return( -1 );
	}
	g_saved[ dev ] = 1;
	return( 0 );
}

/*---------------------------------------------------------------------------
 * rawtty - place the tty line into raw mode
 *---------------------------------------------------------------------------
 */
int
rawtty( dev )
     int dev;
{
	struct sgttyb cttymode;	/* changed keyboard mode	*/
	
	inittty();

	if( baddevice( dev ) ) {
		return( -1 );
	}
	if( g_mode[ dev ] == MODE_RAW ) {
		return( 0 );
	}
	if( ! g_saved[ dev ] ) {
		savetty( dev );
	}
	if( ! g_saved[ dev ] ) {
		return( -1 );
	}
	cttymode = g_kttymode[dev];
	cttymode.sg_flags |= RAW;
	cttymode.sg_flags &= ~ECHO;
	cttymode.sg_flags &= ~CRMOD;
	if( ioctl( dev, TIOCSETP, & cttymode ) < 0 ) {
		return( -1 );
	}
	g_mode[ dev ] = MODE_RAW;
	return( 0 );
}

/*---------------------------------------------------------------------------
 * restoretty - restore the tty line to its original state
 *---------------------------------------------------------------------------
 */
int
restoretty( dev )
     int dev;
{
	inittty();

	if( baddevice( dev ) ) {
		return( -1 );
	}
	if( g_mode[ dev ] == MODE_ORIG ) {
		return( 0 );
	}
	if( ! g_saved[ dev ] ) {
		return( -1 );
	}
	if( ioctl( dev, TIOCSETP, & g_kttymode[ dev ] ) < 0 ) {
		return( -1 );
	}
	g_mode[ dev ] = MODE_ORIG;
	return( 0 );
}

/*---------------------------------------------------------------------------
 * rawtty - place the tty line into cbreak mode
 *---------------------------------------------------------------------------
 */
int
cbreakmode( dev )
	int dev;
{
#ifdef HPPA
	struct termios kttymode;
#else
	struct sgttyb kttymode;
#endif

	inittty();

	if( baddevice( dev ) ) {
		return( -1 );
	}
	if( g_mode[ dev ] == MODE_CBREAK ) {
		return( 0 );
	}
	if( ! g_saved[ dev ] ) {
		savetty( dev );
	}
	if( ! g_saved[ dev ] ) {
		return( -1 );
	}

#ifdef HPPA
	if( tcgetattr( 0, & kttymode ) < 0 ) {
#else
	if( ioctl( 0, TIOCGETP, & kttymode ) < 0 ) {
#endif
		return( -1 );
	}

	/*
         * set up local terminal in cbreak mode
	 */
#ifdef HPPA
	kttymode.c_lflag &= ~ICANON;
	kttymode.c_lflag &= ~ECHO;
	kttymode.c_iflag &= ~ICRNL;
	kttymode.c_oflag &= ~ONLCR;
	kttymode.c_cc[VEOF] = 1; /* MIN */
	kttymode.c_cc[VEOL] = 0; /* TIME */
#else
	kttymode.sg_flags |= CBREAK;
	kttymode.sg_flags &= ~ECHO;
	kttymode.sg_flags &= ~CRMOD;
#endif

#ifdef HPPA
	if( tcsetattr( 0, & kttymode ) < 0 ) {
#else
	if( ioctl( 0, TIOCSETP, & kttymode) < 0 ) {
#endif
		return( -1 );
	}

	g_mode[ dev ] = MODE_CBREAK;
	return( 0 );
}
