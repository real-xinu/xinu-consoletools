/* 
 * support.c - Support routines for reading the configuration file
 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include "cserver.h"
#include "tokens.h"

extern char * xmalloc();
extern int tknext();
extern char * tkvalue();
extern int tkline();

static struct condata * makeEntry();

/*----------------------------------------------------------------------
 * getcondata - returns a pointer to the condata structure for a specific
 *		connection
 *----------------------------------------------------------------------
 */
struct condata *
getcondata( plist, pname, pclass )
     struct condata * plist;
     char * pname;
     char * pclass;
{
	for( ; plist != NULL; plist = plist->next ) {
		if( strequ( plist->conname, pname ) ) {
			if( pclass != NULL && *pclass != '\0' ) {
				if( strequ( plist->conclass, pclass ) ) {
					return( plist );
				}
			}
			else {
				return( plist );
			}
		}
	}
	return( NULL );
}

extern FILE * yyin;

/*---------------------------------------------------------------------------
 * readConfigurationFile -  will fill the table from the configuration file,
 *			and then return the table and count of entries read.
 *---------------------------------------------------------------------------
 */
struct condata *
readConfigurationFile( filename )
     char * filename;
{
	struct condata * ptbl, * scan;
	int count;
	FILE * f;
	
	if( ( f = fopen( filename, "r" ) ) == NULL ) {
		perror( "can't open configuration file" );
		exit( 1 );
	}
	yyin = f;

	for( count = 0, ptbl = scan = makeEntry( filename ); scan != NULL;
	     count++, scan = scan->next ) {
		scan->next = makeEntry( filename );
	}

	fclose( f );
	
	if( count > MAXCONNECTIONS ) {
		FLog( stderr, "max number of connections exceeded" );
		exit( 1 );
	}
	
	return( ptbl );
}

/*---------------------------------------------------------------------------
 * matchtoken - mathc the next token and return the token value
 *---------------------------------------------------------------------------
 */
static char *
matchtoken( tk, filename )
     int tk;
     char * filename;
{
	if( tk != tknext() ) {
		FLog( stderr, "error in configuration file '%s', line %d",
			filename, tkline() );
		exit( 1 );
	}
	return( tkvalue() );
}

static char * defaulthost = NULL;

/*---------------------------------------------------------------------------
 * ismyname - test if this is this MY name
 *
 *---------------------------------------------------------------------------
 */
static int
ismyname( name )
     char * name;
{
	static char myrealname[ 256 ];
	static amnesia = 1;
	struct hostent *he;
	
	if( amnesia ) {
		/* see who I am */
		char myname[ 100 ];
		
		gethostname( myname, sizeof( myname ) );
		he = gethostbyname( myname );
		strcpy( myrealname, he->h_name );
		amnesia = 0;
	}
	if( ( he = gethostbyname( name ) ) == (struct hostent *) 0 ) {
		return( 0 );
	}
	return( strncmp( myrealname, he->h_name, strlen( he->h_name ) ) == 0 );
}

static struct condata * basicMakeEntry();

static int
skipNewlines()
{
	int tk;
	while( ( tk = tknext() ) == TKNEWLINE )
	    ;
	return( tk );
}

/*---------------------------------------------------------------------------
 * makeEntry - read the configuration file for an entry
 *---------------------------------------------------------------------------
 */
static struct condata *
makeEntry( filename )
     char * filename;
{
	struct condata * cdata;
	int i;
	int tk;
	char * tv;

	while( 1 ) {
		if( ( tk = skipNewlines() ) == TKEOF ) {
			return( NULL );
		}

		if( tk != TKSTRING ) {
			FLog( stderr,
			     "error in configuration file '%s', line %d",
			     filename, tkline() );
			exit( 1 );
		}

		tv = tkvalue();
		if( tv[ strlen( tv ) - 1 ] == ':' ) {
			tv[ strlen( tv ) - 1 ] = '\0';
			if( tv[ 0 ] == '\0' ) {
				defaulthost = NULL;
			}
			else {
				defaulthost = tv;
			}
			continue;
		}
		
		cdata = (struct condata *) xmalloc( sizeof( struct condata ) );
		cdata->conname = tkvalue();
		cdata->conclass = matchtoken( TKSTRING, filename );
		cdata->conprog = matchtoken( TKSTRING, filename );
		cdata->argsline[0] = '\0';

		for( i = 0; i < MAXNUMBERARGS; i++ ) {
			cdata->progargs[ i ] = NULL;
		}
	
		if( strlen( cdata->conname ) >= MAXCONNECTIONNAME ) {
			FLog( stderr,
			     "max connection name exceeded for '%s', line %d",
			     cdata->conname, tkline() );
			exit( 1 );
		}
		if( strlen( cdata->conclass ) >= MAXCLASSNAME ) {
			FLog( stderr,
			     "max class name exceeded for '%s', line %d",
			     cdata->conclass, tkline() );
			exit( 1 );
		}

		tk = tknext();
		for( i = 0; i < MAXNUMBERARGS; i++ ) {
			if( tk == TKSTRING ) {
				cdata->progargs[ i ] = tkvalue();
				tk = tknext();
				if( strlen( cdata->progargs[ i ] ) >= MAXARGLENGTH ) {
					FLog( stderr, "max argument length exceeded for '%s', line %d", cdata->progargs[ i ], tkline() );
					exit( 1 );
				}
				if( i != 0 ) {
					strcat( cdata->argsline, " " );
				}
				strcat( cdata->argsline,
					cdata->progargs[ i ] );
			}
			else {
				break;
			}
		}
	
		if( ( tk != TKNEWLINE ) && ( tk != TKEOF ) )  {
			FLog( stderr,
			     "error in configuration file '%s', line %d",
			     filename, tkline() );
			exit( 1 );
		}

		cdata->user[ 0 ] = '\0';
		cdata->user[ MAXUSERNAME - 1 ] = '\0';
		cdata->conpid = BADPID;
		cdata->next = NULL;
		
		if( defaulthost == NULL || ismyname( defaulthost ) ) {
			return( cdata );
		}

		free( cdata );
	}
}

