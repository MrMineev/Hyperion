#ifndef commandline_h
#define commandline_h

typedef struct {
  int argc;
  char **argv;
} CommandlineArguments;

extern CommandlineArguments CLA;

#endif
