#pragma once

#include "fellow/api/drivers/IGraphicsDriver.h"

class GfxDrvCommon;
class GfxDrvDirectDraw;
class GfxDrvDXGI;

class GraphicsDriver : public IGraphicsDriver
{
private:
  bool _use_dxgi = false;

  GfxDrvCommon *_gfxDrvCommon = nullptr;
  GfxDrvDirectDraw *_gfxDrvDirectDraw = nullptr;
  GfxDrvDXGI *_gfxDrvDXGI = nullptr;

public:
  // The windows os drivers depends on some shared information about the window such as HWND or fullscreen/window mode
  // The functions provide that
  bool IsHostBufferWindowed() const;
  HWND GetHWND();
  void SetPauseEmulationWhenWindowLosesFocus(bool pause);
  bool GetDisplayChange() const;

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

  virtual ~GraphicsDriver() = default;
};
