 #include <termios.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>

/* you really probably only need 1 entry for saved tty, since there is only */
/* one tty per process as far as I know.  But the older library assumed you */
/* could have up to 20! 						    */

#define NTTY	20
struct termios savedttys[NTTY];
int ttyissaved[NTTY];

void 
initttys()
{
  memset((void *)ttyissaved, 0, sizeof(ttyissaved));
}

void
rawtty(
   int fd)
{
struct termios t;

if ( ttyissaved[fd] )
  return;

  if ( (fd < 0) || ( fd >= NTTY ) )
    return;

  if ( tcgetattr(fd, savedttys + fd) < 0 )
    return; /* DIE? */

  t = savedttys[fd]; 

  t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  t.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  t.c_cflag &= ~(CSIZE | PARENB);
  t.c_cflag |= CS8;
  t.c_oflag &= ~(OPOST);
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;

  if ( tcsetattr(fd, TCSAFLUSH, &t) < 0 )
    return;		/* DIE? */
  
  ttyissaved[fd] = 1;
}


void
cbreakmode(
   int fd)
{
struct termios t;

if ( ttyissaved[fd] )
  return;

  if ( (fd < 0) || ( fd >= NTTY ) )
    return;

  if ( tcgetattr(fd, savedttys + fd) < 0 )
    return; /* DIE? */

  t = savedttys[fd]; 

  t.c_lflag &= ~(ECHO | ICANON);
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;

  if ( tcsetattr(fd, TCSAFLUSH, &t) < 0 )
    return;		/* DIE? */
  
  ttyissaved[fd] = 1;
}


/* run at beginning : atexit(tty_atexit); */
void
tty_atexit(void)
{
int i;
  for ( i = 0 ; i < NTTY ; ++i )
    restoretty(i);
}


void
restoretty(
   int fd)
{
  if ( ttyissaved[fd] )
    tcsetattr(fd, TCSAFLUSH, savedttys + fd);

  ttyissaved[fd] = 0;
}
