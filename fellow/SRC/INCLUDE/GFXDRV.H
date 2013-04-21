#ifndef GFXDRV_H

#include "draw.h"

// For windows only
extern bool gfx_drv_displaychange;

/*===========================================================================*/
/* Implementing these functions creates a graphics driver for Fellow         */
/*===========================================================================*/

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
extern void gfxDrvChangeDInputDeviceStates(BOOLE);
extern BOOLE gfxDrvTakeScreenShot(BOOLE, STR *);

#endif
