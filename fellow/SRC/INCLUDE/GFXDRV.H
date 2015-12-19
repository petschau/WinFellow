#ifndef GFXDRV_H
#define GFXDRV_H

#include "draw.h"

/*===========================================================================*/
/* Implementing these functions creates a graphics driver for Fellow         */
/*===========================================================================*/

extern void gfxDrvClearCurrentBuffer();

extern void gfxDrvBufferFlip();

extern void gfxDrvSetMode(draw_mode *dm);
extern void gfxDrvSizeChanged();

extern UBY *gfxDrvValidateBufferPointer();
extern void gfxDrvInvalidateBufferPointer();

extern bool gfxDrvEmulationStart(ULO maxbuffercount);
extern ULO gfxDrvEmulationStartPost();

extern void gfxDrvEmulationStop();

extern void gfxDrvRunEventSet();
extern void gfxDrvRunEventReset();

extern bool gfxDrvRestart(DISPLAYDRIVER displaydriver);
extern bool gfxDrvStartup(DISPLAYDRIVER displaydriver);
extern void gfxDrvShutdown();

extern void gfxDrvRegisterRetroPlatformScreenMode(const bool);

extern bool gfxDrvDXGIValidateRequirements(void);

#endif
