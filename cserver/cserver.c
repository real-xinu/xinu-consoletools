/* 
 * connectserver.c - Connect Server Process for distributed connections
 */

#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/fcntl.h>
#include <pwd.h>
#include <signal.h>

#ifdef WITH_LIBWRAP
#include WITH_LIBWRAP
extern int hosts_ctl();                 /* TCP wrappers.        */
int allow_severity = 0;
int deny_severity  = 0;
#endif

extern struct passwd * getpwuid();	/* get password entry	*/

#include "cserver.h"
#include "timeout.h"

#ifdef CS_GROUP

#include <sys/param.h>
#include <grp.h>

#ifndef NGROUPS
#define NGROUPS NGROUPS_UMAX
#endif

extern struct group * getgrnam();
extern struct group * getgrgid();

#endif

static int sendtoReply();

/* Defaults Parameters */
static int verbose = 0;
static int logfile = 1;
static int logusers = 1;
static int background = 1;
static int dumppid = 0;

/* Global Variables */
static int sock = -1;				/* server socket	*/
static struct condata * config = NULL;		/* config data structure*/
static int numconnections = 0;			/* number of connections*/
static char hostname[ MAXHOSTNAME ];		/* name of this host	*/
static char * exec_path = 0;			/* server path 		*/
static char ** exec_args = 0;			/* server arguments	*/

#ifdef WITH_LIBWRAP
static char *progname;                          /* Name of this server. */
#endif

extern struct condata * readConfigurationFile();
extern struct condata * getcondata();

static void findDeadChildren();
static ProcessRequest();
static Status();
static CleanUp();
static MakeConnection();

/*---------------------------------------------------------------------------
 * Reboot - restart the server
 *---------------------------------------------------------------------------
 */
static void
Reboot( i )
    int i;
{
	CleanUp();
	execv( exec_path, exec_args );
	FLog( stderr, "execv(server) failed" );
	exit( 1 );
}

/*---------------------------------------------------------------------------
 * Quit - kill backend daemon
 *---------------------------------------------------------------------------
 */
static void
Quit( i )
    int i;
{
	CleanUp();
	Log( "server exiting" );
	exit( 0 );
}

static void
printusage( sb )
     char * sb;
{
	FLog( stderr, "%s: [-p] [-v] [-bg] [-fg] [-lf] [-lt] [[-/+]ul] [config-file]",
	      sb );
	exit( 1 );
}

static
setSignals()
{
#ifdef SOLARIS
	{
		struct sigaction sa;
		
		sigemptyset( &(sa.sa_mask) );
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = Quit;
		(void) sigaction( SIGQUIT, & sa, 0 );
		(void) sigaction( SIGINT, & sa, 0 );
		sa.sa_handler = Reboot;
		(void) sigaction( SIGHUP, & sa, 0 );
		sa.sa_handler = SIG_DFL;
		(void) sigaction( SIGCHLD, & sa, 0 );
	}
#else
	signal( SIGINT, Quit );
	signal( SIGQUIT, Quit );
	signal( SIGHUP, Reboot );
#endif
}

