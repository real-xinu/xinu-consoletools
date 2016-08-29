/* 
 * cserver.h - main header file for the distributed connection server
 */

#ifndef __cserver_h__
#define __cserver_h__

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

/* configuration database name */
#define CS_CONFIGURATIONFILE	"/p/xinu/etc/connections.config"
    
/* log directory to write errors to	*/
#define	CS_LOGGINGDIR		"/p/xinu/etc/conlog/"

/* group connect server should run as (optional) */
#define CS_GROUP		"newxinu"
    
/* port server will listen on	*/
#define CS_PORT			2025

/* server must run as root (optional) */
#define CS_RUN_AS_ROOT

/* domain name (optional) */
#define DOMAINNAME "cs.purdue.edu"

/* Parameters */
/* maximum reservation time in minutes (optional) */
#define RESERVETIME		10

/* maximum number of servers */
#define MAXSERVERS		256

/* default class name when making a connection */
#define DEFAULTCLASSNAME 	"quark"

/* default server name */
#define DEFAULTSERVERNAME 	"xinuserver.cs.purdue.edu"

/* Environment Variables */

/* default class name - overrides built-in default (optional) */
#define ENVCLASSNAME		"CS_CLASS"

/* colen separated list of servers (optional) */
#define ENVSERVERS		"CS_SERVERS"

/* size constants for data names */
#define MAXHOSTNAME	64		/* max machine name length	*/
#define MAXCONNECTIONS	2048		/* max number of connections	*/
#define MAXUSERNAME	16		/* max user id length		*/
#define MAXCONNECTIONNAME 16		/* max connection name length	*/
#define MAXCLASSNAME	16		/* len of class string		*/
#define MAXTIMELENGTH	12		/* len for time string		*/
#define MAXARGLENGTH	64		/* max argument length		*/
#define MAXNUMBERARGS	16		/* max argument length		*/

/* UDP datagram format for requests and responses to the server */
/* everything is in character format so it is machine independent */

struct request {
	char version;			/* version number of this structure */
	char code;			/* command code			*/
	char user[ MAXUSERNAME ];	/* user id of requester		*/
	char conname[ MAXCONNECTIONNAME ];/* connection name 		*/
	char conclass[ MAXCLASSNAME ];	/* class of connection		*/
};

struct statusreplyData {	/* connection data */
	char conname[ MAXCONNECTIONNAME ];/* connection name 		*/
	char conclass[ MAXCLASSNAME ];	/* class of connection 		*/
	char active;			/* connection currently active	*/
	char user[ MAXUSERNAME ];	/* user login connected		*/
	char contime[ MAXTIMELENGTH ];	/* connection established for	*/
};

#define MAXDETAILS	(sizeof( struct statusreplyData ) * MAXCONNECTIONS)

#define MAXNCONNECTIONS	((MAXCONNECTIONS/1000)*3 + 3 + 1)

struct reply {
	char version;			/* version number of this structure */
	char code;			/* reply code			*/
	char hostname[ MAXHOSTNAME ];	/* host name			*/
	char numconnections[ MAXNCONNECTIONS ];	/* number of connections */
	char details[ MAXDETAILS ];	/* details 			*/
};

/* current version */
#define CURVERSION	'C'

/* request codes */
#define REQ_STATUS		4
#define REQ_TOUCHCONNECTION	8
#define REQ_MAKECONNECTION	9
#define REQ_BREAKCONNECTION	10
#define REQ_REBOOT		11
#define REQ_QUIT		12
#define REQ_VERBOSE_ON		13
#define REQ_VERBOSE_OFF		14

/* response codes */
#define RESP_OK			1
#define RESP_ERR		2

/* connection server's incore database */

struct condata {
	/* connection information */
	char * conname;
	char * conclass;
	char * conprog;
	char * progargs[ MAXNUMBERARGS ];
	char argsline[ MAXNUMBERARGS * MAXARGLENGTH ];

	/* locking information */
	char user[ MAXUSERNAME ];
	int conpid;			/* connection processes		*/
	int contime;			/* when connection established	*/

	struct condata * next;
};

#define BADPID (-1)

#define strequ(x,y)	(strcmp(x,y)==0)

#endif /* __cserver_h__ */
