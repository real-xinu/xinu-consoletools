/* 
 * log.c - log routines
 */

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <sys/file.h>
#include <string.h>
#include <strings.h>

/*---------------------------------------------------------------------------
 * TimeToSb - return pointer to current time string
 *---------------------------------------------------------------------------
 */
static char *
timeToSb ( ti )
     long ti;
{
	struct tm * ptm;
	char * rv;
	
	ptm = localtime( & ti );
	rv = asctime( ptm );
	
	/* rv in form: "Sat Nov 10 11:23:13 1984" */
	
	rv[ 19 ] = '\0';	/* clobber year */
	rv += 4;		/* scoot over day of week */
	
	return( rv );
}

/*---------------------------------------------------------------------------
 * FLog - log to specified file 
 *---------------------------------------------------------------------------
 */
void
FLog( file, format, a1, a2, a3, a4, a5 )
     FILE * file;
     char * format;
     int a1, a2, a3, a4, a5;
{
	int pid;
	char procid[ 32 ];
	
	pid = getpid();
	
	fprintf( file, "%s pid=%d:\t", timeToSb( time (0L) ), pid );
	fprintf( file, format, a1, a2, a3, a4, a5 );
	if( format != NULL && *format != '\0' ) {
		if( format[ strlen( format ) - 1 ] != '\n' ) {
			fprintf( file, "\n" );
		}
	}
	fflush( file );
}

/*---------------------------------------------------------------------------
 * Log - log to stdout
 *---------------------------------------------------------------------------
 */
void
Log( format, a1, a2, a3, a4, a5 )
     char * format;
     int a1, a2, a3, a4, a5;
{
	FLog( stdout, format, a1, a2, a3, a4, a5 );
}

