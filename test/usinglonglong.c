#include <stdlib.h>
#include "longlong.h"

/* return successfully if using "long long" */

int main(int argc, char *argv[]) 
{
#ifdef USE_LONG_LONG
  exit(0);
#else
  exit(1);
#endif
}
