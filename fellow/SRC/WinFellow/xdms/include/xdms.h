#ifndef XDMS_H
#define XDMS_H

#include "pfile.h"

USHORT dmsUnpack(const char *src, char *dst);
void dmsErrMsg(USHORT err, char *i, char *o, char *errMess);

#endif
