/*
 *****************************************************************************
 *
 *		Copyright 1989, Xylogics, Inc.  ALL RIGHTS RESERVED.
 *
 * ALL RIGHTS RESERVED. Licensed Material - Property of Xylogics, Inc.
 * This software is made available solely pursuant to the terms of a
 * software license agreement which governs its use.
 * Unauthorized duplication, distribution or sale is strictly prohibited.
 *
 * Module Description:
 *
 * Command to spool to an Annex, serial or parallel port.
 */

#define RCSDATE $Date: 2002/01/14 19:24:02 $
#define RCSREV	$Revision: 1.1 $
#define RCSID   "$Header: /home/jalembke/cvs/ConsoleTools/reboot/be-reset/be-reset.c,v 1.1 2002/01/14 19:24:02 brylow Exp $"
#ifndef lint
static char rcsid[] = RCSID;
#endif

/*
 * Reset Xinu back-end machines by telling the reset controller which line
 * to reset.
 *
 * usage: be-reset -Aannex -Lline [-pprinter_port] [-D]
 *
 * -A specifies the hostname or Internet address of the Annex to print on.
 *
 * -L specifies the line to reset.
 *
 * -p specifies the printer port on the Annex to use.
 *
 * -D specifies that debug output should be sent to standard output.
 *	This option may be repeated (a la rtelnet) for more detailed
 *	information.
 *		Level	Description
 *		  1	Connection acknowledge
 *		  2	Connection setup details
 *
 */

/*
 *	Include Files
 */

#include <sys/types.h>
#include "../inc/config.h"
#include "../libannex/api_if.h"
#include <netinet/in.h>
#ifdef SYS_V
#include <string.h>
#else
#include <strings.h>
#endif
#include <netdb.h>
#include <fcntl.h>

#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#define CNULL (char *)NULL
#define MAX_SERIAL_PORTS 32
#define MAX_PRINTER_PORTS 32

#define SERIAL 1
#define PARALLEL 2

/*
 *	External Definitions
 */

extern int errno;
extern char *getenv();
#ifndef SYS_V
#ifndef AIX
extern char *sprintf();
#endif
#endif

/*
 *	Global Data Declarations
 */

int debuglevel = 0;		/* debugging level */
int so;				/* token for socket number */

char *prog;			/* name of this program */

char *file_printing;

long bytes;			/* total bytes to send to printer */

/* Last message read from or sent to the Annex */
char buffer[BUFSIZ];
int buffer_count;

int block_send = 0;		/* if set, send in 1-255 byte blocks */
int do_oob_ack = 0;		/* if set, use old out-of-band ack */

char *sock_errors[] = {
#define SEND_DATA 0
	"file \"%s\" aborted while sending data to Annex",
#define SEND_PORTSET 1
	"error during Annex port select",
#define ACK_PORTSET 2
	"error during wait for ACK after port select",
#define ACK_FINAL 3
	"error during wait for final ACK"
};

/*
 *	Macro Definitions
 */

#ifndef BADSIG
#ifndef SIG_ERR
#define BADSIG (int (*)())-1
#else
#define BADSIG SIG_ERR
#endif
#endif

#define	ARGVAL (*++(*argv) || (--argc && *++argv))
#define ARGSTR(s)	*argv += strlen((s) = *argv);

#define SEND(buf,len,msg) \
	if (api_send(so,(buf),(len),API_NOFLAGS,app_nam,TRUE) != (len)) { \
    		fatal(CNULL, sock_errors[(msg)], file_printing); \
	}

#define SENDOOB(buf,len,msg) \
	if (api_send(so, (buf), (len), API_OOB, app_nam, TRUE) != (len)) { \
    		fatal(CNULL, sock_errors[(msg)], file_printing); \
	}

#define	RECV(buf,len,msg,ret) \
	if((ret = api_recv(so,(buf),(len),API_NOFLAGS,TRUE,app_nam)) < 0) { \
		if (debuglevel > 1) \
			printf("Error:  api_recv returned %d.\n",ret); \
    		fatal(CNULL, sock_errors[(msg)], file_printing); \
	} else if (debuglevel > 1) \
		printf("Received %d bytes.  %02X %02X ...\n",ret,(buf)[0],(buf)[1]);

