#include <stdio.h>
#include <ctype.h>
#include <sgtty.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/fcntl.h>

struct  baudent {
  char		*name;
  speed_t	rate;
} baudlist[] = {
  {"0",      B0},
  {"50",     B50},
  {"75",     B75},
  {"110",    B110},
  {"134",    B134},
  {"134.5",  B134},
  {"150",    B150},
  {"200",    B200},
  {"300",    B300},
  {"600",    B600},
  {"1200",   B1200},
  {"1800",   B1800},
  {"2400",   B2400},
  {"4800",   B4800},
  {"9600",   B9600},
  {"exta",   B19200},
  {"19200",  B19200},
  {"extb",   B38400},
  {"38400",  B38400}
};

#define NBAUD  (sizeof(baudlist)/sizeof(struct baudent))
#define NOBAUD  (-1)


static int
lookupBaudRate( 
   char *baudName)
{
  int i;
  
  for( i = 0; i < NBAUD; i++ ) {
    if( ! strcmp( baudName, baudlist[i].name ) ) {
      return( i );
    }
  }
  return( NOBAUD );
}

static speed_t
baudRate( 
   int index)
{
  return( baudlist[ index ].rate );
}

/*---------------------------------------------------------------------------
 * OpenTTYLine - open tty line and initialize it
 *---------------------------------------------------------------------------
 */
int
OpenTTYLine( 
   char *dev, 
   char *baudrate)
{
  int devfd;
  struct termios t;
  
  if( ( devfd = open( dev, O_RDWR | O_NONBLOCK, 0 ) ) < 0 ) {
    return( -1 );
  }

  if ( tcgetattr( devfd, &t ) < 0 ) {
    return( -1 );
  }

  if( baudrate != NULL ) {
    int baudindex;
    
    baudindex = lookupBaudRate( baudrate );
    if( baudindex != NOBAUD ) {
      cfsetispeed( &t, baudRate( baudindex ) );
      cfsetospeed( &t, baudRate( baudindex ) );
    }
  }

  /* Set the terminal in RAW mode :  See p 354 "Advanced Programming */
  /* in the UNIX Environment" by W. Richard Stevens                  */
  t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  t.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  t.c_cflag &= ~(CSIZE | PARENB);
  t.c_cflag |= CS8;
  t.c_oflag &= ~(OPOST);
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;

  if ( tcsetattr( devfd, TCSAFLUSH, &t ) < 0 ) {
    return( -1 );
  }
  
  return( devfd );
}

/*---------------------------------------------------------------------------
 * SendTTYBreak - send a break down the tty line
 *---------------------------------------------------------------------------
 */
int
SendTTYBreak( 
   int fdtty )
{
  return tcsendbreak(fdtty, 0);
}

