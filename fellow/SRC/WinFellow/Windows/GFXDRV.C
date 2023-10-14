#include "GfxDrvCommon.h"
#include "GfxDrvDXGI.h"
#include "GFXDRV.H"
#include "FELLOW.H"
#include "gfxdrv_directdraw.h"
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

uint8_t* gfxDrvValidateBufferPointer()
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

  fellowAddLog("gfxDrvSaveScreenshot(filtered=%s, filename=%s) %s.\n",
    bSaveFilteredScreenshot ? "true" : "false", szActualFilename, result ? "successful" : "failed");

  return result;
}

bool gfxDrvRestart(DISPLAYDRIVER displaydriver)
{
  gfxDrvShutdown();
  drawClearModeList();
  return gfxDrvStartup(displaydriver);
}

// Called when the application starts up
bool gfxDrvStartup(DISPLAYDRIVER displaydriver)
{
  gfx_drv_use_dxgi = (displaydriver == DISPLAYDRIVER_DIRECT3D11);

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
      fellowAddLog("gfxDrv ERROR: Direct3D requirements not met, falling back to DirectDraw.\n");
      gfx_drv_use_dxgi = false;
      return gfxDrvDDrawStartup();
    }

    gfxDrvDXGI = new GfxDrvDXGI();
    bool dxgiStartupResult = gfxDrvDXGI->Startup();
    if (dxgiStartupResult)
    {
      return true;
    }
    fellowAddLog("gfxDrv ERROR: Direct3D present but no adapters found, falling back to DirectDraw.\n");
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
