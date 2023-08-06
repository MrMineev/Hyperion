#ifndef dmode_h
#define dmode_h

#include <stdbool.h>

typedef struct {
  bool mode; // true if debug mode
} DebugMode;

extern DebugMode DMODE;

#endif
