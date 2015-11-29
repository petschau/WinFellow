#ifndef GFXDRV_DIRECTDRAW_H
#define GFXDRV_DIRECTDRAW_H

#include "DEFS.H"
#include "DRAW.H"

void gfxDrvDDrawSetMode(draw_mode *dm);
void gfxDrvDDrawSizeChanged();

UBY *gfxDrvDDrawValidateBufferPointer();
void gfxDrvDDrawInvalidateBufferPointer();

void gfxDrvDDrawFlip();

bool gfxDrvDDrawEmulationStart(ULO maxbuffercount);
unsigned int gfxDrvDDrawEmulationStartPost();
void gfxDrvDDrawEmulationStop();

bool gfxDrvDDrawStartup();
void gfxDrvDDrawShutdown();

#endif
