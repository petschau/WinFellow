#pragma once

#include <ddraw.h>
#include <list>

#include "fellow/api/defs.h"
#include "fellow/hud/HudPropertyProvider.h"
#include "fellow/application/GfxDrvMappedBufferPointer.h"
#include "fellow/application/GfxDrvColorBitsInformation.h"
#include "MappedChipsetFramebuffer.h"
#include "fellow/application/DisplayMode.h"
#include "ChipsetBufferRuntimeSettings.h"
#include "fellow/application/HostRenderConfiguration.h"
#include "GfxDrvHudLegacy.h"
#include "fellow/os/windows/directdraw/DirectDrawDevice.h"

class GfxDrvDirectDraw
{
private:
  DirectDrawDevice *_ddrawDeviceCurrent{};
  std::list<DirectDrawDevice *> _ddrawDevices;
  GfxDrvHudLegacy *_legacyHud{};

  bool _initialized;

  void LogDirectDrawDeviceInformation();
  bool DirectDrawDeviceInformationInitialize();
  void DirectDrawDeviceInformationRelease();

  bool Initialize();
  void Release();

  void InitializeLegacyHud(HudPropertyProvider *hudPropertyProvider, const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings);
  void ReleaseLegacyHud();

public:
  BOOL EnumerateDirectDrawDevice(GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName);
  std::list<DisplayMode> GetDisplayModeList();

  bool SaveScreenshot(bool bTakeFilteredScreenshot, const char *filename);

  void ClearCurrentBuffer();

  void SizeChanged(unsigned int width, unsigned int height);
  void PositionChanged();

  GfxDrvMappedBufferPointer MapChipsetFramebuffer();
  void UnmapChipsetFramebuffer();

  GfxDrvColorBitsInformation GetColorBitsInformation();

  void Flip();

  void RenderHud(const MappedChipsetFramebuffer &mappedFramebuffer);

  bool EmulationStart(
      const HostRenderConfiguration &hostRenderConfiguration,
      const HostRenderRuntimeSettings &hostRenderRuntimeSettings,
      const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings,
      HudPropertyProvider *hudPropertyProvider);
  unsigned int EmulationStartPost(const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings, HWND hwnd);
  void EmulationStop();

  bool Startup();
  void Shutdown();
};
