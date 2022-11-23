/*=========================================================================*/
/* Fellow                                                                  */
/* Draws an Amiga screen in a host display buffer                          */
/*                                                                         */
/* Authors: Petter Schau                                                   */
/*          Worfje                                                         */
/*                                                                         */
/*                                                                         */
/* Copyright (C) 1991, 1992, 1996 Free Software Foundation, Inc.           */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/
#include <algorithm>

#include "fellow/api/defs.h"
#include "fellow/api/Services.h"
#include "fellow/api/Drivers.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/chipset/HostFrameCopier.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/DualPlayfieldMapper.h"
#include "fellow/api/debug/DiagnosticFeatures.h"
#include "fellow/application/HostRenderer.h"
#include "fellow/chipset/Graphics.h"
#include "fellow/api/drivers/IGraphicsDriver.h"

#include "fellow/chipset/draw_pixelrenderers.h"
#include "fellow/chipset/draw_interlace_control.h"

#include "fellow/chipset/BitplaneShifter.h"

#include <map>

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif

using namespace std;
using namespace fellow::api;

bool HostRenderer::ShouldSkipFrame() const
{
  return _frameSkipCounter != 0;
}

void HostRenderer::AdvanceFrameSkipCounter()
{
  _frameSkipCounter--;

  if (_frameSkipCounter < 0)
  {
    _frameSkipCounter = _hostRenderConfiguration.FrameskipRatio;
  }
}

void HostRenderer::InitializeFrameSkipCounter()
{
  // This ensures that the first frame after initialization is always drawn, regardless of the frame-skip ratio
  _frameSkipCounter = 0;
}

ULO HostRenderer::GetFramecounter() const
{
  return _framecounter;
}

void HostRenderer::AdvanceFramecounter()
{
  _framecounter++;
}

void HostRenderer::InitializeFramecounter()
{
  _framecounter = 0;
}

unsigned int HostRenderer::GetDrawBufferIndex() const
{
  return _buffer_draw;
}

const RectShresi &HostRenderer::GetChipsetBufferMaxClip() const
{
  return _chipsetBufferRuntimeSettings.MaxClip;
}

void HostRenderer::Configure(const HostRenderConfiguration &hostRenderConfiguration)
{
  _hostRenderConfiguration = hostRenderConfiguration;

  drawInterlaceConfigure(_hostRenderConfiguration.Deinterlace, _hostRenderConfiguration.ScaleStrategy);
}

// Only used by RP
void HostRenderer::SetChipsetBufferMaxClip(const RectShresi &chipsetBufferMaxClip)
{
  Service->Log.AddLogDebug(
      "SetChipsetMaxClip(shresi rect left=%u, top=%u, right=%u, bottom=%u)\n", chipsetBufferMaxClip.Left, chipsetBufferMaxClip.Top, chipsetBufferMaxClip.Right, chipsetBufferMaxClip.Bottom);

  _chipsetBufferRuntimeSettings.MaxClip = chipsetBufferMaxClip;
}

// Only used by RP
void HostRenderer::SetHostOutputClip(const RectShresi &hostOutputClip)
{
  Service->Log.AddLogDebug("SetHostOutputClip(shresi rect left=%u, top=%u, right=%u, bottom=%u)\n", hostOutputClip.Left, hostOutputClip.Top, hostOutputClip.Right, hostOutputClip.Bottom);

  _hostRenderConfiguration.ChipsetOutputClip = hostOutputClip;
}

void HostRenderer::InitializeMaxClip()
{
  auto &maxClip = _chipsetBufferRuntimeSettings.MaxClip;

  if (DiagnosticFeatures.ShowBlankedArea)
  {
    maxClip.Left = 18 * 4;
    maxClip.Top = 0 * 2;
    maxClip.Right = 472 * 4;
    maxClip.Bottom = 314 * 2;
  }
  else
  {
    maxClip.Left = 88 * 4;
    maxClip.Top = 26 * 2;
    maxClip.Right = 472 * 4;
    maxClip.Bottom = 314 * 2;
  }
}

const DisplayMode *HostRenderer::GetFirstDisplayMode() const
{
  return _displayModes.empty() ? nullptr : &_displayModes.front();
}

void HostRenderer::ClearDisplayModeList()
{
  _displayModes.clear();
}

