/* This module provides an equivalent of <netinet/in.h> on  */
/* unix systems. */

#include "longlong.h"

unsigned long htonl(unsigned long x);
unsigned long ntohl(unsigned long x);
mg_ullong ntohll(mg_ullong x);
unsigned short htons(unsigned short x);
unsigned short ntohs(unsigned short x);