main( argc, argv )
     int argc;
     char * argv[];
{
	int i;			/* counter variable		*/
	char * configurationFile = CS_CONFIGURATIONFILE;
	char pwname[ 32 ];

#ifdef WITH_LIBWRAP
	/* Get the name we were invoked with, for TCP wrappers. */
	progname = strrchr (argv[0], '/');
	if (progname == (char *) NULL)
	  progname = argv[0];
	else
	  progname++;
#endif

	umask( 0 );

	pwname[ sizeof( pwname ) - 1 ] = '\0';
	pwname[ 0 ] = '\0';
	{
		struct passwd * pw;
	
		if( ( pw = getpwuid( geteuid() ) ) == NULL ) {
			perror( "getpwuid()" );
			exit( 1 );
		}
		strncpy( pwname, pw->pw_name, sizeof( pwname ) - 1 );
	}

#ifdef CS_RUN_AS_ROOT
	if( strequ( pwname, "brylow" ) ) {
		/* ok */
	}
	else if( ! strequ( pwname, "root" ) ) {
		FLog( stderr, "must be 'root': running as '%s'", pwname );
		exit( 1 );
	}
#endif

#ifdef CS_GROUP
	if( strcmp( pwname, "root" ) == 0 ) {
		int gid;
		struct group * gent;
		struct passwd * pw;

		if( ( gent = getgrnam( CS_GROUP ) ) == NULL ) {
			perror( "getgrnam()" );
			exit( 1 );
		}
		if( setgid( gent->gr_gid ) != 0 ) {
			perror( "setgid()" );
			exit( 1 );
		}
		if( ( pw = getpwnam( "nobody" ) ) == NULL ) {
			perror( "getpwnam()" );
			exit( 1 );
		}
		if( setuid( pw->pw_uid ) != 0 ) {
			perror( "setuid()" );
			exit( 1 );
		}
	}
	else {
		int groups[ NGROUPS ];
		int ngroups, i;
		struct group * gent;
		
		if( ( gent = getgrnam( CS_GROUP ) ) == NULL ) {
			perror( "getgrnam()" );
			exit( 1 );
		}
		
		ngroups = getgroups( NGROUPS, groups );

		for( i = 0; i < ngroups; i++ ) {
			struct group * gr;

			if( ( gr = getgrgid( groups[ i ] ) ) == NULL ) {
				perror( "getgrgid()" );
				exit( 1 );
			}
			if( gent->gr_gid == gr->gr_gid ) {
				break;
			}
		}
		if( i >= ngroups ) {
			FLog( stderr, "must be in group '%s'", CS_GROUP );
			exit( 1 );
		}
	}
#endif
	
	exec_path = argv[ 0 ];
	exec_args = argv;
	for( i = 1; i < argc; i++ ) {
		if( strequ( argv[ i ], "-h" ) ) {
			printusage( argv[ 0 ] );
		}
		else if( strequ( argv[ i ], "-v" ) ) {
			verbose = 1;
		}
		else if( strequ( argv[ i ], "-bg" ) ) {
			background = 1;
		}
		else if( strequ( argv[ i ], "-fg" ) ) {
			background = 0;
		}
		else if( strequ( argv[ i ], " -lf " ) ) {
			logfile = 1;
		}
		else if( strequ( argv[ i ], "-lt" ) ) {
			logfile = 0;
		}
		else if( strequ( argv[ i ], "+ul" ) ) {
			logusers = 1;
		}
		else if( strequ( argv[ i ], "-ul" ) ) {
			logusers = 0;
		}
		else if( strequ( argv[ i ], "-p" ) ) {
			dumppid = 1;
		}
		else if( argv[ i ][ 0 ] == '-' ) {
			FLog( stderr, "unexpected argument '%s'", argv[ i ] );
			printusage( argv[ 0 ] );
		}
		else {
			configurationFile = argv[ i ];
		}
	}
	
	if( background ) {
		int fd;
		
		if( fork() > 0 ) {
			exit( 0 );
		}
		
		/* disassociate the server from the invoker's terminal	*/
		close( 0 );
		if( ( fd = open( "/", O_RDONLY ) ) != 0 ) {
			dup2( fd, 0 );
			close( fd );
		}
#if 0
#ifndef HPPA
		if( ( fd = open( "/dev/tty", O_RDWR ) ) >= 0 ) {
			ioctl( fd, TIOCNOTTY, 0 );
			close( fd );
		}
#endif
#endif
	}

	if( dumppid ) {
		char pid[32];
		fprintf( stdout, "%d\n", getpid() );
		fflush( stdout );
	}

	if( gethostname( hostname, MAXHOSTNAME ) != 0 ) {
		perror( "gethostname()" );
		exit( 1 );
	}
	hostname[ MAXHOSTNAME - 1 ] = '\0';
	
	if( logfile ) {
		char logfilename[ 256 ];
		int fd;

		strcpy( logfilename, CS_LOGGINGDIR );

		if( logfilename[ strlen( logfilename ) - 1 ] != '/' ) {
			logfilename[ strlen( logfilename ) ] = '/';
			logfilename[ strlen( logfilename ) + 1 ] = '\0';
		}
		strcat( logfilename, hostname );
		
		if( ( fd = open( logfilename, O_CREAT | O_APPEND | O_WRONLY,
				 0664 ) ) < 0) {
			perror( "open()" );
			exit( 1 );
		}

		close( 1 );
		close( 2 );
		dup2( fd, 1 );
		dup2( fd, 2 );
		close( fd );
	}

	if( verbose ) {
		Log( "reading configuration file '%s'", configurationFile );
	}
	if( ( config = readConfigurationFile( configurationFile ) ) == NULL ){
		if( verbose ) {
			Log( "no connections found in configuration file" );
		}
		exit( 0 );
	}
	{
		struct condata * curr = config;
		numconnections = 0;
		while( curr != NULL ) {
			numconnections++;
			curr = curr->next;
		}
	}

	Log( "cserver started by: %s", pwname );

	setSignals();

	if( ( sock = passiveUDP( CS_PORT ) ) < 0 ) {
		FLog( stderr, "cannot open well known port" );
		exit( 1 );
	}
	for(;;) {
		struct sockaddr_in sa;
		struct request req;
		
		if( verbose ) {
			Log("waiting for request");
		}
		if( recvfromRequest( sock, & sa, XLTIMEOUT, & req ) > 0 ){
			findDeadChildren();
			ProcessRequest( & sa, & req );
		}
		else {
			findDeadChildren();
		}
	}
}

