
#include <stdio.h>

bzero( buf, len )
     char * buf;
     int len;
{
	for(; len > 0; len-- ) {
		*buf++ = 0;
	}
}

bcopy( from, to, len )
     char * from, * to;
     int len;
{
	for(; len > 0; len-- ) {
		*to++ = *from++;
	}
}

char *
index( buf, ch )
     char * buf;
     char ch;
{
	for(; *buf; buf++ ) {
		if( *buf == ch ) {
			return( buf );
		}
	}
	return( NULL );
}

#ifdef SOLARIS

int
getdtablesize()
{
	/* TODO: fix this */ 
	return( 20 );
}

#endif

#ifdef HPPA

int
getdtablesize()
{
	return( _NFILE );
}

#endif
