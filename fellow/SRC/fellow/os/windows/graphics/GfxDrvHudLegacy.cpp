#include "GfxDrvHudLegacy.h"
#include "fellow/os/windows/retroplatform/RetroPlatform.h"

void GfxDrvHudLegacy::DrawLED16(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const uint16_t color)
{
  uint16_t *bufw = ((uint16_t *)(mappedChipsetFramebuffer.TopPointer + mappedChipsetFramebuffer.HostLinePitch * y)) + x;
  for (unsigned int y1 = 0; y1 < height; y1++)
  {
    for (unsigned int x1 = 0; x1 < width; x1++)
    {
      *(bufw + x1) = color;
    }
    bufw = (uint16_t *)(((uint8_t *)bufw) + mappedChipsetFramebuffer.HostLinePitch);
  }
}

void GfxDrvHudLegacy::DrawLED24(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const uint32_t color)
{
  uint8_t *bufb = mappedChipsetFramebuffer.TopPointer + mappedChipsetFramebuffer.HostLinePitch * y + x * 3;
  const uint8_t color24_1 = (uint8_t)((color & 0xff0000) >> 16);
  const uint8_t color24_2 = (uint8_t)((color & 0x00ff00) >> 8);
  const uint8_t color24_3 = (uint8_t)(color & 0x0000ff);
  for (unsigned int y1 = 0; y1 < height; y1++)
  {
    for (unsigned int x1 = 0; x1 < width; x1++)
    {
      *(bufb + x1 * 3) = color24_1;
      *(bufb + x1 * 3 + 1) = color24_2;
      *(bufb + x1 * 3 + 2) = color24_3;
    }
    bufb = bufb + mappedChipsetFramebuffer.HostLinePitch;
  }
}

void GfxDrvHudLegacy::DrawLED32(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height, const uint32_t color)
{
  uint32_t *bufl = ((uint32_t *)(mappedChipsetFramebuffer.TopPointer + mappedChipsetFramebuffer.HostLinePitch * y)) + x;
  for (unsigned int y1 = 0; y1 < height; y1++)
  {
    for (unsigned int x1 = 0; x1 < width; x1++)
    {
      *(bufl + x1) = color;
    }
    bufl = (uint32_t *)(((uint8_t *)bufl) + mappedChipsetFramebuffer.HostLinePitch);
  }
}

void GfxDrvHudLegacy::DrawLED(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const unsigned int index, const bool state)
{
  unsigned int x, y, height, width;

#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
  {
#endif
    x = _ledFirstX + (_ledWidth + _ledGap) * index + ((_chipsetOutputClip.Left - _chipsetMaxClip.Left) / 2) * _coreBufferScaleFactor;
    y = _ledFirstY + (_chipsetOutputClip.Top - _chipsetMaxClip.Top) * _coreBufferScaleFactor;
    height = _ledHeight;
    width = _ledWidth;
#ifdef RETRO_PLATFORM
  }
  else
  {
    // TODO: Verify values
    x = _ledFirstX + (_ledWidth + _ledGap) * index + (_chipsetOutputClip.Left / 2 - _chipsetMaxClip.Left) * 2;
    y = _ledFirstY + (_chipsetOutputClip.Top / 2 - _chipsetMaxClip.Top) * 2;
    height = _ledHeight / 2;
    width = _ledWidth / 2;
  }
#endif

  const uint32_t color = (index == 0) ? ((state) ? _powerLedColorOn : _powerLedColorOff) : ((state) ? _floppyLedColorOn : _floppyLedColorOff);

  switch (_activeBufferColorBits)
  {
    case 16: DrawLED16(mappedChipsetFramebuffer, x, y, _ledWidth, height, color); break;
    case 24: DrawLED24(mappedChipsetFramebuffer, x, y, _ledWidth, height, color); break;
    case 32: DrawLED32(mappedChipsetFramebuffer, x, y, _ledWidth, height, color); break;
    default: break;
  }
}

void GfxDrvHudLegacy::DrawPowerLED(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const bool state)
{
  DrawLED(mappedChipsetFramebuffer, 0, state);
}

void GfxDrvHudLegacy::DrawFloppyLEDs(const MappedChipsetFramebuffer &mappedChipsetFramebuffer)
{
  for (unsigned int i = 0; i < 4; i++)
  {
    const HudFloppyDriveStatus &status = _hudPropertyProvider->GetFloppyDriveStatus(i);
    DrawLED(mappedChipsetFramebuffer, i + 1, status.DriveExists && status.MotorOn);
  }
}

