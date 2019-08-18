#pragma once

#include "fellow/api/portable.h"

/* Maximum values for memory, don't change */

#define CHIPMEM 0x200000
#define FASTMEM 0x800000
#define BOGOMEM 0x1c0000
#define KICKMEM 0x080000

/* Fellow types to ensure correct sizes */

typedef unsigned char UBY;
typedef unsigned short int UWO;
typedef unsigned int ULO;
typedef unsigned FELLOW_LONG_LONG ULL;
typedef signed char BYT;
typedef signed short int WOR;
typedef signed int LON;
typedef signed FELLOW_LONG_LONG LLO;
typedef int BOOLE;

#define FALSE 0
#define TRUE 1
typedef char STR;

#ifndef X64
#define PTR_TO_INT(i) ((ULO)i)
#define PTR_TO_INT_MASK_TYPE(i) ((ULO)i)
#endif
#ifdef X64
#define PTR_TO_INT(i) ((ULL)i)
#define PTR_TO_INT_MASK_TYPE(i) ((ULL)i)
#endif

/* Filename length used throughout the code */

#define CFG_FILENAME_LENGTH 256

extern UBY configromname[];

typedef void (*planar2chunkyroutine)();

#ifdef _DEBUG
#define F_ASSERT(expr)                                                                                                                                                                                 \
  if (!(expr)) throw;
#else
#define F_ASSERT(expr) ;
#endif
