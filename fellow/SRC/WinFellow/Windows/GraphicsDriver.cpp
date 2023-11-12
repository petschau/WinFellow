#include "GfxDrvCommon.h"
#include "GfxDrvDXGI.h"
#include "GraphicsDriver.h"
#include "FellowMain.h"
#include "DirectDrawDriver.h"
#include "RetroPlatform.h"
#include "VirtualHost/Core.h"

bool gfx_drv_use_dxgi = false;

GfxDrvCommon *gfxDrvCommon = nullptr;
GfxDrvDXGI *gfxDrvDXGI = nullptr;

void gfxDrvClearCurrentBuffer()
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->ClearCurrentBuffer();
  }
  else
  {
    gfxDrvDDrawClearCurrentBuffer();
  }
}

uint8_t *gfxDrvValidateBufferPointer()
{
  gfxDrvCommon->RunEventWait();

  if (gfx_drv_use_dxgi)
  {
    return gfxDrvDXGI->ValidateBufferPointer();
  }

  return gfxDrvDDrawValidateBufferPointer();
}

void gfxDrvInvalidateBufferPointer()
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->InvalidateBufferPointer();
  }
  else
  {
    gfxDrvDDrawInvalidateBufferPointer();
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
    gfxDrvDDrawFlip();
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
    gfxDrvDDrawSizeChanged(width, height);
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
    gfxDrvDDrawPositionChanged();
  }
}

void gfxDrvSetMode(draw_mode *dm, bool windowed)
{
  gfxDrvCommon->SetDrawMode(dm, windowed);

  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->SetMode(dm, windowed);
  }
  else
  {
    gfxDrvDDrawSetMode(dm, windowed);
  }
}

void gfxDrvGetBufferInformation(draw_buffer_information *buffer_information)
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->GetBufferInformation(buffer_information);
  }
  else
  {
    gfxDrvDDrawGetBufferInformation(buffer_information);
  }
}

bool gfxDrvEmulationStart(unsigned int maxbuffercount)
{
  if (!gfxDrvCommon->EmulationStart())
  {
    return false;
  }

  if (gfx_drv_use_dxgi)
  {
    return gfxDrvDXGI->EmulationStart(maxbuffercount);
  }

  return gfxDrvDDrawEmulationStart(maxbuffercount);
}

uint32_t gfxDrvEmulationStartPost()
{
  gfxDrvCommon->EmulationStartPost();

  if (gfx_drv_use_dxgi)
  {
    return gfxDrvDXGI->EmulationStartPost();
  }

  return gfxDrvDDrawEmulationStartPost();
}

void gfxDrvEmulationStop()
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->EmulationStop();
  }
  else
  {
    gfxDrvDDrawEmulationStop();
  }

  gfxDrvCommon->EmulationStop();
}

bool gfxDrvSaveScreenshot(const bool bSaveFilteredScreenshot, const char *szFilename)
{
  char szActualFilename[MAX_PATH] = "";
  bool result = false;

  if (szFilename[0] == 0)
  {
    // filename not set, automatically generate one
    _core.Fileops->GetScreenshotFileName(szActualFilename);
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
    result = gfxDrvDDrawSaveScreenshot(bSaveFilteredScreenshot, szActualFilename);
  }

  _core.Log->AddLog("gfxDrvSaveScreenshot(filtered=%s, filename=%s) %s.\n", bSaveFilteredScreenshot ? "true" : "false", szActualFilename, result ? "successful" : "failed");

  return result;
}

bool gfxDrvRestart(DISPLAYDRIVER displaydriver)
{
  gfxDrvShutdown();
  drawClearModeList();
  return gfxDrvStartup(displaydriver);
}

// Returns the actual display driver used, in case there was a problem with dxgi/direct 3d 11 and the fallback to direct draw was used instead
DISPLAYDRIVER gfxDrvTryChangeDisplayDriver(DISPLAYDRIVER newDisplayDriver, bool showErrorMessageBoxes)
{
  if (newDisplayDriver == DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11 && gfx_drv_use_dxgi) return DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11;
  if (newDisplayDriver == DISPLAYDRIVER::DISPLAYDRIVER_DIRECTDRAW && !gfx_drv_use_dxgi) return DISPLAYDRIVER::DISPLAYDRIVER_DIRECTDRAW;

  DISPLAYDRIVER actualDisplayDriver = newDisplayDriver;

  if (newDisplayDriver == DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11)
  {
    if (!gfxDrvDXGIValidateRequirements())
    {
      _core.Log->AddLog("ERROR: Configuration specified Direct3D 11, but validation of host Direct3D 11 environment failed. Falling back to DirectDraw.\n");

      if (showErrorMessageBoxes)
      {
        fellowShowRequester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, "Direct3D 11 is required but could not be loaded, falling back to DirectDraw.");
      }

      actualDisplayDriver = DISPLAYDRIVER::DISPLAYDRIVER_DIRECTDRAW;
    }
  }

  bool result = gfxDrvRestart(actualDisplayDriver);

  if (!result && actualDisplayDriver == DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11)
  {
    _core.Log->AddLog(
        "ERROR: Failed to restart graphics driver for Direct3D 11 even though host environment validation indicated it would be available. Falling back to DirectDraw.\n");

    if (showErrorMessageBoxes)
    {
      fellowShowRequester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, "Failed to initialize Direct3D 11, falling back to DirectDraw.");
    }

    actualDisplayDriver = DISPLAYDRIVER::DISPLAYDRIVER_DIRECTDRAW;
    result = gfxDrvRestart(actualDisplayDriver);
  }

  if (!result)
  {
    _core.Log->AddLog("ERROR: Failed to restart graphics driver in DirectDraw mode.\n");

    if (showErrorMessageBoxes)
    {
      fellowShowRequester(FELLOW_REQUESTER_TYPE::FELLOW_REQUESTER_TYPE_ERROR, "Failed to restart display driver for DirectDraw.");
    }
  }

  return actualDisplayDriver;
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
    if (!gfxDrvDXGIValidateRequirements())
    {
      _core.Log->AddLog("gfxDrv ERROR: Direct3D requirements not met, falling back to DirectDraw.\n");
      gfx_drv_use_dxgi = false;
      return gfxDrvDDrawStartup();
    }

    gfxDrvDXGI = new GfxDrvDXGI();
    bool dxgiStartupResult = gfxDrvDXGI->Startup();
    if (dxgiStartupResult)
    {
      return true;
    }
    _core.Log->AddLog("gfxDrv ERROR: Direct3D present but no adapters found, falling back to DirectDraw.\n");
    gfx_drv_use_dxgi = false;
  }

  return gfxDrvDDrawStartup();
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
    gfxDrvDDrawShutdown();
  }

  if (gfxDrvCommon != nullptr)
  {
    delete gfxDrvCommon;
    gfxDrvCommon = nullptr;
  }
}

bool gfxDrvDXGIValidateRequirements()
{
  return GfxDrvDXGI::ValidateRequirements();
}
