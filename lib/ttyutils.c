#include <stdio.h>
#include <ctype.h>
#include <sgtty.h>
#ifdef HPPA
#include <termios.h>
#endif
#ifndef linux
# include <sys/ttold.h>
#else
# include <rpcsvc/rex.h>
# ifndef tIOC
#  define tIOC    ('t'<<8)
#  define TIOCGETP        (tIOC|8)
#  define TIOCSETP        (tIOC|9)
#  define sg_flags flags
# endif
#endif
#include <sys/types.h>
#include <sys/file.h>
#include <sys/fcntl.h>

struct	baudent {
	char	*name;
	int	rate;
} baudlist[] = {
	{"0",		B0},
	{"50",		B50},
	{"75",		B75},
	{"110",		B110},
	{"134",		B134},
	{"134.5",	B134},
	{"150",		B150},
	{"200",		B200},
	{"300",		B300},
	{"600",		B600},
	{"1200",	B1200},
	{"1800",	B1800},
	{"2400",	B2400},
	{"4800",	B4800},
	{"9600",	B9600},
#ifndef linux
	{"exta",	EXTA},
	{"19200",	EXTA},
	{"extb",	EXTB},
	{"38400",	EXTB}
#else
	{"exta",	B19200},
	{"19200",	B19200},
	{"extb",	B38400},
	{"38400",	B38400}
#endif
};

#define NBAUD	(sizeof(baudlist)/sizeof(struct baudent))
#define NOBAUD	(-1)


static int
lookupBaudRate( baudName )
     char *baudName;
{
	int i;
	
	for( i = 0; i < NBAUD; i++ ) {
		if( ! strcmp( baudName, baudlist[i].name ) ) {
			return( i );
		}
	}
	return( NOBAUD );
}

static int
baudRate( index )
     int index;
{
	return( baudlist[ index ].rate );
}

/*---------------------------------------------------------------------------
 * OpenTTYLine - open tty line and initialize it
 *---------------------------------------------------------------------------
 */
int
OpenTTYLine( dev, baudrate )
     char * dev;
     char * baudrate;
{
	int devfd;
	struct sgttyb lttymode;
	
	if( ( devfd = open( dev, O_RDWR | O_NONBLOCK, 0 ) ) < 0 ) {
		return( -1 );
	}

	if( ioctl( devfd, TIOCGETP, & lttymode ) < 0 )  {
		return( -1 );
	}
	
	if( baudrate != NULL ) {
		int baudindex;
		
		baudindex = lookupBaudRate( baudrate );
		if( baudindex != NOBAUD ) {
#ifndef linux
			lttymode.sg_ospeed = lttymode.sg_ispeed =
				 baudRate( baudindex );
#else
			lttymode.chars[1] = lttymode.chars[0] =
				 baudRate( baudindex );
#endif
		}
	}
	
	lttymode.sg_flags |= RAW;
	lttymode.sg_flags &= ~ECHO;
	
	if( ioctl( devfd, TIOCSETP, & lttymode ) < 0 ) {
		return( -1 );
	}
	return( devfd );
}

/*---------------------------------------------------------------------------
 * SendTTYBreak - send a break down the tty line
 *---------------------------------------------------------------------------
 */
int
SendTTYBreak( fdtty )
     int fdtty;
{
#ifdef HPPA
	ioctl( fdtty, TCSBRK, NULL );
#else
	ioctl( fdtty, TIOCSBRK, NULL );
	sleep( 1 );
	ioctl( fdtty, TIOCCBRK, NULL );
#endif
	return( 0 );
}