//=========================================
// Draws one char in the FPS counter buffer
//=========================================

void GfxDrvHudLegacy::DrawFpsChar(const char character, const unsigned int x)
{
  for (unsigned int i = 0; i < 5; i++)
  {
    for (unsigned int j = 0; j < 4; j++)
    {
      _fpsBuffer[i][x * 4 + j] = _fpsFont[character][i][j];
    }
  }
}

//=====================================
// Draws text in the FPS counter buffer
//=====================================

void GfxDrvHudLegacy::DrawFpsText(const char *text)
{
  for (unsigned int i = 0; i < 4; i++)
  {
    const char c = *text++;
    switch (c)
    {
      case '0': DrawFpsChar(9, i); break;
      case '1': DrawFpsChar(0, i); break;
      case '2': DrawFpsChar(1, i); break;
      case '3': DrawFpsChar(2, i); break;
      case '4': DrawFpsChar(3, i); break;
      case '5': DrawFpsChar(4, i); break;
      case '6': DrawFpsChar(5, i); break;
      case '7': DrawFpsChar(6, i); break;
      case '8': DrawFpsChar(7, i); break;
      case '9': DrawFpsChar(8, i); break;
      case '%': DrawFpsChar(10, i); break;
      case ' ':
      default: DrawFpsChar(11, i); break;
    }
  }
}

//===================================
// Copy FPS buffer to a 16 bit screen
//===================================

void GfxDrvHudLegacy::DrawFpsToFramebuffer16(const MappedChipsetFramebuffer &mappedChipsetFramebuffer)
{
  uint16_t *bufw = ((uint16_t *)mappedChipsetFramebuffer.TopPointer) + _chipsetBufferDimensions.Width - (20 * _coreBufferScaleFactor);

  // Move left to offset clipping at the right
  bufw -= ((_chipsetMaxClip.Right - _chipsetOutputClip.Right) * _coreBufferScaleFactor) / 2;

  // Move down to offset clipping at top
  bufw += ((_chipsetOutputClip.Top - _chipsetMaxClip.Top) * _coreBufferScaleFactor * mappedChipsetFramebuffer.HostLinePitch) / 2;

  for (unsigned int y = 0; y < 5; y++)
  {
    for (unsigned int x = 0; x < 20; x++)
    {
      const uint16_t color = _fpsBuffer[y][x] ? 0xffff : 0;
      const unsigned int index = x * _coreBufferScaleFactor;
      *(bufw + index) = color;

      if (_coreBufferScaleFactor > 1)
      {
        *(bufw + index + 1) = color;
      }
    }
    bufw = (uint16_t *)(((uint8_t *)bufw) + mappedChipsetFramebuffer.HostLinePitch);
  }
}

//===================================
// Copy FPS buffer to a 24 bit screen
//===================================

void GfxDrvHudLegacy::DrawFpsToFramebuffer24(const MappedChipsetFramebuffer &mappedChipsetFramebuffer)
{
  uint8_t *bufb = mappedChipsetFramebuffer.TopPointer + (_chipsetBufferDimensions.Width - 20) * 3 * _coreBufferScaleFactor;

  // Move left to offset clipping at the right
  bufb -= ((_chipsetMaxClip.Right - _chipsetOutputClip.Right) * _coreBufferScaleFactor * 3) / 2;

  // Move down to offset clipping at top
  bufb += (_chipsetOutputClip.Top - _chipsetMaxClip.Top) * _coreBufferScaleFactor * mappedChipsetFramebuffer.HostLinePitch;

  for (unsigned int y = 0; y < 5; y++)
  {
    for (unsigned int scaling = 0; scaling < _coreBufferScaleFactor; scaling++)
    {
      for (unsigned int x = 0; x < 20; x++)
      {
        const uint8_t color = _fpsBuffer[y][x] ? 0xff : 0;
        const unsigned int index = x * _coreBufferScaleFactor;
        *(bufb + index) = color;
        *(bufb + index + 1) = color;
        *(bufb + index + 2) = color;

        if (_coreBufferScaleFactor > 1)
        {
          *(bufb + index + 3) = color;
          *(bufb + index + 4) = color;
          *(bufb + index + 5) = color;
        }
      }
      bufb += mappedChipsetFramebuffer.HostLinePitch;
    }
  }
}

//===================================
// Copy FPS buffer to a 32 bit screen
//===================================

