/* 
 * token.h - header file for parser
 */

#ifndef __token_h__
#define __token_h__

typedef union {
    char * sb;
} YYSTYPE;

#define TKSTRING  257
#define TKNEWLINE 258
#define TKEOF	  0

#endif /* __token_h__ */
