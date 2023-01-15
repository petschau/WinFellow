#pragma once

#include "fellow/api/drivers/IGraphicsDriver.h"
#include "fellow/os/windows/graphics/GfxDrvCommon.h"

class GfxDrvDirectDraw;
class GfxDrvDXGI;

class GraphicsDriver : public IGraphicsDriver
{
private:
  bool _use_dxgi = false;
  bool _isInitialized = false;
  std::list<DISPLAYDRIVER> _listOfInitializedDrivers;

  GfxDrvCommon _gfxDrvCommon;
  GfxDrvDirectDraw *_gfxDrvDirectDraw = nullptr;
  GfxDrvDXGI *_gfxDrvDXGI = nullptr;

  bool InitializeDXGI();
  void ReleaseDXGI();

  bool InitializeDirectDraw();
  void ReleaseDirectDraw();

  bool InitializeCommon(fellow::api::IRuntimeEnvironment *runtimeEnvironment);
  void ReleaseCommon();

  void SwitchGraphicsDriver(DISPLAYDRIVER displaydriver);

public:
  // The windows os drivers depends on some shared information about the window such as HWND or fullscreen/window mode
  // The functions provide that

#ifdef RETRO_PLATFORM
  cfg *rp_startup_config = nullptr;
#endif

  bool IsHostBufferWindowed() const;
  HWND GetHWND();
  void SetPauseEmulationWhenWindowLosesFocus(bool pause);
  bool GetPauseEmulationWhenWindowLosesFocus();
  bool GetDisplayChange() const;
  const DimensionsUInt &GetHostBufferSize() const;
  void HideWindow();
  void RunEventSet();
  void RunEventReset();
  bool SwitchDisplayDriver(DISPLAYDRIVER displayDriver);

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

  bool EmulationStart(
      const HostRenderConfiguration &hostRenderConfiguration,
      const HostRenderRuntimeSettings &hostRenderRuntimeSettings,
      const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings,
      HudPropertyProvider *hudPropertyProvider) override;
  ULO EmulationStartPost(const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings) override;
  void EmulationStop() override;

  std::list<DISPLAYDRIVER> GetListOfInitializedDrivers() const override;
  bool IsInitialized() const override;
  void Initialize(fellow::api::IRuntimeEnvironment *runtimeEnvironment) override;
  void Release() override;

  virtual ~GraphicsDriver() = default;
};