const DisplayMode *HostRenderer::FindDisplayMode(unsigned int width, unsigned int height, unsigned int bits, unsigned int refresh, bool allowAnyRefresh) const
{
  auto item_iterator = ranges::find_if(_displayModes, [width, height, bits, refresh, allowAnyRefresh](const DisplayMode &dm) {
    return (dm.Width == width) && (dm.Height == height) && (dm.Bits == bits) && (allowAnyRefresh || (dm.Refresh == refresh));
  });

  return (item_iterator != _displayModes.end()) ? &(*item_iterator) : nullptr;
}

const list<DisplayMode> &HostRenderer::GetDisplayModes() const
{
  return _displayModes;
}

bool HostRenderer::RestartGraphicsDriver(DISPLAYDRIVER displaydriver)
{
  Driver->Graphics.Shutdown();
  ClearDisplayModeList();
  bool startupOk = Driver->Graphics.Startup(displaydriver);
  if (startupOk)
  {
    _displayModes = Driver->Graphics.GetDisplayModeList();
  }

  return startupOk;
}

void HostRenderer::SetFullscreenMode(const DimensionsUInt &size, DisplayColorDepth colorDepth, unsigned int refreshRate)
{
  unsigned int width = size.Width;
  unsigned int height = size.Height;
  unsigned int colorbits;

  switch (colorDepth)
  {
    case DisplayColorDepth::Color16Bits: colorbits = 16; break;
    case DisplayColorDepth::Color24Bits: colorbits = 24; break;
    case DisplayColorDepth::Color32Bits: colorbits = 32; break;
    default: F_ASSERT(false); break;
  }

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode())
  {
    height = RETRO_PLATFORM_MAX_PAL_LORES_HEIGHT * 2;
    width = RETRO_PLATFORM_MAX_PAL_LORES_WIDTH * 2;
  }
#endif

  // Find with exact refresh
  const DisplayMode *mode_found = FindDisplayMode(width, height, colorbits, refreshRate, false);
  if (mode_found == nullptr)
  {
    // Fallback to ignore refresh
    mode_found = FindDisplayMode(width, height, colorbits, refreshRate, true);
  }

  if (mode_found != nullptr)
  {
    _hostRenderRuntimeSettings.DisplayMode = *mode_found;
  }
  else
  {
    // No match, take the first in the list
    _hostRenderRuntimeSettings.DisplayMode = *GetFirstDisplayMode();
  }
}

void HostRenderer::SetWindowedMode(const DimensionsUInt &size)
{
  _hostRenderRuntimeSettings.DisplayMode.Width = size.Width;
  _hostRenderRuntimeSettings.DisplayMode.Height = size.Height;
  _hostRenderRuntimeSettings.DisplayMode.Bits = 32;
  _hostRenderRuntimeSettings.DisplayMode.Refresh = 0;
  _hostRenderRuntimeSettings.DisplayMode.IsWindowed = true;
  _hostRenderRuntimeSettings.DisplayMode.Id = -1;
  _hostRenderRuntimeSettings.DisplayMode.Name = "Window mode";
}

void HostRenderer::SetDisplayModeForConfiguration()
{
  if (_hostRenderConfiguration.OutputKind == DisplayOutputKind::Fullscreen)
  {
    SetFullscreenMode(_hostRenderConfiguration.Size, _hostRenderConfiguration.ColorDepth, _hostRenderConfiguration.RefreshRate);
  }
  else
  {
    SetWindowedMode(_hostRenderConfiguration.Size);
  }
}

unsigned int HostRenderer::GetAutomaticInternalScaleFactor() const
{
  return (_hostRenderConfiguration.Size.Width < 1280) ? 1 : 2;
}

// Determines scale factor, indirectly the size, of the backbuffer for the screen rendered by the core of the emulator
// Scale is limited to either 1X (hires sized, approx. 768x576) or 2X (shres sized, approx 1536x1152)
// Host output with scale beyond 2X has to be handled by the host rendering system based on this image.
unsigned int HostRenderer::GetChipsetBufferScaleFactor() const
{
  if (_hostRenderConfiguration.Scale == DisplayScale::Automatic)
  {
    return GetAutomaticInternalScaleFactor();
  }

  return _hostRenderConfiguration.Scale == DisplayScale::FixedScale1X ? 1 : 2;
}

