#pragma once

#include <cstdint>

#include "fellow/api/configuration/HudConfiguration.h"
#include "fellow/hud/HudPropertyProvider.h"
#include "fellow/application/DrawDimensionTypes.h"
#include "MappedChipsetFramebuffer.h"

class GfxDrvHudLegacy
{
private:
  HudPropertyProvider *_hudPropertyProvider;
  const DimensionsUInt &_chipsetBufferDimensions;
  const RectShresi &_chipsetMaxClip;
  const RectShresi &_chipsetOutputClip;
  const unsigned int _coreBufferScaleFactor;
  const unsigned int _activeBufferColorBits;

  const unsigned int _ledCount = 5;
  const unsigned int _ledWidth = 12;
  const unsigned int _ledHeight = 4;
  const unsigned int _ledFirstX = 16;
  const unsigned int _ledFirstY = 4;
  const unsigned int _ledGap = 8;

  // Host colors in the host pixel format
  const uint32_t _floppyLedColorOn;
  const uint32_t _floppyLedColorOff;
  const uint32_t _powerLedColorOn;
  const uint32_t _powerLedColorOff;

  const bool _fpsFont[12][5][4] = {false, false, true,  false,                                                                                                                 /* 1 */
                                   false, false, true,  false, false, false, true,  false, false, false, true,  false, false, false, true,  false, true,  true,  true,  false, /* 2 */
                                   false, false, true,  false, true,  true,  true,  false, true,  false, false, false, true,  true,  true,  false, true,  true,  true,  false, /* 3 */
                                   false, false, true,  false, true,  true,  true,  false, false, false, true,  false, true,  true,  true,  false, true,  false, true,  false, /* 4 */
                                   true,  false, true,  false, true,  true,  true,  false, false, false, true,  false, false, false, true,  false, true,  true,  true,  false, /* 5 */
                                   true,  false, false, false, true,  true,  true,  false, false, false, true,  false, true,  true,  true,  false, true,  true,  true,  false, /* 6 */
                                   true,  false, false, false, true,  true,  true,  false, true,  false, true,  false, true,  true,  true,  false, true,  true,  true,  false, /* 7 */
                                   false, false, true,  false, false, false, true,  false, false, false, true,  false, false, false, true,  false, true,  true,  true,  false, /* 8 */
                                   true,  false, true,  false, true,  true,  true,  false, true,  false, true,  false, true,  true,  true,  false, true,  true,  true,  false, /* 9 */
                                   true,  false, true,  false, true,  true,  true,  false, false, false, true,  false, true,  true,  true,  false, true,  true,  true,  false, /* 0 */
                                   true,  false, true,  false, true,  false, true,  false, true,  false, true,  false, true,  true,  true,  false, false, false, false, false, /* % */
                                   true,  false, false, true,  false, false, true,  false, false, true,  false, false, true,  false, false, true,  false, false, false, false, /* space */
                                   false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};

  bool _fpsBuffer[5][20];

  void DrawLED16(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const uint16_t color);
  void DrawLED24(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const uint32_t color);
  void DrawLED32(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const uint32_t color);
  void DrawLED(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int index, const bool state);
  void DrawFloppyLEDs(const MappedChipsetFramebuffer &mappedChipsetFramebuffer);
  void DrawPowerLED(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const bool state);

  void DrawFpsChar(const char character, const unsigned int x);
  void DrawFpsText(const char *text);

  void DrawFpsToFramebuffer16(const MappedChipsetFramebuffer &mappedChipsetFramebuffer);
  void DrawFpsToFramebuffer24(const MappedChipsetFramebuffer &mappedChipsetFramebuffer);
  void DrawFpsToFramebuffer32(const MappedChipsetFramebuffer &mappedChipsetFramebuffer);
  void DrawFpsCounter(const MappedChipsetFramebuffer &mappedChipsetFramebuffer);

public:
  void DrawHUD(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const HudConfiguration &hudConfiguration);

  GfxDrvHudLegacy(
      HudPropertyProvider *hudPropertyProvider,
      const DimensionsUInt &chipsetBufferDimensions,
      const RectShresi &chipsetMaxClip,
      const RectShresi &chipsetOutputClip,
      const unsigned int coreBufferScaleFactor,
      const unsigned int activeBufferColorBits,
      const uint32_t floppyLedColorOn,
      const uint32_t floppyLedColorOff,
      const uint32_t powerLedColorOn,
      const uint32_t powerLedColorOff);
};
