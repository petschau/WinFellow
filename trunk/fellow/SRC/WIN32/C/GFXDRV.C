#include "GfxDrvCommon.h"
#include "GfxDrvDXGI.h"
#include "GFXDRV.H"
#include "gfxdrv_directdraw.h"

bool gfx_drv_use_dxgi = true;

GfxDrvCommon *gfxDrvCommon = 0;
GfxDrvDXGI *gfxDrvDXGI = 0;

void gfxDrvClearCurrentBuffer()
{
  // Nothing, yet
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

void gfxDrvBufferFlip()
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->Flip();
  }
  else
  {
    gfxDrvDDrawFlip();
  }
}

void gfxDrvSizeChanged()
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->SizeChanged();
  }
  else
  {
    gfxDrvDDrawSizeChanged();
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
  gfxDrvCommon->rp_startup_config = cfgManagerGetCurrentConfig(&cfg_manager);
#endif

  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI = new GfxDrvDXGI();
    return gfxDrvDXGI->Startup();
  }

  return gfxDrvDDrawStartup();
}

// Called when the application is closed
void gfxDrvShutdown()
{
  if (gfx_drv_use_dxgi)
  {
    gfxDrvDXGI->Shutdown();

    if (gfxDrvDXGI != 0)
    {
      delete gfxDrvDXGI;
      gfxDrvDXGI = 0;
    }
  }
  else
  {
    gfxDrvDDrawShutdown();
  }

  if (gfxDrvCommon != 0)
  {
    delete gfxDrvCommon;
    gfxDrvCommon = 0;
  }
}