// When display scale is auto, this value can not be used to determine actual scale in the host
// In that case, each renderer will scale the output clip area from the internal backbuffer to whatever size the host window has
// For Automatic scaling, this function is undefined
unsigned int HostRenderer::GetOutputScaleFactor() const
{
  if (_hostRenderConfiguration.Scale == DisplayScale::Automatic)
  {
    F_ASSERT(false);
  }

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode())
  {
    // TODO: Verify value
    return RP.GetDisplayScale() * 2;
  }
#endif

  unsigned int outputScaleFactor = 1;

  switch (_hostRenderConfiguration.Scale)
  {
    case DisplayScale::FixedScale1X: outputScaleFactor = 1; break;
    case DisplayScale::FixedScale2X: outputScaleFactor = 2; break;
    case DisplayScale::FixedScale3X: outputScaleFactor = 3; break;
    case DisplayScale::FixedScale4X: outputScaleFactor = 4; break;
    default: F_ASSERT(false); break;
  }

  return outputScaleFactor;
}

unsigned int HostRenderer::GetBufferCount() const
{
  return _buffer_count;
}

// Calculate pixel area in the chipset buffer that corresponds to the shresi coordinates for the output area
// Chipset output clip area must have been set first
// Also depends on the host window scale factor
//   1 -> chipset buffer is allocated as hires (640x512 kind)
//   2 -> chipset buffer is allocated as shres (1280x1024 kind)
void HostRenderer::CalculateOutputClipPixels()
{
  const unsigned int chipsetBufferScaleFactor = GetChipsetBufferScaleFactor();
  unsigned int outputClipLeft, outputClipTop, outputClipWidth, outputClipHeight;

  const auto &outputClip = _hostRenderConfiguration.ChipsetOutputClip;
  const auto &maxClip = _chipsetBufferRuntimeSettings.MaxClip;

#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
  {
#endif
    if (chipsetBufferScaleFactor == 1)
    {
      outputClipLeft = (outputClip.Left - maxClip.Left) / 2;
      outputClipTop = outputClip.Top - maxClip.Top;
      outputClipWidth = outputClip.GetWidth() / 2;
      outputClipHeight = outputClip.GetHeight();
    }
    else
    {
      outputClipLeft = outputClip.Left - maxClip.Left;
      outputClipTop = (outputClip.Top - maxClip.Top) * chipsetBufferScaleFactor;
      outputClipWidth = outputClip.GetWidth();
      outputClipHeight = outputClip.GetHeight() * chipsetBufferScaleFactor;
    }
#ifdef RETRO_PLATFORM
  }
  else
  {
    // TODO: Review these numbers, certainly very wrong now.
    outputClipLeft = outputClip.Left - (maxClip.Left * chipsetBufferScaleFactor);
    outputClipTop = outputClip.Top - (maxClip.Top * chipsetBufferScaleFactor);
    outputClipWidth = outputClip.GetWidth();
    outputClipHeight = outputClip.GetHeight();
  }
#endif

  _chipsetBufferRuntimeSettings.OutputClipPixels = RectPixels(outputClipLeft, outputClipTop, outputClipLeft + outputClipWidth, outputClipTop + outputClipHeight);
}

//==============================================
// Selects drawing routines for the current mode
//==============================================

void HostRenderer::DrawModeFunctionTablesInitialize(const unsigned int activeBufferColorBits) const
{
  // Currently located in pixelrenderers
  drawModeFunctionsInitialize(activeBufferColorBits, GetChipsetBufferScaleFactor(), _hostRenderConfiguration.ScaleStrategy);

  // Re-initializes all internal pointers to drawing functions, based on current BPLCON0 values
  if (chipset_info.IsCycleExact)
  {
    bitplane_shifter.UpdateDrawBatchFunc();
  }
  else
  {
    graphNotifyBplCon0Changed();
  }
}

//===========================================
// Tells gfx drv to publish the current frame
//===========================================

void HostRenderer::FlipBuffer()
{
  if (++_buffer_show >= _buffer_count)
  {
    _buffer_show = 0;
  }
  if (++_buffer_draw >= _buffer_count)
  {
    _buffer_draw = 0;
  }

  Driver->Graphics.BufferFlip();
}

