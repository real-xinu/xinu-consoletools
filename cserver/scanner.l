%{
/*
 * Scanner for machines database.
 *
 * Steve Chapin, 13 Apr 1988
 */
#include <stdio.h>

#include "tokens.h"

extern char *xmalloc();

YYSTYPE yylval;

/*---------------------------------------------------------------------------
 * newsb - makes a new string and copy in the contents of the argument
 *---------------------------------------------------------------------------
 */
char * newsb( sb )
     char * sb;
{
	char * sb2;

	if( sb == NULL ) {
		return( NULL );
	}
	sb2 = (char *) xmalloc( strlen( sb ) + 1 );
	strcpy( sb2, sb );
	return( sb2 );
}

static int linecnt = 1;

%}
C		[^\n\t ]
N		[\n]
W		[\t ]|"\\"\n
%%
{W}			/* eat up white space */ ;
"#"[^\n]*		/* eat up comment lines */ ;
{N}			{
    			    yylval.sb = NULL;
			    linecnt++;
			    return( TKNEWLINE );
			}
{C}+			{
    			    yylval.sb = newsb( yytext );
		    	    return( TKSTRING );
			}
%%

int
tkline()
{
	return( linecnt );
}

int
tknext()
{
	int rv;
	static int eof = 0;

	if(  eof ) {
		return( TKEOF );
	}

	if( ( rv = yylex() ) == TKEOF ) {
		eof = 1;
	}

	return( rv );
}

char *
tkvalue()
{
	return( yylval.sb );
}