/*
 * Annex LPD protocol definitions
 */
#define ANNEX_SPOOL_CMD '\011'
#define ACK '\0'
#define NACK '\001'
#define FRAME '\002'

#define OFF		0	/* timer off */
#define ACK_WAIT	10	/* seconds to wait for initial ACK */
#define ACK_TIMEOUT	30	/* seconds to timeout final ACK */

/*
 * usage: display usage info and exit
 */
usage()
{
	fprintf(stderr,
		"Usage: %s -Aannex -L# [-pport] [-D]\n",
		prog);
	exit(1);
}

/* 
 * fatal: print error message and give up
 */
/*VARARGS2*/
void
fatal(perr, str, a1, a2, a3, a4, a5)
char *perr, *str, *a1, *a2, *a3, *a4, *a5;
{
	fprintf(stderr, "%s: ", prog);
	if (str != NULL)
		fprintf(stderr, str, a1, a2, a3, a4, a5);
	if (perr != NULL) {
		if (str != NULL)
			fprintf(stderr, ": ");
		perror(perr);
	} else
		fprintf(stderr, "\n");
	exit(1);
}

/*
 * gotpipe: catch SIGPIPE and die
 */
void gotpipe()
{
	fatal(CNULL,
		file_printing == NULL
		? "Annex connection was lost unexpectedly"
		: "Annex connection was lost during attempt to spool \"%s\"" ,
		file_printing);
}

main(argc,argv)
int argc;
char *argv[];
{
        char *annex = NULL, *sbLine = NULL;
        int port = 1;		/* default, if nothing specified */
        int type = PARALLEL;	/* default, if nothing specified */
	int line = -1;
	char opt; 

	prog = (prog = rindex(argv[0],'/')) ? ++prog : *argv;
	/*
	 * Crack arguments
	 */
	if (argc <= 1)
	   usage(); 
	while (--argc > 0) {			/* for each argument */
		if (**(++argv) == '-')	/* found a flag, deal with it */
			while (**argv && (opt = *++*argv) != '\0')
			switch (opt) {
			case 'D':
				debuglevel++;
				break;
			case 'A':		/* specify annex */
				if (!ARGVAL)
					usage();
				ARGSTR(annex);
				break;
            		case 'p':    /* specify annex parallel port */
                		if (!ARGVAL)
				    usage();

				port = atoi(*argv);
				if (port == -1)
				    usage();
				if (port <= 0 || port > MAX_PRINTER_PORTS)
				    fatal(CNULL,"invalid parallel port number %d", port);
				type = PARALLEL;
                		break;
			case 'L':        /* specify line to reset */
				if (!ARGVAL)
				    usage();

				ARGSTR(sbLine);
				line = atoi(sbLine);
				if(line == -1)
				    usage();
				if (line < 1 || line > MAX_SERIAL_PORTS)
				    fatal(CNULL,"invalid line number %d", line);
				break;
			default:		/* unknown argument */
				usage();
				break;
			}
		else
			break;
   	}
	if (line < 0 || annex == NULL)
		usage();

	/* want streaming mode */
	bytes = 1024;

	if (debuglevel > 0)
		printf("Connecting to Annex %s port %d\n",annex,port);
	make_connection(annex);
	set_annex_port(port, type);

	if (debuglevel > 1)
		printf("Connection set-up is complete.\n");

	reset_line(line);
	/*
	 * End-of-job handshake; wait for final ACK
	 */
	if (ack_ack())
		fatal(CNULL,"Annex didn't acknowledge final data");

	exit (0);
}

/*
 * reset_line: send the reset char, line spec char, then the reset
 */
reset_line(line)
int line;
{
	char buf[2];
	char sb[20];
	int i;
	file_printing = "reset request";

	buf[0] = (char)0x30;		/* reset controller */
	if (debuglevel > 1)
		printf("Reset switch (%0o)\n", (int)buf[0]);
	send_block(buf, 1, SEND_DATA);

	buf[0] = (char)line;		/* turn on relay for line */
	if (debuglevel > 1)
		printf("Turn on line %d (%0o)\n", line, (int)buf[0]);
	send_block(buf, 1, SEND_DATA);
	sleep(1);

	buf[0] = (char)0x30;		/* reset controller */
	if (debuglevel > 1)
		printf("Reset switch (%0o)\n", (int)buf[0]);
	send_block(buf, 1, SEND_DATA);

	return(0);
}