const uint64_t *HostRenderer::GetHostColors() const
{
  return _hostColor;
}

uint64_t HostRenderer::GetHostColorForColor12Bit(const uint16_t color12Bit) const
{
  return _hostColorMap[color12Bit];
}

// Not the best place for this, this is host buffer related. Should introduce a domain event that informs draw about color change and
// lets it do whatever it needs to do
void HostRenderer::InitializeHostColorMap(const GfxDrvColorBitsInformation &colorBitsInformation)
{
  for (uint32_t color12Bit = 0; color12Bit < 4096; color12Bit++)
  {
    const uint32_t r = ((color12Bit & 0xf00) >> 8) << (colorBitsInformation.RedPosition + colorBitsInformation.RedSize - 4);
    const uint32_t g = ((color12Bit & 0xf0) >> 4) << (colorBitsInformation.GreenPosition + colorBitsInformation.GreenSize - 4);
    const uint32_t b = (color12Bit & 0xf) << (colorBitsInformation.BluePosition + colorBitsInformation.BlueSize - 4);

    _hostColorMap[color12Bit] = (uint64_t)(r | g | b);

    if (colorBitsInformation.ColorBits <= 16)
    {
      _hostColorMap[color12Bit] |= (_hostColorMap[color12Bit] << 48 | _hostColorMap[color12Bit] << 32 | _hostColorMap[color12Bit] << 16);
    }
    else if (colorBitsInformation.ColorBits == 32)
    {
      _hostColorMap[color12Bit] |= _hostColorMap[color12Bit] << 32;
    }
  }

  for (unsigned int colorRegisterIndex = 0; colorRegisterIndex < 64; colorRegisterIndex++)
  {
    _hostColor[colorRegisterIndex] = _hostColorMap[bitplane_registers.color[colorRegisterIndex]];
  }
}

void HostRenderer::ColorChangedHandler(const unsigned int colorIndex, const UWO color12Bit, const UWO halfbriteColor12Bit)
{
  _hostColor[colorIndex] = _hostColorMap[color12Bit];
  _hostColor[colorIndex + 32] = _hostColorMap[halfbriteColor12Bit];
}

//============================================================================
// Validates the buffer pointer
//
// lineNumber is in shresi coordinate system, ie. 0-627
//
// Values set in draw_buffer_info:
//
// top_ptr points to the absolute top of the framebuffer
//
// buffer_line_pitch is size in bytes to advance a line of pixels
// frame_line_pitch is size in bytes to advance an Amiga (314p) line (scale factor applied)
//============================================================================

MappedChipsetFramebuffer HostRenderer::MapChipsetFramebuffer()
{
  GfxDrvMappedBufferPointer mappedBufferPointer = Driver->Graphics.MapChipsetFramebuffer();

  if (!mappedBufferPointer.IsValid)
  {
    Service->Log.AddLog("Host buffer ptr is nullptr\n");
    return MappedChipsetFramebuffer{.IsValid = false};
  }

  // Internal scale 1X - interlaced line is 1 buffer lines
  // Internal scale 2X - interlaced line is 2 buffer lines

  return MappedChipsetFramebuffer{
      .TopPointer = mappedBufferPointer.Buffer,
      .HostLinePitch = mappedBufferPointer.Pitch,
      .AmigaLinePitch = mappedBufferPointer.Pitch * GetChipsetBufferScaleFactor() * 2, // Pitch to go to next line in field
      .IsValid = true};
}

//=================================
// Unmaps the buffer memory pointer
//=================================

void HostRenderer::UnmapChipsetFramebuffer()
{
  Driver->Graphics.UnmapChipsetFramebuffer();
}

void HostRenderer::SetClearAllBuffersFlag()
{
  _clear_buffers = GetBufferCount();
}

void HostRenderer::CalculateChipsetBufferSize()
{
  const unsigned int scaleFactor = GetChipsetBufferScaleFactor();
  const auto &maxClip = _chipsetBufferRuntimeSettings.MaxClip;

  _chipsetBufferRuntimeSettings.Dimensions.Width = (maxClip.GetWidth() * scaleFactor) / 2;
  _chipsetBufferRuntimeSettings.Dimensions.Height = maxClip.GetHeight() * scaleFactor;
  _chipsetBufferRuntimeSettings.ScaleFactor = scaleFactor;
}

