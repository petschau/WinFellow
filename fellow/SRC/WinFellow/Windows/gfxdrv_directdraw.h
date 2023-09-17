#ifndef GFXDRV_DIRECTDRAW_H
#define GFXDRV_DIRECTDRAW_H

#include "DEFS.H"
#include "DRAW.H"

void gfxDrvDDrawClearCurrentBuffer();
void gfxDrvDDrawSetMode(draw_mode *dm, bool windowed);
void gfxDrvDDrawSizeChanged(unsigned int width, unsigned int height);
void gfxDrvDDrawPositionChanged();

uint8_t *gfxDrvDDrawValidateBufferPointer();
void gfxDrvDDrawInvalidateBufferPointer();
void gfxDrvDDrawGetBufferInformation(draw_buffer_information *buffer_information);

void gfxDrvDDrawFlip();

bool gfxDrvDDrawEmulationStart(uint32_t maxbuffercount);
unsigned int gfxDrvDDrawEmulationStartPost();
void gfxDrvDDrawEmulationStop();

bool gfxDrvDDrawStartup();
void gfxDrvDDrawShutdown();

bool gfxDrvDDrawSaveScreenshotFromDCArea(HDC, DWORD, DWORD, DWORD, DWORD, uint32_t, DWORD, const STR *);
bool gfxDrvDDrawSaveScreenshot(const bool, const STR *);

#endif
