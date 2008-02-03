#ifndef DEFS_H
#define DEFS_H

#include "portable.h"
#include "renaming.h"

/* Defines, some must be set also in include\asm\defs.inc */


/*===========================================================================*/
/* Add UAE filesystem to the executable                                      */
/*===========================================================================*/

#define UAE_FILESYS


/*===========================================================================*/
/* Add GUI support to compiled binary                                        */
/*===========================================================================*/

#define WGUI


/*===========================================================================*/
/* Disable some or all OS dependent drivers                                  */
/* As anyone can observe, keyboard and mouse is disabled here, but still     */
/* works in Windows, so these no longer makes sense.                         */
/*===========================================================================*/

#define WITH_OS_TIMER

/*========================================*/
/* 68000 prefetch word emulation          */
/* Slower, but some programs need it      */
/* Should not be enabled if the intention */
/* is to run 010++ binaries.              */
/*========================================*/

#define PREFETCH

/*================================*/
/* Use direct memory PC-pointer   */
/* Visibly faster than controlled */
/* memory access.                 */
/*================================*/

//#define PC_PTR

/*================================*/
/* The rest is not wise to change */
/*================================*/

/*#define TSC_PROFILING*/


/* Maximum values for memory, don't change */

#define CHIPMEM 0x200000
#define FASTMEM 0x800000
#define BOGOMEM 0x1c0000
#define KICKMEM 0x080000

/* Fellow types to ensure correct sizes */

typedef unsigned char UBY;
typedef unsigned short int UWO;
typedef unsigned long ULO;
typedef unsigned FELLOW_LONG_LONG ULL;
typedef signed char BYT;
typedef signed short int WOR;
typedef signed long LON;
typedef signed FELLOW_LONG_LONG LLO;
typedef int BOOLE;
#define FALSE 0
#define TRUE  1
typedef char STR;


/* Filename length used throughout the code */

#define CFG_FILENAME_LENGTH 256


/* Various function type prototypes */

typedef ULO amigaadress;
typedef ULO (*ciareadfunc)(ULO i);
typedef void (*ciawritefunc)(ULO i, ULO data);

/*------------------------------------*/
/* The decode routines have this type */
/*------------------------------------*/

typedef void (*decoderoutinetype)(ULO,ULO);

extern UBY configromname[];

typedef union {
          ULO *lptr;
          UWO *wptr;
          UBY *bptr;
          ULO lval;
          UWO wval[2];
          UBY bval[4];
          } ptunion;

#define LORES 1
#define HIRES 2

typedef void (*planar2chunkyroutine)(void);

typedef void (*playbuffer_routine)(void);
typedef void (*sound_before_emu_routine)(void);
typedef void (*sound_after_emu_routine)(void);

typedef ULO (*eareadfunc)(ULO);
typedef void (*eawritefunc)(ULO,ULO);
typedef ULO (*eacalcfunc)(ULO);
typedef void (*eac063isfunc)(void);

typedef void (*buseventfunc)(void);

#define CYCLESPERLINE    228
#define CYCLESPERFRAME 71364
#define LINESPERFRAME    313

#define FELLOWVERSION        "WinFellow alpha v0.4.4 build 2"
#define FELLOWLONGVERSION    "WinFellow Amiga Emulator alpha v0.4.4 build 2" 
#define FELLOWNUMERICVERSION "0.4.4.2"

//#define FELLOWVERSION        "WinFellow alpha v0.4.4 - CVS"
//#define FELLOWLONGVERSION    "WinFellow Amiga Emulator alpha v0.4.4 - CVS" 
//#define FELLOWNUMERICVERSION "0.4.4.1"

#endif
