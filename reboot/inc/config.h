/*
 *
 * Annex-UX R8.0 src/inc/config.h Wed Oct 26 10:23:56 EST 1994
 *
 * hardware "SUN" software "SYS_V" network "TLI"
 *
 */
 
#include <sys/types.h>
#include <sys/socket.h>
 
/* map function names */

 
#define bcopy(s1, s2, l) memcpy(s2, s1, l)
#define bzero(b, l) memset(b, 0, l)
#define rindex strrchr
#define index strchr
