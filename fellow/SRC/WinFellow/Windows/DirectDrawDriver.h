#pragma once

#include "Defs.h"
#include "Renderer.h"

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

bool gfxDrvDDrawSaveScreenshotFromDCArea(HDC, DWORD, DWORD, DWORD, DWORD, uint32_t, DWORD, const char *);
bool gfxDrvDDrawSaveScreenshot(const bool, const char *);
