#include "GfxDrvCommon.h"
#include "GfxDrvDXGI.h"
#include "GFXDRV.H"
#include "FELLOW.H"
#include "gfxdrv_directdraw.h"
#include "RetroPlatform.h"
#include "fileops.h"

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

UBY* gfxDrvValidateBufferPointer()
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

__int64 t_previous = 0;

extern volatile __int64 timertime;

void DelayFlip()
{
  __int64 elapsed_time = timertime - t_previous;
  __int64 timestamp = timertime;
  __int64 wait_for = t_previous + 18;

  // Busy loop...
  // Replace with wait for an event set from the timer, if it is fast enough
  while (wait_for > timertime);

  t_previous = timertime;
}

void gfxDrvBufferFlip()
{
  if (soundGetEmulation() == SOUND_PLAY && soundGetNotification() == SOUND_MMTIMER_NOTIFICATION)
  {
    DelayFlip();
  }

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

void gfxDrvSetMode(draw_mode *dm)
{
  gfxDrvCommon->SetDrawMode(dm);

  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->SetMode(dm);
  }
  else
  {
    gfxDrvDDrawSetMode(dm);
  }
}

void gfxDrvGetBufferInformation(draw_mode *dm, draw_buffer_information *buffer_information)
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->GetBufferInformation(dm, buffer_information);
  }
  else
  {
    gfxDrvDDrawGetBufferInformation(dm, buffer_information);
  }
}

bool gfxDrvEmulationStart(unsigned int maxbuffercount)
{
  t_previous = timertime;

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

ULO gfxDrvEmulationStartPost()
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

bool gfxDrvSaveScreenshot(const bool bSaveFilteredScreenshot, const STR *szFilename)
{
  STR szActualFilename[MAX_PATH] = "";
  bool result = false;

  if (szFilename[0] == 0)
  {
    // filename not set, automatically generate one
    fileopsGetScreenshotFileName(szActualFilename);
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

  fellowAddLog("gfxDrvSaveScreenshot(%s, %s) %s.\n",
    bSaveFilteredScreenshot ? "filtered" : "raw", szActualFilename, result ? "successful" : "failed");

  return result;
}

bool gfxDrvRestart(DISPLAYDRIVER displaydriver)
{
  gfxDrvShutdown();
  drawModesFree();
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
    gfxDrvCommon->rp_startup_config = cfgManagerGetCurrentConfig(&cfg_manager);
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

bool gfxDrvDXGIValidateRequirements(void)
{
  HINSTANCE hDll;

  hDll = LoadLibrary("d3d11.dll");
  if (hDll) {
    FreeLibrary(hDll);
  }
  else
  {
    fellowAddLog("gfxDrvDXGIValidateRequirements() ERROR: d3d11.dll could not be loaded.\n");
    return false;
  }

  hDll = LoadLibrary("dxgi.dll");
  if (hDll) {
    FreeLibrary(hDll);
  }
  else
  {
    fellowAddLog("gfxDrvDXGIValidateRequirements() ERROR: dxgi.dll could not be loaded.\n");
    return false;
  }

  return true;
}

void gfxDrvRegisterRetroPlatformScreenMode(const bool bStartup)
{
  ULO lHeight, lWidth, lDisplayScale;

  if (RP.GetScanlines())
    cfgSetDisplayScaleStrategy(gfxDrvCommon->rp_startup_config, DISPLAYSCALE_STRATEGY_SCANLINES);
  else
    cfgSetDisplayScaleStrategy(gfxDrvCommon->rp_startup_config, DISPLAYSCALE_STRATEGY_SOLID);

  if (bStartup) {
    RP.SetScreenHeight(cfgGetScreenHeight(gfxDrvCommon->rp_startup_config));
    RP.SetScreenWidth(cfgGetScreenWidth(gfxDrvCommon->rp_startup_config));
  }

  lHeight = RP.GetScreenHeightAdjusted();
  lWidth  = RP.GetScreenWidthAdjusted();
  lDisplayScale = RP.GetDisplayScale();

  cfgSetScreenHeight(gfxDrvCommon->rp_startup_config, lHeight);
  cfgSetScreenWidth (gfxDrvCommon->rp_startup_config, lWidth);

  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->RegisterRetroPlatformScreenMode(false, lWidth, lHeight, lDisplayScale);
  }
  else
  {
    gfxDrvDDrawRegisterRetroPlatformScreenMode(false, lWidth, lHeight, lDisplayScale);
  }
}