//=============================
// Called on every end of frame
//=============================

void HostRenderer::HardReset()
{
}

//==========================
// Called on emulation start
//==========================

bool HostRenderer::EmulationStart()
{
  Service->Log.AddLog("HostRenderer::EmulationStart()\n");
  _hostRenderConfiguration.Log();

  SetDisplayModeForConfiguration();
  InitializeFrameSkipCounter();
  CalculateChipsetBufferSize();
  CalculateOutputClipPixels();
  _hostRenderRuntimeSettings.OutputScaleFactor = GetOutputScaleFactor();

  _hostRenderStatistics.FpsStatsClear();

  _hostRenderRuntimeSettings.Log();

  return Driver->Graphics.EmulationStart(_hostRenderConfiguration, _hostRenderRuntimeSettings, _chipsetBufferRuntimeSettings, &_hudPropertyProvider);
}

bool HostRenderer::EmulationStartPost()
{
  _buffer_count = Driver->Graphics.EmulationStartPost(_chipsetBufferRuntimeSettings);

  if (_buffer_count == 0)
  {
    const char *errorMessage = "Failure: The graphics driver failed to start. See fellow.log for more details.";
    Service->Log.AddLog(errorMessage);
    Driver->Gui.Requester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, errorMessage);
    return false;
  }

  _buffer_show = 0;
  _buffer_draw = _buffer_count - 1;

  // Color bit layout for the buffers are needed to finalize tables for the drawing. DirectDraw cannot provide this until after the ...post, dxgi does provide it earlier as it is fixed to 32-bit
  // backbuffer
  const GfxDrvColorBitsInformation colorBitsInformation = Driver->Graphics.GetColorBitsInformation();
  _chipsetBufferRuntimeSettings.ColorBits = colorBitsInformation.ColorBits;

  // Remember also clear HostColor() on reset
  InitializeHostColorMap(colorBitsInformation);
  drawHAMTableInit(colorBitsInformation);

  DrawModeFunctionTablesInitialize(_chipsetBufferRuntimeSettings.ColorBits);

  Service->Log.AddLog(
      "drawEmulationStartPost(): Chipset buffer is (%u,%u,%u)\n", _chipsetBufferRuntimeSettings.Dimensions.Width, _chipsetBufferRuntimeSettings.Dimensions.Height, colorBitsInformation.ColorBits);

  return true;
}

//=========================
// Called on emulation halt
//=========================

void HostRenderer::EmulationStop()
{
  Driver->Graphics.EmulationStop();
}

//===========================
// Called on emulator startup
//===========================

bool HostRenderer::Startup()
{
  // This config contains default values with command line parameters applied
  auto startupConfig = cfgManagerGetCurrentConfig(&cfg_manager);

  ClearDisplayModeList();
  if (!Driver->Graphics.Startup(cfgGetDisplayDriver(startupConfig)))
  {
    return false;
  }
  DualPlayfieldMapper::InitializeMappingTable();

#ifdef RETRO_PLATFORM
  if (!RP.GetHeadlessMode())
#endif
  {
    // RP sets different values
    InitializeMaxClip();
    SetHostOutputClip(_chipsetBufferRuntimeSettings.MaxClip);
  }

  InitializeFramecounter();
  _clear_buffers = 0;

  drawInterlaceStartup();

  bitplane_registers.SetColorChangeEventHandler(this);
  return true;
}

/*============================================================================*/
/* Things uninitialized at emulator shutdown                                  */
/*============================================================================*/

void HostRenderer::Shutdown()
{
  ClearDisplayModeList();
  Driver->Graphics.Shutdown();
}

void HostRenderer::UpdateDrawFunctions()
{
  drawSetLineRoutine(draw_line_BG_routine);
  if (graph_playfield_on)
  {
    // check if bit 8 of register dmacon is 1; check if bitplane DMA is enabled
    // check if bit 12, 13 and 14 of register bplcon0 is 1;
    // check if atleast one bitplane is active
    if (((dmacon & 0x0100) == 0x0100) && ((bitplane_registers.bplcon0 & 0x7000) > 0x0001))
    {
      drawSetLineRoutine(draw_line_BPL_manage_routine);
    }
  }
}

