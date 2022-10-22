#include "GfxDrvCommon.h"
#include "fellow/os/windows/dxgi/GfxDrvDXGI.h"
#include "fellow/os/windows/directdraw/GfxDrvDirectDraw.h"
#include "fellow/application/GraphicsDriver.h"
#include "fellow/api/Services.h"
#include "fellow/hud/HudPropertyProvider.h"

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif

using namespace std;
using namespace fellow::api;

bool gfx_drv_use_dxgi = false;

GfxDrvCommon *gfxDrvCommon = nullptr;
GfxDrvDirectDraw *gfxDrvDirectDraw = nullptr;
GfxDrvDXGI *gfxDrvDXGI = nullptr;

void gfxDrvClearCurrentBuffer()
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->ClearCurrentBuffer();
  }
  else
  {
    gfxDrvDirectDraw->ClearCurrentBuffer();
  }
}

GfxDrvMappedBufferPointer gfxDrvMapChipsetFramebuffer()
{
  if (gfx_drv_use_dxgi)
  {
    return gfxDrvDXGI->MapChipsetFramebuffer();
  }

  return gfxDrvDirectDraw->MapChipsetFramebuffer();
}

void gfxDrvUnmapChipsetFramebuffer()
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->UnmapChipsetFramebuffer();
  }
  else
  {
    gfxDrvDirectDraw->UnmapChipsetFramebuffer();
  }
}

void gfxDrvBufferFlip()
{
  gfxDrvCommon->Flip();

  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->Flip();
  }
  else
  {
    gfxDrvDirectDraw->Flip();
  }

  gfxDrvCommon->RunEventWait();
}

void gfxDrvDrawHUD(const MappedChipsetFramebuffer &mappedFramebuffer)
{
  if (gfx_drv_use_dxgi)
  {
  }
  else
  {
    gfxDrvDirectDraw->RenderHud(mappedFramebuffer);
  }
}

void gfxDrvNotifyActiveStatus(bool active)
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->NotifyActiveStatus(active);
  }
}

void gfxDrvSizeChanged(unsigned int width, unsigned int height)
{
  gfxDrvCommon->SizeChanged(width, height);

  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->SizeChanged(width, height);
  }
  else
  {
    gfxDrvDirectDraw->SizeChanged(width, height);
  }
}

void gfxDrvPositionChanged()
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->PositionChanged();
  }
  else
  {
    gfxDrvDirectDraw->PositionChanged();
  }
}

GfxDrvColorBitsInformation gfxDrvGetColorBitsInformation()
{
  if (gfx_drv_use_dxgi)
  {
    return gfxDrvDXGI->GetColorBitsInformation();
  }

  return gfxDrvDirectDraw->GetColorBitsInformation();
}

list<DisplayMode> gfxDrvGetDisplayModeList()
{
  if (gfx_drv_use_dxgi)
  {
    return gfxDrvDXGI->GetDisplayModeList();
  }

  return gfxDrvDirectDraw->GetDisplayModeList();
}

bool gfxDrvEmulationStart(
    const HostRenderConfiguration &hostRenderConfiguration,
    const HostRenderRuntimeSettings &hostRenderRuntimeSettings,
    const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings,
    HudPropertyProvider *hudPropertyProvider)
{
  if (!gfxDrvCommon->EmulationStart(hostRenderConfiguration, hostRenderRuntimeSettings.DisplayMode))
  {
    return false;
  }

  if (gfx_drv_use_dxgi)
  {
    return gfxDrvDXGI->EmulationStart(hostRenderConfiguration, chipsetBufferRuntimeSettings, hostRenderRuntimeSettings.DisplayMode, hudPropertyProvider);
  }

  return gfxDrvDirectDraw->EmulationStart(hostRenderConfiguration, hostRenderRuntimeSettings, chipsetBufferRuntimeSettings, hudPropertyProvider);
}

ULO gfxDrvEmulationStartPost(const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings)
{
  gfxDrvCommon->EmulationStartPost();

  if (gfx_drv_use_dxgi)
  {
    return gfxDrvDXGI->EmulationStartPost();
  }

  return gfxDrvDirectDraw->EmulationStartPost(chipsetBufferRuntimeSettings, gfxDrvCommon->GetHWND());
}

void gfxDrvEmulationStop()
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->EmulationStop();
  }
  else
  {
    gfxDrvDirectDraw->EmulationStop();
  }

  gfxDrvCommon->EmulationStop();
}

bool gfxDrvSaveScreenshot(const bool bSaveFilteredScreenshot, const STR *szFilename)
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

  if (gfx_drv_use_dxgi)
  {
    result = gfxDrvDXGI->SaveScreenshot(bSaveFilteredScreenshot, szActualFilename);
  }
  else
  {
    result = gfxDrvDirectDraw->SaveScreenshot(bSaveFilteredScreenshot, szActualFilename);
  }

  Service->Log.AddLog("gfxDrvSaveScreenshot(filtered=%s, filename=%s) %s.\n", bSaveFilteredScreenshot ? "true" : "false", szActualFilename, result ? "successful" : "failed");

  return result;
}

// Called when the application starts up
bool gfxDrvStartup(DISPLAYDRIVER displaydriver)
{
  gfx_drv_use_dxgi = (displaydriver == DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11);

  gfxDrvCommon = new GfxDrvCommon();

  if (!gfxDrvCommon->Startup())
  {
    return false;
  }

#ifdef RETRO_PLATFORM
  if (RP.GetHeadlessMode())
  {
    gfxDrvCommon->rp_startup_config = cfgManagerGetCurrentConfig(&cfg_manager);
    RP.RegisterRetroPlatformScreenMode(true);
  }
#endif

  if (gfx_drv_use_dxgi)
  {
    if (!gfxDrvValidateRequirements())
    {
      Service->Log.AddLog("gfxDrv ERROR: Direct3D requirements not met, falling back to DirectDraw.\n");
      gfx_drv_use_dxgi = false;
      gfxDrvDirectDraw = new GfxDrvDirectDraw();
      return gfxDrvDirectDraw->Startup();
    }

    gfxDrvDXGI = new GfxDrvDXGI();
    bool dxgiStartupResult = gfxDrvDXGI->Startup();
    if (dxgiStartupResult)
    {
      return true;
    }
    Service->Log.AddLog("gfxDrv ERROR: Direct3D present but no adapters found, falling back to DirectDraw.\n");
    gfx_drv_use_dxgi = false;
  }

  gfxDrvDirectDraw = new GfxDrvDirectDraw();
  return gfxDrvDirectDraw->Startup();
}

// Called when the application is closed
void gfxDrvShutdown()
{
  if (gfx_drv_use_dxgi)
  {
    if (gfxDrvDXGI != nullptr)
    {
      delete gfxDrvDXGI;
      gfxDrvDXGI = nullptr;
    }
  }
  else
  {
    gfxDrvDirectDraw->Shutdown();
    if (gfxDrvDirectDraw != nullptr)
    {
      delete gfxDrvDirectDraw;
      gfxDrvDirectDraw = nullptr;
    }
  }

  if (gfxDrvCommon != nullptr)
  {
    delete gfxDrvCommon;
    gfxDrvCommon = nullptr;
  }
}

bool gfxDrvValidateRequirements()
{
  return GfxDrvDXGI::ValidateRequirements();
}
