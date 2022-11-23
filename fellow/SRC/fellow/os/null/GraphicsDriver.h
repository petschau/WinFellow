#pragma once

#include "fellow/api/drivers/IGraphicsDriver.h"

class GraphicsDriver : public IGraphicsDriver
{
  public:
  void ClearCurrentBuffer() override;
  void BufferFlip() override;
  void SizeChanged(unsigned int width, unsigned int height) override;
  void PositionChanged() override;
  GfxDrvMappedBufferPointer MapChipsetFramebuffer() override;
  void UnmapChipsetFramebuffer() override;
  GfxDrvColorBitsInformation GetColorBitsInformation() override;
  std::list<DisplayMode> GetDisplayModeList() override;
  void DrawHUD(const MappedChipsetFramebuffer &mappedFramebuffer) override;
  void NotifyActiveStatus(bool active) override;
  bool SaveScreenshot(const bool, const char *filename) override;
  bool ValidateRequirements() override;
  
  bool EmulationStart(
    const HostRenderConfiguration &hostRenderConfiguration,
    const HostRenderRuntimeSettings &hostRenderRuntimeSettings,
    const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings,
    HudPropertyProvider *hudPropertyProvider) override;
  ULO EmulationStartPost(const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings) override;
  void EmulationStop() override;
  bool Startup(DISPLAYDRIVER displaydriver) override;
  void Shutdown() override;
};