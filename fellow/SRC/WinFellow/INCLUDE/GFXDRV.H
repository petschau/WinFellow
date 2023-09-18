#pragma once

#include "draw.h"

/*===========================================================================*/
/* Implementing these functions creates a graphics driver for Fellow         */
/*===========================================================================*/

extern void gfxDrvClearCurrentBuffer();

extern void gfxDrvBufferFlip();

extern void gfxDrvSetMode(draw_mode *dm, bool windowed);
extern void gfxDrvSizeChanged(unsigned int width, unsigned int height);
extern void gfxDrvPositionChanged();

extern uint8_t *gfxDrvValidateBufferPointer();
extern void gfxDrvInvalidateBufferPointer();
extern void gfxDrvGetBufferInformation(draw_buffer_information *buffer_information);

extern uint32_t gfxDrvEmulationStartPost();

extern bool gfxDrvEmulationStart(unsigned int maxbuffercount);
extern void gfxDrvEmulationStop();

extern void gfxDrvNotifyActiveStatus(bool active);

extern bool gfxDrvRestart(DISPLAYDRIVER displaydriver);
extern bool gfxDrvStartup(DISPLAYDRIVER displaydriver);
extern void gfxDrvShutdown();

extern bool gfxDrvSaveScreenshot(const bool, const char *filename);

extern bool gfxDrvDXGIValidateRequirements(void);
