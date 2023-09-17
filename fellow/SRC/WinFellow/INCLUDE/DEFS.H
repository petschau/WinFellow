#ifndef DEFS_H
#define DEFS_H

#include <cstdint>

#include "portable.h"

//#define DRAW_TSC_PROFILE

/*================================*/
/* The rest is not wise to change */
/*================================*/

/* Maximum values for memory, don't change */

#define CHIPMEM 0x200000
#define FASTMEM 0x800000
#define BOGOMEM 0x1c0000
#define KICKMEM 0x080000

/* Fellow types to ensure correct sizes */

typedef unsigned char UBY;
typedef unsigned short int UWO;
typedef unsigned FELLOW_LONG_LONG ULL;
typedef signed char BYT;
typedef signed short int WOR;
typedef signed int LON;
typedef signed FELLOW_LONG_LONG LLO;
typedef int BOOLE;

#define FALSE 0
#define TRUE  1
typedef char STR;

#ifndef X64
#define PTR_TO_INT(i) ((uint32_t)i)
#define PTR_TO_INT_MASK_TYPE(i) ((uint32_t)i)
#endif
#ifdef X64
#define PTR_TO_INT(i) ((ULL)i)
#define PTR_TO_INT_MASK_TYPE(i) ((ULL)i)
#endif

/* Filename length used throughout the code */

#define CFG_FILENAME_LENGTH 256

/*------------------------------------*/
/* The decode routines have this type */
/*------------------------------------*/

typedef void (*decoderoutinetype)(uint32_t, uint32_t);

extern UBY configromname[];

typedef union {
          uint32_t *lptr;
          UWO *wptr;
          UBY *bptr;
          uint32_t lval;
          UWO wval[2];
          UBY bval[4];
          } ptunion;

typedef void (*planar2chunkyroutine)(void);

typedef void (*playbuffer_routine)(void);
typedef void (*sound_before_emu_routine)(void);
typedef void (*sound_after_emu_routine)(void);

typedef void (*buseventfunc)(void);

#endif