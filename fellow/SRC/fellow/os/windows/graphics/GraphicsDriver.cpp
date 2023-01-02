#include "fellow/os/windows/graphics/GraphicsDriver.h"
#include "fellow/os/windows/graphics/GfxDrvCommon.h"
#include "fellow/os/windows/dxgi/GfxDrvDXGI.h"
#include "fellow/os/windows/directdraw/GfxDrvDirectDraw.h"
#include "fellow/api/Services.h"
#include "fellow/hud/HudPropertyProvider.h"

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif

using namespace std;
using namespace fellow::api;

bool GraphicsDriver::IsHostBufferWindowed() const
{
  return _gfxDrvCommon.IsHostBufferWindowed();
}

HWND GraphicsDriver::GetHWND()
{
  return _gfxDrvCommon.GetHWND();
}

void GraphicsDriver::SetPauseEmulationWhenWindowLosesFocus(bool pause)
{
  _gfxDrvCommon.SetPauseEmulationWhenWindowLosesFocus(pause);
}

bool GraphicsDriver::GetPauseEmulationWhenWindowLosesFocus()
{
  return _gfxDrvCommon.GetPauseEmulationWhenWindowLosesFocus();
}

bool GraphicsDriver::GetDisplayChange() const
{
  return _gfxDrvCommon.GetDisplayChange();
}

const DimensionsUInt &GraphicsDriver::GetHostBufferSize() const
{
  return _gfxDrvCommon.GetHostBufferSize();
}

void GraphicsDriver::HideWindow()
{
  return _gfxDrvCommon.HideWindow();
}

void GraphicsDriver::RunEventSet()
{
  _gfxDrvCommon.RunEventSet();
}

void GraphicsDriver::RunEventReset()
{
  _gfxDrvCommon.RunEventReset();
}

void GraphicsDriver::ClearCurrentBuffer()
{
  if (_use_dxgi)
  {
    _gfxDrvDXGI->ClearCurrentBuffer();
  }
  else
  {
    _gfxDrvDirectDraw->ClearCurrentBuffer();
  }
}

GfxDrvMappedBufferPointer GraphicsDriver::MapChipsetFramebuffer()
{
  if (_use_dxgi)
  {
    return _gfxDrvDXGI->MapChipsetFramebuffer();
  }

  return _gfxDrvDirectDraw->MapChipsetFramebuffer();
}

void GraphicsDriver::UnmapChipsetFramebuffer()
{
  if (_use_dxgi)
  {
    _gfxDrvDXGI->UnmapChipsetFramebuffer();
  }
  else
  {
    _gfxDrvDirectDraw->UnmapChipsetFramebuffer();
  }
}

void GraphicsDriver::BufferFlip()
{
  _gfxDrvCommon.Flip();

  if (_use_dxgi)
  {
    _gfxDrvDXGI->Flip();
  }
  else
  {
    _gfxDrvDirectDraw->Flip();
  }

  _gfxDrvCommon.RunEventWait();
}

void GraphicsDriver::DrawHUD(const MappedChipsetFramebuffer &mappedFramebuffer)
{
  if (_use_dxgi)
  {
  }
  else
  {
    _gfxDrvDirectDraw->RenderHud(mappedFramebuffer);
  }
}

void GraphicsDriver::NotifyActiveStatus(bool active)
{
  if (_use_dxgi)
  {
    _gfxDrvDXGI->NotifyActiveStatus(active);
  }
}

void GraphicsDriver::SizeChanged(unsigned int width, unsigned int height)
{
  _gfxDrvCommon.SizeChanged(width, height);

  if (_use_dxgi)
  {
    _gfxDrvDXGI->SizeChanged(width, height);
  }
  else
  {
    _gfxDrvDirectDraw->SizeChanged(width, height);
  }
}

void GraphicsDriver::PositionChanged()
{
  if (_use_dxgi)
  {
    _gfxDrvDXGI->PositionChanged();
  }
  else
  {
    _gfxDrvDirectDraw->PositionChanged();
  }
}

GfxDrvColorBitsInformation GraphicsDriver::GetColorBitsInformation()
{
  if (_use_dxgi)
  {
    return _gfxDrvDXGI->GetColorBitsInformation();
  }

  return _gfxDrvDirectDraw->GetColorBitsInformation();
}

list<DisplayMode> GraphicsDriver::GetDisplayModeList()
{
  if (_use_dxgi)
  {
    return _gfxDrvDXGI->GetDisplayModeList();
  }

  return _gfxDrvDirectDraw->GetDisplayModeList();
}

bool GraphicsDriver::EmulationStart(
    const HostRenderConfiguration &hostRenderConfiguration,
    const HostRenderRuntimeSettings &hostRenderRuntimeSettings,
    const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings,
    HudPropertyProvider *hudPropertyProvider)
{
  if (!_gfxDrvCommon.EmulationStart(hostRenderConfiguration, hostRenderRuntimeSettings.DisplayMode))
  {
    return false;
  }

  if (_use_dxgi)
  {
    return _gfxDrvDXGI->EmulationStart(hostRenderConfiguration, chipsetBufferRuntimeSettings, hostRenderRuntimeSettings.DisplayMode, hudPropertyProvider);
  }

  return _gfxDrvDirectDraw->EmulationStart(hostRenderConfiguration, hostRenderRuntimeSettings, chipsetBufferRuntimeSettings, hudPropertyProvider);
}

