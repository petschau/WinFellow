#ifndef XDMS_H
#define XDMS_H

#ifndef USHORT
#define USHORT unsigned short
#endif

#include "pfile.h"

extern "C" USHORT dmsUnpack(char *, char *);
extern "C" void dmsErrMsg(USHORT, char *, char *, char *);

#endif