/*---------------------------------------------------------------------------
 * ServerReplyMesg -- build and send reply mesg
 *---------------------------------------------------------------------------
 */
static 
ServerReplyMesg( psa, resp, mesg )
     struct sockaddr_in * psa;
     int resp;
     char * mesg;
{
	struct reply reply;
	int len = 0;

	initReply( & reply, resp, hostname, 0 );

	if( mesg == NULL ) {
		len = 1;
		reply.details[ 0 ] = '\0';
	}
	else {
		len += strlen( mesg ) + 1;
		
		strcpy( reply.details, mesg );
	}

	sendtoReply( sock, psa, & reply, len );
}

#define MAXMESG 128

/*---------------------------------------------------------------------------
 * ProcessRequest - process the network request
 *---------------------------------------------------------------------------
 */
static 
ProcessRequest ( psa, req )
     struct sockaddr_in * psa;
     struct request * req;
{
	char mesg[ MAXMESG ];
	struct condata * cdata;
	int tcp_wrap_result;

#ifdef WITH_LIBWRAP
	/* Ask TCP wrappers if this source address is OK. */
	tcp_wrap_result = hosts_ctl("cserver", STRING_UNKNOWN,
				    inet_ntoa (psa->sin_addr), &req->user);
	/* Nope.  Reject request without even responding. */
	if (0 == tcp_wrap_result)
	  {
	    Log( "Rejecting connection from barred IP %s.\n",
		 inet_ntoa (psa->sin_addr) );
	    return;
	  }
#endif

	mesg[ 0 ] = '\0';
	if( req->version != CURVERSION ) {
		ServerReplyMesg( psa, RESP_ERR, "unknown version" );
		return;
	}
	switch( req->code ) {
	case REQ_VERBOSE_ON:
		verbose = 1;
		ServerReplyMesg( psa, RESP_OK, "verbose-mode on" );
		break;
		
	case REQ_VERBOSE_OFF:
		verbose = 0;
		ServerReplyMesg( psa, RESP_OK, "verbose-mode off" );
		break;
		
	case REQ_REBOOT:
		if( verbose ) {
			Log( "received a reboot request" );
		}
		ServerReplyMesg( psa, RESP_OK, "rebooted" );
		Reboot( 0 );
		break;
		
	case REQ_QUIT:
		if( verbose ) {
			Log( "received a quit request" );
		}
		ServerReplyMesg( psa, RESP_OK, "quitting" );
		Quit( 0 );
		break;
		
	case REQ_STATUS:
		if( verbose ) {
			Log( "received a status request" );
		}
		Status( psa, req );
		break;

	case REQ_TOUCHCONNECTION:
		if( verbose ) {
			Log( "received a touch-connection request" );
		}
		cdata = getcondata( config, req->conname, req->conclass );
		if( cdata != NULL && cdata->conpid != BADPID &&
		    strequ( cdata->user, req->user ) ) {
			
			cdata->contime = time( 0 );
			
		}
#if 0
		if( cdata == NULL ) {
			ServerReplyMesg(psa, RESP_ERR, "connection not found");
		}
		else if( cdata->conpid == BADPID ) {
			ServerReplyMesg( psa, RESP_ERR,
					"connection not running" );
		}
		else if( ! strequ( cdata->user, req->user ) ) {
			ServerReplyMesg( psa, RESP_ERR,
					"connection not yours" );
		}
		else {
			cdata->contime = time( 0 );
			ServerReplyMesg( psa, RESP_OK, "connection touched" );
		}
#endif
		break;
		
	case REQ_MAKECONNECTION:
		if( verbose ) {
			Log( "received a make-connection request" );
		}
		cdata = getcondata( config, req->conname, req->conclass );
		if( cdata == NULL ) {
			ServerReplyMesg(psa, RESP_ERR, "connection not found");
		}
		else if( cdata->conpid != BADPID ) {
			ServerReplyMesg( psa, RESP_ERR, "connection running" );
		}
		else {
			MakeConnection( psa, req, cdata );
		}
		break;

	case REQ_BREAKCONNECTION:
		if( verbose ) {
			Log( "received a break-connection request" );
		}
		cdata = getcondata( config, req->conname, req->conclass );
		if( cdata == NULL ) {
			ServerReplyMesg(psa, RESP_ERR, "connection not found");
			break;
		}
		if( cdata->conpid == BADPID ) {
			ServerReplyMesg( psa, RESP_ERR,
					"connection not running" );
		}
		else {
			kill( cdata->conpid, SIGKILL );
			ServerReplyMesg( psa, RESP_OK, "connection broken" );
		}
		break;
		
	default:
		if( verbose ) {
			FLog( stderr, "received an invalid request" );
		}
		ServerReplyMesg( psa, RESP_ERR, "invalid command" );
		break;
	}
	return;
}