void HostRenderer::ReinitializeRendering()
{
  DrawModeFunctionTablesInitialize(_chipsetBufferRuntimeSettings.ColorBits);
  graphLineDescClear();
}

void HostRenderer::DrawFrameInHostLineExact(const MappedChipsetFramebuffer &mappedChipsetFramebuffer)
{
  // chipsetClipMax top and bottom are always pre-aligned to producing full Amiga lines, ie. they are even to avoid the complexity of splitting Amiga lines in half during rendering phase.
  // chipsetClipOutput can be anything within chipsetClipMax.

  const unsigned int chipsetClipTop = _chipsetBufferRuntimeSettings.MaxClip.Top;
  const unsigned int chipsetClipHeight = _chipsetBufferRuntimeSettings.MaxClip.GetHeight();

  const unsigned int fieldStartLine = chipsetClipTop / 2;
  const unsigned int fieldHeight = chipsetClipHeight / 2;

  uint8_t *framebuffer = mappedChipsetFramebuffer.TopPointer;

  // If deinterlacing and drawing the short field, add one Amiga interlace line
  if (drawGetUseInterlacedRendering() && !drawGetFrameIsLong())
  {
    framebuffer += mappedChipsetFramebuffer.AmigaLinePitch / 2;
  }

  for (unsigned int y = 0; y < fieldHeight; ++y)
  {
    graph_line *graph_line_ptr = graphGetLineDesc(_buffer_draw, fieldStartLine + y);

    if (graph_line_ptr != nullptr)
    {
      if (graph_line_ptr->linetype != graph_linetypes::GRAPH_LINE_SKIP && graph_line_ptr->linetype != graph_linetypes::GRAPH_LINE_BPL_SKIP)
      {
        graph_line_ptr->draw_line_routine(graph_line_ptr, framebuffer, mappedChipsetFramebuffer.HostLinePitch);
      }

      graph_line_ptr->sprite_ham_slot = 0xffffffff;
    }

    framebuffer += mappedChipsetFramebuffer.AmigaLinePitch;
  }
}

/*==============================================================================*/
/* Drawing end of frame handler                                                 */
/*==============================================================================*/

void HostRenderer::EndOfFrame()
{
  if (!ShouldSkipFrame())
  {
    if (_clear_buffers > 0)
    {
      Driver->Graphics.ClearCurrentBuffer();
      --_clear_buffers;
    }

    const unsigned int chipsetClipTop = _chipsetBufferRuntimeSettings.MaxClip.Top;

    auto mappedFramebuffer = MapChipsetFramebuffer();

    if (mappedFramebuffer.IsValid)
    {
      // Renders the entire Amiga pixel area into a host provided buffer as defined by the chipset clip max limits.
      // This represents the entire overscan area and more than most monitors would ever display.
      // Actual visible host output might be a sub-set, as defined by draw_clip_amiga (Amiga coordinates) and draw_buffer_clip (Same area in absolute pixels within the buffer)

      // Renders Amiga screen in the chipset buffer
      if (chipsetIsCycleExact())
      {
        host_frame_copier.DrawFrameInHost(mappedFramebuffer, GetChipsetBufferScaleFactor(), _hostRenderConfiguration.ChipsetOutputClip, GetHostColors()[0]);
      }
      else
      {
        DrawFrameInHostLineExact(mappedFramebuffer);
      }

      // Legacy HUD renders with CPU into the chipset buffer and requires the mapped framebuffer pointer
      DrawHUD(mappedFramebuffer);
      UnmapChipsetFramebuffer();

      _hostRenderStatistics.FpsStatsTimestamp();

      // Direct2D HUD renders with graphics commands into the D3D11 pipeline and does not require a mapped framebuffer pointer (nor does it alter the chipset buffer)

      FlipBuffer();
    }
  }

  AdvanceFramecounter();
  AdvanceFrameSkipCounter();
}

void HostRenderer::DrawHUD(const MappedChipsetFramebuffer &mappedChipsetFramebuffer)
{
  Driver->Graphics.DrawHUD(mappedChipsetFramebuffer);
}

HostRenderer::HostRenderer() : _hudPropertyProvider(_hostRenderStatistics)
{
}

HostRenderer Draw;
