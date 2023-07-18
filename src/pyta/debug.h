#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifndef DEBUG
#define DEBUG 0

#endif

#define debug_msg(fmt...) \
  do {if (DEBUG) fprintf (stderr, fmt);} while(0);

#endif
