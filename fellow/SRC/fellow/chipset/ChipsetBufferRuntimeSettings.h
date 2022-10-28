#pragma once

#include "fellow/application/DrawDimensionTypes.h"

struct ChipsetBufferRuntimeSettings
{
  RectShresi MaxClip; // Shresi coordinate extent of the chipset buffer, ie. the screen area that the emulator is generating an internal image for. Set during startup to ChipsetMaxClipPal, RP sets
                      // different values.
  RectPixels OutputClipPixels; // The pixel area in the chipset buffer that is to be viewed on the host. Derived from ChipsetOutputClip and scale options.
  DimensionsUInt Dimensions;   // Physical pixel dimensions of the chipset buffer, derived from ChipsetMaxClip and scale options
  unsigned int ColorBits;      // In fullscreen: Has the value asked for; In a window: Color bits of desktop (ddraw) or always 32-bit (dxgi)
  unsigned int ScaleFactor;    // Scale at which the chipset buffer is rendered. Is either 1 (hiresi) or 2 (shresi).
};
