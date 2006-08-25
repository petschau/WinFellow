#ifndef GFXDRVSDL_H
#define GFXDRVSDL_H

/*===========================================================================*/
/* Implementing these functions creates a SDL graphics driver for Fellow     */
/*===========================================================================*/

#ifndef DRAW_H
#include "draw.h"
#endif

extern BOOLE gfxdrv_sdl_initialized;

extern void gfxDrvSDL_BufferFlip(void);
extern void gfxDrvSDL_SetMode(draw_mode *dm, ULO vertical_stretch, ULO vertical_stretch_strategy);
extern UBY *gfxDrvSDL_ValidateBufferPointer(void);
extern void gfxDrvSDL_InvalidateBufferPointer(void);
extern BOOLE gfxDrvSDL_EmulationStart(ULO maxbuffercount);
extern ULO gfxDrvSDL_EmulationStartPost(void);
extern void gfxDrvSDL_EmulationStop(void);
extern BOOLE gfxDrvSDL_Startup(void);
extern void gfxDrvSDL_Shutdown(void);

#endif
