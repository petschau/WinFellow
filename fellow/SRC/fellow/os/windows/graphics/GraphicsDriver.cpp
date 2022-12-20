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
  return _gfxDrvCommon->IsHostBufferWindowed();
}

HWND GraphicsDriver::GetHWND()
{
  return _gfxDrvCommon->GetHWND();
}

void GraphicsDriver::SetPauseEmulationWhenWindowLosesFocus(bool pause)
{
  _gfxDrvCommon->SetPauseEmulationWhenWindowLosesFocus(pause);
}

bool GraphicsDriver::GetDisplayChange() const
{
  return _gfxDrvCommon->GetDisplayChange();
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
  _gfxDrvCommon->Flip();

  if (_use_dxgi)
  {
    _gfxDrvDXGI->Flip();
  }
  else
  {
    _gfxDrvDirectDraw->Flip();
  }

  _gfxDrvCommon->RunEventWait();
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
  _gfxDrvCommon->SizeChanged(width, height);

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
  if (!_gfxDrvCommon->EmulationStart(hostRenderConfiguration, hostRenderRuntimeSettings.DisplayMode))
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
  _gfxDrvCommon->EmulationStartPost();

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

  _gfxDrvCommon->EmulationStop();
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

// Called when the application starts up
bool GraphicsDriver::Startup(DISPLAYDRIVER displaydriver)
{
  _use_dxgi = (displaydriver == DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11);

  _gfxDrvCommon = new GfxDrvCommon();

  if (!_gfxDrvCommon->Startup())
  {
    return false;
  }

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode())
  {
    _gfxDrvCommon->rp_startup_config = cfgManagerGetCurrentConfig(&cfg_manager);
    RP.RegisterRetroPlatformScreenMode(true);
  }
#endif

  if (_use_dxgi)
  {
    if (!ValidateRequirements())
    {
      Service->Log.AddLog("gfxDrv ERROR: Direct3D requirements not met, falling back to DirectDraw.\n");
      _use_dxgi = false;
      _gfxDrvDirectDraw = new GfxDrvDirectDraw();
      return _gfxDrvDirectDraw->Startup();
    }

    _gfxDrvDXGI = new GfxDrvDXGI();
    bool dxgiStartupResult = _gfxDrvDXGI->Startup();
    if (dxgiStartupResult)
    {
      return true;
    }
    Service->Log.AddLog("gfxDrv ERROR: Direct3D present but no adapters found, falling back to DirectDraw.\n");
    _use_dxgi = false;
  }

  _gfxDrvDirectDraw = new GfxDrvDirectDraw();
  return _gfxDrvDirectDraw->Startup();
}

// Called when the application is closed
void GraphicsDriver::Shutdown()
{
  if (_use_dxgi)
  {
    if (_gfxDrvDXGI != nullptr)
    {
      delete _gfxDrvDXGI;
      _gfxDrvDXGI = nullptr;
    }
  }
  else
  {
    _gfxDrvDirectDraw->Shutdown();
    if (_gfxDrvDirectDraw != nullptr)
    {
      delete _gfxDrvDirectDraw;
      _gfxDrvDirectDraw = nullptr;
    }
  }

  if (_gfxDrvCommon != nullptr)
  {
    delete _gfxDrvCommon;
    _gfxDrvCommon = nullptr;
  }
}

bool GraphicsDriver::ValidateRequirements()
{
  return GfxDrvDXGI::ValidateRequirements();
}