/*
 * make_connection: establish TCP connection with designated Annex
 */
make_connection (hostname)
char *hostname;
{
	struct hostent *host;
	struct servent *serv;
	struct sockaddr_in sname;
	static char app_nam[]="be-reset:make_connection";

	if (signal(SIGPIPE, gotpipe) == BADSIG)
		fatal("signal",CNULL);

#ifdef SYS_V
	memset((void *)&sname, 0, sizeof(sname));
#else
	bzero ((char *)&sname, sizeof(sname));
#endif

	if (debuglevel > 1)
		printf("Attempting to resolve host name \"%s\".\n",hostname);
	sname.sin_addr.s_addr = inet_addr(hostname);
	if (sname.sin_addr.s_addr != -1) {
		sname.sin_family = AF_INET;
	} else {
		host = gethostbyname(hostname);
		if (host) {
			sname.sin_family = host->h_addrtype;
#ifdef SYS_V
			memcpy((void *)&sname.sin_addr,
				(void *)host->h_addr,
				(size_t)host->h_length);
#else
			bcopy(host->h_addr,
				(caddr_t)&sname.sin_addr,
				host->h_length);
#endif
		} else
			fatal(CNULL,
				"can't find host address for Annex \"%s\"",
				hostname);
	}

#ifdef DEBUG
	sname.sin_port = htons(5555);
#else
	if (debuglevel > 1)
		printf("Attempting to resolve service \"printer/tcp\".\n");
	serv = getservbyname("printer","tcp");
	if (serv == NULL)
		fatal(CNULL,"can't get service to printer");
	sname.sin_port = serv->s_port;
	if (debuglevel > 1)
		printf("\t-> resolved to port %d.\n",
			ntohs(sname.sin_port));
#endif

#ifdef EXOS
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	sin->sin_port = 0;
#endif

	if (debuglevel > 1)
		printf("Opening connection to API layer.\n");
	if((so = api_open(IPPROTO_TCP, &sname, app_nam, TRUE)) < 0)
	    exit(1);

#ifdef TLI			/* only bind for TLI, Sockets don't need bind*/
	if (debuglevel > 1)
		printf("Binding API address.\n");
	switch (api_bind(so, (struct t_bind **)0, (struct sockaddr_in *)0, app_nam, TRUE)) {
	    case 0:
		break;
	    case 1:
		exit(-1);
	    case 2:
		exit(1);
	    default:
		break;
	}
#endif

	if (debuglevel > 1)
		printf("Connecting to host through API.\n");
	switch (api_connect(so, &sname, IPPROTO_TCP, app_nam, TRUE)) {
	    case 0:
		break;
	    case 1:
		exit(-1);
	    case 2:
		exit(1);
	    default:
		break;
	}
}

/*
 * Tell the Annex which port to print on by creating a escape sequence
 * to select which port to print on:
 *
 *	  CMD TYPE ; UNIT ; BYTES \n
 *
 * CMD is \011 is for the new-style lpd, where you select, get an ack,
 * and then shovel data.
 *
 *					serial			parallel
 * TYPE is the port type:		1			2
 * UNIT is the unit number:		[1-MAX_SERIAL_PORTS]	1
 * (TYPE & UNIT are ASCII strings)
 */

set_annex_port(port, type)
int port;
int type;
{
	static char app_nam[]="be-reset:set_annex_port";

	(void)sprintf(buffer,"%c%d;%d;%ld\n",
			ANNEX_SPOOL_CMD,	/* command byte */
			type,			/* parallel or serial */
			port,			/* unit */
			bytes);

	if (debuglevel > 1)
		printf("Sending port designator --\n\t\"%s\"",buffer);
	SEND(buffer, strlen(buffer), SEND_PORTSET);

	/* check if Annex supports ACKing */
	if (ack_wait(ACK_PORTSET))
		fatal(CNULL, "Annex can't access requested printer");

	/* Check if count/buffers needed and supported */
	if (buffer_count > 1) {
		if (buffer[1] == 1) {
			block_send = 1;
			if (debuglevel > 0)
				printf("Block send mode.\n");
		} else if (debuglevel > 0)
			printf("Stream mode.\n");
	} else {
		do_oob_ack = 1;		/* backwards compatibility */
		if (debuglevel > 0)
			printf("Backward compatibility mode.\n");
	}
}

