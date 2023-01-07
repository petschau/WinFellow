#ifndef XDMS_H
#define XDMS_H

#ifndef USHORT
#define USHORT unsigned short
#endif

#include "pfile.h"

extern USHORT dmsUnpack(char *, char *);
extern void dmsErrMsg(USHORT, char *, char *, char *);

#endif