static int
scopy( from, to )
	char * from;
	char * to;
{
	int tmp = strlen( from );
	bcopy( from, to, tmp );
	to += tmp;
	*to = '\0';
	return( tmp + 1 );
}


/*---------------------------------------------------------------------------
 * Status - process status request
 *---------------------------------------------------------------------------
 */
static
Status( psa, req )
     struct sockaddr_in * psa;
     struct request * req;
{
	struct reply reply;
	struct condata * cdata;
	int numc = 0;
	int now = time( 0 );
	char * buf;
	
	buf = reply.details;

	for( cdata = config; cdata != NULL; cdata = cdata->next ) {
		int tmp;
		int active;
		
		if( req->conname[ 0 ] != '\0' &&
		    ! strequ( req->conname, cdata->conname ) ) {
			continue;
		}
		if( req->conclass[ 0 ] != '\0' &&
		    ! strequ( req->conclass, cdata->conclass ) ) {
			continue;
		}
		
		numc++;

		buf += scopy( cdata->conname, buf );
		
		buf += scopy( cdata->conclass, buf );

		active = ( cdata->conpid != BADPID );
		
		if( ! active ) {
			*buf++ = 0;
			continue;
		}
		
		*buf++ = 1;
		
		buf += scopy( cdata->user, buf );

		{
			char tbuf[ 12 ];
			int hours = 0, min = 0, sec = 0;
	
			sec = now - cdata->contime;
			hours = sec / ( 60 * 60 ); 
			sec %= ( 60 * 60 );
			min   = sec / 60;
			sec %= 60;
	
			sprintf( tbuf, "%02d:%02d:%02d", hours, min, sec );

			buf += scopy( tbuf, buf );
		}
	}
	
	initReply( & reply, RESP_OK, hostname, numc );

	sendtoReply( sock, psa, & reply, buf - reply.details );
}

/*---------------------------------------------------------------------------
 * CleanUp - clean up server
 *---------------------------------------------------------------------------
 */
static 
CleanUp()
{
	struct condata * cdata;
	
	close( sock );
	for( cdata = config; cdata != NULL; cdata = cdata->next ) {
		if( cdata->conpid != BADPID ) {
			kill( cdata->conpid, SIGKILL );
		}
	}
}


static void
socktimeout( i )
    int i;
{
	FLog( stderr, "acceptTCP() timed out" );
	exit( 1 );
}

static int
makeTCPConnection( psa )
     struct sockaddr_in * psa;
{
	int fd, rv;
	struct sockaddr_in sa;
	int len;
	char mesg[ MAXMESG ];
	
	if( ( fd = passiveTCP( 0, 5 ) ) < 0 ) {
		FLog( stderr, "passiveTCP( 0, 5 ) failed" );
		exit( 1 );
	}
	
	len = sizeof( struct sockaddr_in );
	getsockname( fd, (struct sockaddr *) & sa, & len );
	
	sprintf( mesg, "%d : connect port", ntohs( sa.sin_port )  );
	ServerReplyMesg( psa, RESP_OK, mesg );

	close( sock );
	
	signal( SIGALRM, socktimeout );
	
	alarm( LTIMEOUT );
	rv = acceptTCP( fd );
	alarm( 0 );

	close( fd );

	if( rv < 0 ) {
		FLog( stderr, "acceptTCP() failed" );
		exit( 1 );
	}
	
