#ifndef GFXDRV_H

#include "draw.h"

/*===========================================================================*/
/* Implementing these functions creates a graphics driver for Fellow         */
/*===========================================================================*/

extern void gfxDrvSet8BitColor(ULO col, ULO r, ULO g, ULO b);
extern void gfxDrvBufferFlip(void);
extern void gfxDrvSetMode(draw_mode *dm, ULO vertical_stretch, ULO vertical_stretch_strategy);
extern UBY *gfxDrvValidateBufferPointer(void);
extern void gfxDrvInvalidateBufferPointer(void);
extern BOOLE gfxDrvEmulationStart(ULO maxbuffercount);
extern ULO gfxDrvEmulationStartPost(void);
extern void gfxDrvEmulationStop(void);
extern void gfxDrvRunEventSet(void);
extern void gfxDrvRunEventReset(void);
extern BOOLE gfxDrvStartup(void);
extern void gfxDrvShutdown(void);

#endif
