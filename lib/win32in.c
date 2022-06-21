#include "longlong.h"

/* This module provides an equivalent of <netinet/in.h> on  */
/* unix systems. */

unsigned long htonl(unsigned long x) {
#if defined (LITTLE_ENDIAN) || defined (_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN)
	x = ((x >> 24) & 0x000000FF) | ((x >> 8) & 0x0000FF00) |
		((x << 8) & 0x00FF0000) | ((x << 24) & 0xFF000000);
#endif

	return x;
}

mg_ullong ntohll(mg_ullong x)
{
#if defined (LITTLE_ENDIAN) || defined(_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN)
	x = 
	  ((x >> 56) & 0x00000000000000FF) | ((x >> 40) & 0x000000000000FF00) |
	  ((x >> 24) & 0x0000000000FF0000) | ((x >> 8)  & 0x00000000FF000000) |
	  ((x << 8)  & 0x000000FF00000000) | ((x << 24) & 0x0000FF0000000000) |
		((x << 40) & 0x00FF000000000000) | ((x << 56) & 0xFF00000000000000);
#endif
  return x;
}

unsigned long ntohl(unsigned long x) {
#if defined (LITTLE_ENDIAN) || defined (_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN)
	x = ((x >> 24) & 0x000000FF) | ((x >> 8) & 0x0000FF00) |
		((x << 8) & 0x00FF0000) | ((x << 24) & 0xFF000000);
#endif
	return x;
}

unsigned short htons(unsigned short x) {
#if defined (LITTLE_ENDIAN) || defined (_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN)
	x = ((x >> 8) & 0x00FF) | ((x << 8) & 0xFF00);
#endif
	return x;
}

unsigned short ntohs(unsigned short x) {
#if defined (LITTLE_ENDIAN) || defined (_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN)
	x = ((x >> 8) & 0x00FF) | ((x << 8) & 0xFF00);
#endif
	return x;
}

/* just to test stuff
#include <stdio.h>

main () {
	long x;
	short sx;

	x = 0x000000FF;
	printf ("before %i, ",(int)x);
	x = htonl (x);
	printf ("after %i\n",(int)x);

	x = 0x0000FF00;
	printf ("before %i, ",(int)x);
	x = htonl (x);
	printf ("after %i\n",(int)x);

	x = 0x00FF0000;
	printf ("before %i, ",(int)x);
	x = htonl (x);
	printf ("after %i\n",(int)x);

	x = 0xFF000000;
	printf ("before %i, ",(int)x);
	x = htonl (x);
	printf ("after %i\n\n",(int)x);

	sx = 0x00FF;
	printf ("before %i, ",(int)sx);
	sx = htons (sx);
	printf ("after %i\n",(int)sx);

	sx = 0xFF00;
	printf ("before %i, ",(int)sx);
	sx = htons (sx);
	printf ("after %i\n",(int)sx);
}
*/
