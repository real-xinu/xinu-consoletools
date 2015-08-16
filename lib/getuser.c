
#include <stdio.h>
#include <pwd.h>

#include "cserver.h"

extern char * xmalloc();

/* getlogin does not work when odt, or download is used within a 	*/
/* 'script' session - so we must use getpwuid like whoami does		*/

struct passwd * getpwuid();		/* get password entry 		*/

static char * g_getuser = NULL;

/*---------------------------------------------------------------------------
 * getuser - get the user's name
 *---------------------------------------------------------------------------
 */
char *
getuser()
{
	if( g_getuser == NULL ) {
		struct passwd * pp;

		pp = getpwuid( geteuid() );
		if( pp == (struct passwd *) NULL ) {
			fprintf( stderr, "getuser() failed\n" );
			exit( 1 );
		}
		g_getuser = xmalloc( MAXUSERNAME );
		strncpy( g_getuser, pp->pw_name, MAXUSERNAME - 1 );
		g_getuser[ MAXUSERNAME - 1 ] = '\0';
	}
	return( g_getuser );
}