ULO GraphicsDriver::EmulationStartPost(const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings)
{
  _gfxDrvCommon.EmulationStartPost();

  if (_use_dxgi)
  {
    return _gfxDrvDXGI->EmulationStartPost();
  }

  return _gfxDrvDirectDraw->EmulationStartPost(chipsetBufferRuntimeSettings, GetHWND());
}

void GraphicsDriver::EmulationStop()
{
  if (_use_dxgi)
  {
    _gfxDrvDXGI->EmulationStop();
  }
  else
  {
    _gfxDrvDirectDraw->EmulationStop();
  }

  _gfxDrvCommon.EmulationStop();
}

bool GraphicsDriver::SaveScreenshot(const bool bSaveFilteredScreenshot, const char *szFilename)
{
  STR szActualFilename[MAX_PATH] = "";
  bool result = false;

  if (szFilename[0] == 0)
  {
    // filename not set, automatically generate one
    Service->Fileops.GetScreenshotFileName(szActualFilename);
  }
  else
  {
    strcpy(szActualFilename, szFilename);
  }

  if (_use_dxgi)
  {
    result = _gfxDrvDXGI->SaveScreenshot(bSaveFilteredScreenshot, szActualFilename);
  }
  else
  {
    result = _gfxDrvDirectDraw->SaveScreenshot(bSaveFilteredScreenshot, szActualFilename);
  }

  Service->Log.AddLog("gfxDrvSaveScreenshot(filtered=%s, filename=%s) %s.\n", bSaveFilteredScreenshot ? "true" : "false", szActualFilename, result ? "successful" : "failed");

  return result;
}

bool GraphicsDriver::SwitchDisplayDriver(DISPLAYDRIVER displaydriver)
{
  Release();
  InitializeGraphicsDriver(displaydriver);
  return _isInitialized;
}

bool GraphicsDriver::InitializeDXGI()
{
  if (!ValidateRequirements())
  {
    Service->Log.AddLog("gfxDrv ERROR: Direct3D requirements not met, falling back to DirectDraw.\n");
    return false;
  }

  _gfxDrvDXGI = new GfxDrvDXGI();
  bool result = _gfxDrvDXGI->Startup();
  if (!result)
  {
    Service->Log.AddLog("gfxDrv ERROR: Direct3D present but no adapters found, falling back to DirectDraw.\n");
    delete _gfxDrvDXGI;
    _gfxDrvDXGI = nullptr;
  }

  return result;
}

void GraphicsDriver::ReleaseDXGI()
{
  if (_gfxDrvDXGI != nullptr)
  {
    _gfxDrvDXGI->Shutdown();
    delete _gfxDrvDXGI;
    _gfxDrvDXGI = nullptr;
  }
}

bool GraphicsDriver::InitializeDirectDraw()
{
  _gfxDrvDirectDraw = new GfxDrvDirectDraw();
  bool result = _gfxDrvDirectDraw->Startup();
  if (!result)
  {
    Service->Log.AddLog("gfxDrv ERROR: DirectDraw initialization failed.\n");
    delete _gfxDrvDirectDraw;
    _gfxDrvDirectDraw = nullptr;
  }

  return result;
}

void GraphicsDriver::ReleaseDirectDraw()
{
  if (_gfxDrvDirectDraw != nullptr)
  {
    _gfxDrvDirectDraw->Shutdown();
    delete _gfxDrvDirectDraw;
    _gfxDrvDirectDraw = nullptr;
  }
}

bool GraphicsDriver::InitializeCommon()
{
  _gfxDrvCommon.Initialize();
  return _gfxDrvCommon.IsInitialized();
}

void GraphicsDriver::ReleaseCommon()
{
  _gfxDrvCommon.Release();
}

bool GraphicsDriver::IsInitialized() const
{
  return _isInitialized;
}

void GraphicsDriver::InitializeGraphicsDriver(DISPLAYDRIVER displaydriver)
{
  _isInitialized = InitializeCommon();
  if (!_isInitialized)
  {
    return;
  }

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode())
  {
    rp_startup_config = cfgManagerGetCurrentConfig(&cfg_manager);
    RP.RegisterRetroPlatformScreenMode(true);
  }
#endif

  _use_dxgi = displaydriver == DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11;
  if (_use_dxgi)
  {
    _isInitialized = InitializeDXGI();
    if (_isInitialized)
    {
      return;
    }
  }

  // Fallback or DirectDraw selected
  _use_dxgi = false;
  _isInitialized = InitializeDirectDraw();
  if (!_isInitialized)
  {
    _gfxDrvCommon.Release();
  }
}

void GraphicsDriver::Initialize()
{
  InitializeGraphicsDriver(DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11);
}

void GraphicsDriver::Release()
{
  if (!_isInitialized)
  {
    return;
  }

  ReleaseDXGI();
  ReleaseDirectDraw();
  ReleaseCommon();

  _isInitialized = false;
}

bool GraphicsDriver::ValidateRequirements()
{
  return GfxDrvDXGI::ValidateRequirements();
}