/*
 * Send a block of data to the Annex.
 *
 * In block_send mode, this is a pair of bytes that encode an integer
 * in big-endian format in the range 0x0001 through 0xFFFF, followed by
 * 1 to 65536 bytes.  A count of 0x0000 is reserved for end-of-all-files
 * indication.
 *
 * When not in block_send mode, data are just streamed over.
 */
send_block(ptr,len,msg)
char *ptr;
int len,msg;
{
	char blockbuf[2];
	unsigned int piece;
	static char app_nam[]="be-reset:send_block";

	if (block_send) {
		while (len > 0) {
			piece = len;
			if (piece > 65535)
				piece = 65535;
			blockbuf[0] = piece / 256;
			blockbuf[1] = piece % 256;
			SEND(blockbuf,2,msg);
			SEND(ptr,piece,msg);
			len -= piece;
			ptr += piece;
			}
		}
	else
		SEND(ptr,len,msg);
}

/************** Annex LPD protocol handshaking routines **************/

/*
 * ack_alrm: catch SIGALRM and longjmp back
 */
void
ack_alrm(dummy)
int dummy;
{
	static char app_nam[]="be-reset:ack_alrm";

#ifdef SYS_V
	signal(SIGALRM,ack_alrm);
#endif
	(void)alarm(ACK_TIMEOUT);
	if (debuglevel > 0)
		printf("ACK timeout -- going to re-try.\n");
	SEND("",1,ACK_FINAL);	/* detect lost connections! */
}

/*
 * Wait for an inline acknowledgement from the Annex
 *
 * return 0 if we get the ACK, 1 o/w.
 */
int
ack_wait(msg)
int msg;
{
	static char app_nam[]="be-reset:ack_wait";

	if (debuglevel > 1)
		printf("Waiting for acknowledge.\n");
	RECV(buffer,BUFSIZ,msg,buffer_count);
	return (buffer_count <= 0 || buffer[0] != ACK);
}

/*
 * Send an OOB ACK and wait for an inline ACK back.
 * Use timeout to prevent waiting forever.
 *
 * return 0 if exit handshake went ok, 1 o/w.
 *
 * If in block_send mode, then send the magic end-of-all-blocks message
 * and wait for ACK (no OOB).  If old (pre R6.2) Annex detected in
 * set-up, then send an OOB message to terminate the data.  This part is
 * a little unclean.
 */

ack_ack()
{
	int gotack;
	static char app_nam[]="be-reset:ack_ack";

	if (block_send) {
		char endblock[2];

		if (debuglevel > 0)
			printf("Sending End-Of-Blocks message.\n");
	/* send end-of-all-blocks message */
		endblock[0] = endblock[1] = '\0';
		SEND(endblock,2,ACK_FINAL);
		}
	else if (do_oob_ack) {
#ifdef MSG_OOB
		if (debuglevel > 0)
			printf("Sending OOB message.\n");
	/* try to get the OOB past the end of the data for SysV */
		sleep(1);
	/* send OOB ACK to indicate EOF */
		SENDOOB("",1,ACK_FINAL);
	/* clear for recv's on broken 4.2 systems */
		SEND("",1,ACK_FINAL);
#else
		if (debuglevel > 0)
			printf("No OOB possible -- just closing.\n");
	/* if no OOB available, then just trust that it got there. */
		return 0;
#endif
		}
	if (debuglevel > 0)
		printf("Waiting for final ACK.\n");
	if (signal(SIGALRM, ack_alrm) == BADSIG)
		fatal("signal",CNULL);
	(void)alarm(ACK_TIMEOUT);
	gotack = ack_wait(ACK_FINAL);
	(void)alarm(OFF);
	return gotack;
}