	return( rv );
}

/*---------------------------------------------------------------------------
 * MakeConnection - make the connection and run the program
 *---------------------------------------------------------------------------
 */
static int
MakeConnection( psa, req, cdata )
     struct sockaddr_in * psa;
     struct request * req;
     struct condata * cdata;
{
	int pid;

	if ( logusers ) {
		Log( "user %s: %s %s", req->user, cdata->conprog,
		     cdata->argsline );
	}
	
	if( ( pid = fork() ) == 0 ) {
		int tcp = makeTCPConnection( psa );
		char * argv[ MAXNUMBERARGS + 2 ];
		int i;

		argv[ 0 ] = cdata->conprog;
		for( i = 0; i < MAXNUMBERARGS; i++ ) {
			argv[ i + 1 ] = cdata->progargs[ i ];
		}
		argv[ MAXNUMBERARGS + 1 ] = NULL;

		if( dup2( tcp, 0 ) == -1 ) {
			FLog( stderr, "dup2(tcp,0) failed" );
			exit( 1 );
		}
		if( dup2( tcp, 1 ) == -1 ) {
			FLog( stderr, "dup2(tcp,1) failed" );
			exit( 1 );
		}
		if( dup2( tcp, 2 ) == -1 ) {
			FLog( stderr, "dup2(tcp,2) failed" );
			exit( 1 );
		}

		close( tcp );

		execv( cdata->conprog, argv );
		
		FLog( stderr, "execv(%s) failed", cdata->conprog );
		exit( 1 );
	}
	if( verbose ) {
		Log( "program pid = %d", pid );
	}
	if( pid < 0 ) {
		FLog( stderr, "fork failed" );
	}
	else {
		cdata->conpid = pid;
		cdata->contime = time( 0 );
		strncpy( cdata->user, req->user, MAXUSERNAME - 1 );
	}
	return( cdata->conpid );
}

/*---------------------------------------------------------------------------
 * findDeadChildren - collect status of any children that might have fininshed
 *	       recently
 *---------------------------------------------------------------------------
 */
static void
findDeadChildren()
{
	int childpid;
	int waitoptions = WNOHANG;
	int status;
	
#ifdef SOLARIS
	while((childpid = waitpid((pid_t)-1, &status, waitoptions)) > 0 ) {
#else
	while( ( childpid = wait3( & status, waitoptions, NULL ) ) > 0 ) {
#endif
		struct condata * cdata;
		if( verbose ) {
			Log( "child process %d has died ", childpid );
		}
		for( cdata = config; cdata != NULL; cdata = cdata->next ) {
			if( cdata->conpid == childpid ) {
				cdata->user[ 0 ] = '\0';
				cdata->contime = 0;
				cdata->conpid = BADPID;
			}
		}
	}
}

#ifndef linux
 extern char * sys_errlist[];
#endif

/*---------------------------------------------------------------------------
 * sendtoReply - send a reply struct in a UDP packet to a specific host
 *---------------------------------------------------------------------------
 */
static int
sendtoReply( s, to, reply, len )
    int s;
    struct sockaddr_in * to;
    struct reply * reply;
    int len;
{

	int rv = len;
	
	len += sizeof( struct reply );
	len -= MAXDETAILS;
	
	if( sendto( s, (char *) reply, len, 0,
		    (struct sockaddr *)to, sizeof( struct sockaddr ) ) <= 0 ) {
		FLog( stderr, "can't sendto reply: %s", sys_errlist[ errno ] );
		return( -1 );
	}
	return( rv );
}

/*---------------------------------------------------------------------------
 * recvfromRequest - receive a request structure in a UDP packet
 *---------------------------------------------------------------------------
 */
int
recvfromRequest( s, from, timeout, req )
     int s;
     struct sockaddr_in * from;
     int timeout;
     struct request * req;
{
	int status;
	int fromlen = sizeof( struct sockaddr_in );

	if( ! readDelay( s, timeout ) ) {
		return( -1 );
	}
	
	if( ( status = recvfrom( s, (char *) req, sizeof( struct request ), 0,
				 (struct sockaddr *)from, &fromlen ) ) <= 0 ) {
		FLog( stderr, "can't receive request: %s",
		      sys_errlist[ errno ] );
		return( -1 );
	}
	
	if( status != sizeof( struct request ) ) {
		FLog( stderr, "request wrong size" );
		return( -1 );
	}

	return( sizeof( struct request ) );
}
