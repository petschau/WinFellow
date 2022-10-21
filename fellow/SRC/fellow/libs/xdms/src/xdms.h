#ifndef XDMS_H
#define XDMS_H

#ifndef USHORT
#define USHORT unsigned short
#endif

#include "pfile.h"

USHORT dmsUnpack(char *, char *);
void dmsErrMsg(USHORT, char *, char *, char *);

#endif