void GfxDrvHudLegacy::DrawFpsToFramebuffer32(const MappedChipsetFramebuffer &mappedChipsetFramebuffer)
{
  const unsigned int coreBufferScaleFactor = _coreBufferScaleFactor;
  uint32_t *bufl = ((uint32_t *)mappedChipsetFramebuffer.TopPointer) + mappedChipsetFramebuffer.HostLinePitch - (20 * coreBufferScaleFactor);

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode())
  {
    // move left to offset for clipping at the right; since width is adjusted dynamically for scale, reduce by that, all other values are static
    bufl -= RETRO_PLATFORM_MAX_PAL_LORES_WIDTH * 2 - RP.GetScreenWidthAdjusted() / RP.GetDisplayScale() - RP.GetClippingOffsetLeftAdjusted();

    // move down to compensate for clipping at top
    bufl += RP.GetClippingOffsetTopAdjusted() * mappedChipsetFramebuffer.HostLinePitch / 4;
  }
  else
#endif
  {
    // Move left to offset clipping at the right
    bufl -= ((_chipsetMaxClip.Right - _chipsetOutputClip.Right) * coreBufferScaleFactor) / 2;

    // Move down to offset clipping at top
    bufl += ((_chipsetOutputClip.Top - _chipsetMaxClip.Top) * coreBufferScaleFactor * mappedChipsetFramebuffer.HostLinePitch) / 4;
  }

  for (unsigned int y = 0; y < 5; y++)
  {
    for (unsigned int scaling = 0; scaling < coreBufferScaleFactor; scaling++)
    {
      for (unsigned int x = 0; x < 20; x++)
      {
        const uint32_t color = _fpsBuffer[y][x] ? 0xffffffff : 0;
        const unsigned int index = x * coreBufferScaleFactor;
        *(bufl + index) = color;

        if (coreBufferScaleFactor > 1)
        {
          *(bufl + index + 1) = color;
        }
      }
      bufl = (uint32_t *)(((uint8_t *)bufl) + mappedChipsetFramebuffer.HostLinePitch);
    }
  }
}

//=========================================
// Draws FPS counter in current framebuffer
//=========================================

void GfxDrvHudLegacy::DrawFpsCounter(const MappedChipsetFramebuffer &mappedChipsetFramebuffer)
{
  char s[16];
  sprintf(s, "%u", _hudPropertyProvider->GetFps());
  DrawFpsText(s);
  switch (_activeBufferColorBits)
  {
    case 16: DrawFpsToFramebuffer16(mappedChipsetFramebuffer); break;
    case 24: DrawFpsToFramebuffer24(mappedChipsetFramebuffer); break;
    case 32: DrawFpsToFramebuffer32(mappedChipsetFramebuffer); break;
    default: break;
  }
}

void GfxDrvHudLegacy::DrawHUD(const MappedChipsetFramebuffer &mappedChipsetFramebuffer, const HudConfiguration &hudConfiguration)
{
  if (hudConfiguration.FPSEnabled)
  {
    DrawFpsCounter(mappedChipsetFramebuffer);
  }

  if (hudConfiguration.LEDEnabled)
  {
    DrawPowerLED(mappedChipsetFramebuffer, _hudPropertyProvider->GetPowerLedOn());
    DrawFloppyLEDs(mappedChipsetFramebuffer);
  }
}

GfxDrvHudLegacy::GfxDrvHudLegacy(
    HudPropertyProvider *hudPropertyProvider,
    const DimensionsUInt &chipsetBufferDimensions,
    const RectShresi &chipsetMaxClip,
    const RectShresi &chipsetOutputClip,
    const unsigned int coreBufferScaleFactor,
    const unsigned int activeBufferColorBits,
    const uint32_t floppyLedColorOn,
    const uint32_t floppyLedColorOff,
    const uint32_t powerLedColorOn,
    const uint32_t powerLedColorOff)
  : _hudPropertyProvider(hudPropertyProvider),
    _chipsetBufferDimensions(chipsetBufferDimensions),
    _chipsetMaxClip(chipsetMaxClip),
    _chipsetOutputClip(chipsetOutputClip),
    _coreBufferScaleFactor(coreBufferScaleFactor),
    _activeBufferColorBits(activeBufferColorBits),
    _floppyLedColorOn(floppyLedColorOn),
    _floppyLedColorOff(floppyLedColorOff),
    _powerLedColorOn(powerLedColorOn),
    _powerLedColorOff(powerLedColorOff)
{
}
