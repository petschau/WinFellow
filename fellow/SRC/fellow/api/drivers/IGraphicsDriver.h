#pragma once

#include <list>

#include "fellow/api/defs.h"
#include "fellow/configuration/Configuration.h"
#include "fellow/api/drivers/GfxDrvMappedBufferPointer.h"
#include "fellow/api/drivers/GfxDrvColorBitsInformation.h"
#include "fellow/chipset/MappedChipsetFramebuffer.h"
#include "fellow/application/HostRenderConfiguration.h"
#include "fellow/application/HostRenderRuntimeSettings.h"
#include "fellow/chipset/ChipsetBufferRuntimeSettings.h"
#include "fellow/application/DisplayMode.h"
#include "fellow/hud/HudPropertyProvider.h"

//==================================
// Interface for the graphics driver
//==================================

class IGraphicsDriver
{
  public:
  virtual void ClearCurrentBuffer() = 0;
  virtual void BufferFlip() = 0;
  virtual void SizeChanged(unsigned int width, unsigned int height) = 0;
  virtual void PositionChanged() = 0;
  virtual GfxDrvMappedBufferPointer MapChipsetFramebuffer() = 0;
  virtual void UnmapChipsetFramebuffer() = 0;
  virtual GfxDrvColorBitsInformation GetColorBitsInformation() = 0;
  virtual std::list<DisplayMode> GetDisplayModeList() = 0;
  virtual void DrawHUD(const MappedChipsetFramebuffer &mappedFramebuffer) = 0;
  virtual void NotifyActiveStatus(bool active) = 0;
  virtual bool SaveScreenshot(const bool, const char *filename) = 0;
  virtual bool ValidateRequirements() = 0;
  
  virtual bool EmulationStart(
    const HostRenderConfiguration &hostRenderConfiguration,
    const HostRenderRuntimeSettings &hostRenderRuntimeSettings,
    const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings,
    HudPropertyProvider *hudPropertyProvider) = 0;
  virtual ULO EmulationStartPost(const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings) = 0;
  virtual void EmulationStop() = 0;
  virtual bool Startup(DISPLAYDRIVER displaydriver) = 0;
  virtual void Shutdown() = 0;
};
