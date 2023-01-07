#include "fellow/api/defs.h"
#include "DirectDrawDevice.h"
#include "DirectDrawErrorLogger.h"
#include "fellow/api/Services.h"
#include "fellow/os/windows/directdraw/GfxDrvDCScreenshot.h"
#include "fellow/chipset/Graphics.h"

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#include "fellow/configuration/Configuration.h"
#endif

using namespace std;

void DirectDrawDevice::LogError(const char *header, HRESULT err)
{
  DirectDrawErrorLogger::LogError(header, err);
}

//===================================================
// Get information about the current client rectangle
//===================================================

void DirectDrawDevice::FindWindowClientRect()
{
  GetClientRect(Hwnd, &hwnd_clientrect_win);
  memcpy(&hwnd_clientrect_screen, &hwnd_clientrect_win, sizeof(RECT));
  ClientToScreen(Hwnd, (LPPOINT)&hwnd_clientrect_screen);
  ClientToScreen(Hwnd, (LPPOINT)&hwnd_clientrect_screen + 1);
}

//========================================================
// Select the correct drawing surface for a draw operation
//========================================================

void DirectDrawDevice::SelectTargetSurface(LPDIRECTDRAWSURFACE *lpDDS, LPDDSURFACEDESC *lpDDSD)
{
  *lpDDS = lpDDSSecondary;
  *lpDDSD = &ddsdSecondary;
}

//=======================================================
// Select the correct target surface for a blit operation
//=======================================================

void DirectDrawDevice::SelectBlitTargetSurface(LPDIRECTDRAWSURFACE *lpDDS)
{
  *lpDDS = lpDDSPrimary;
}

//================
// Release clipper
//================

void DirectDrawDevice::ClipperRelease()
{
  if (lpDDClipper != nullptr)
  {
    auto clipperReleaseResult = IDirectDrawClipper_Release(lpDDClipper);
    if (clipperReleaseResult != DD_OK)
    {
      LogError("GfxDrvDirectDraw::ClipperRelease(): ", clipperReleaseResult);
    }

    lpDDClipper = nullptr;
  }
}

//===================
// Initialize clipper
//===================

bool DirectDrawDevice::ClipperInitialize()
{
  auto createClipperResult = IDirectDraw2_CreateClipper(lpDD2, 0, &lpDDClipper, nullptr);
  if (createClipperResult != DD_OK)
  {
    LogError("GfxDrvDirectDraw::ClipperInitialize(): CreateClipper() ", createClipperResult);
    return false;
  }

  auto clipperSetHWndResult = IDirectDrawClipper_SetHWnd(lpDDClipper, 0, Hwnd);
  if (clipperSetHWndResult != DD_OK)
  {
    LogError("GfxDrvDirectDraw::ClipperInitialize(): SetHWnd() ", clipperSetHWndResult);
    ClipperRelease();
    return false;
  }

  auto primarySurfaceSetClipperResult = IDirectDrawSurface_SetClipper(lpDDSPrimary, lpDDClipper);
  if (primarySurfaceSetClipperResult != DD_OK)
  {
    LogError("GfxDrvDirectDraw::ClipperInitialize(): SetClipper() ", primarySurfaceSetClipperResult);
    ClipperRelease();
    return false;
  }

  return true;
}

//===============================================================
// Creates the DirectDraw1 object for the given DirectDraw device
// Called on emulator startup
//===============================================================

bool DirectDrawDevice::DirectDraw1ObjectInitialize()
{
  lpDD = nullptr;

  HRESULT err = DirectDrawCreate(&GUID, &lpDD, nullptr);
  if (err != DD_OK)
  {
    LogError("GfxDrvDirectDraw::DirectDraw1ObjectInitialize(): ", err);
    return false;
  }

  return true;
}

//================================================================
// Releases the DirectDraw1 object for the given DirectDraw device
// Called on emulator startup
//================================================================

bool DirectDrawDevice::DirectDraw1ObjectRelease()
{
  HRESULT err = DD_OK;

  if (lpDD != nullptr)
  {
    err = IDirectDraw_Release(lpDD);
    if (err != DD_OK)
    {
      LogError("GfxDrvDirectDraw::DirectDraw1ObjectRelease(): ", err);
    }
    lpDD = nullptr;
  }

  return (err == DD_OK);
}

//===============================================================
// Creates the DirectDraw2 object for the given DirectDraw device
// Requires that a DirectDraw1 object has been initialized
// Called on emulator startup
//===============================================================

bool DirectDrawDevice::DirectDraw2ObjectInitialize()
{
  lpDD2 = nullptr;

  HRESULT err = IDirectDraw_QueryInterface(lpDD, IID_IDirectDraw2, (LPVOID *)&lpDD2);
  if (err != DD_OK)
  {
    LogError("GfxDrvDirectDraw::DirectDraw2ObjectInitialize(): ", err);
    return false;
  }

  DDCAPS caps{};
  caps.dwSize = sizeof(caps);
  HRESULT res = IDirectDraw_GetCaps(lpDD2, &caps, nullptr);
  if (res != DD_OK)
  {
    LogError("GetCaps()", res);
    return false;
  }

  can_stretch_y = (caps.dwFXCaps & DDFXCAPS_BLTARITHSTRETCHY) || (caps.dwFXCaps & DDFXCAPS_BLTARITHSTRETCHYN) || (caps.dwFXCaps & DDFXCAPS_BLTSTRETCHY) || (caps.dwFXCaps & DDFXCAPS_BLTSHRINKYN);
  if (!can_stretch_y)
  {
    fellow::api::Service->Log.AddLog("gfxdrv: WARNING: No hardware stretch\n");
  }

  no_dd_hardware = !!(caps.dwCaps & DDCAPS_NOHARDWARE);
  if (no_dd_hardware)
  {
    fellow::api::Service->Log.AddLog("gfxdrv: WARNING: No DirectDraw hardware\n");
  }

  return true;
}

//================================================================
// Releases the DirectDraw2 object for the given DirectDraw device
// Called on emulator shutdown
//================================================================

bool DirectDrawDevice::DirectDraw2ObjectRelease()
{
  HRESULT err = DD_OK;

  if (lpDD2 != nullptr)
  {
    err = IDirectDraw2_Release(lpDD2);
    if (err != DD_OK)
    {
      LogError("GfxDrvDirectDraw::DirectDraw2ObjectRelease(): ", err);
    }
    lpDD2 = nullptr;
  }

  return (err == DD_OK);
}

//=================================================================
// Creates the DirectDraw object for the selected DirectDraw device
// Called on emulator startup
// Returns success or not
//=================================================================

bool DirectDrawDevice::DirectDrawObjectInitialize()
{
  bool result = DirectDraw1ObjectInitialize();

  if (result)
  {
    result = DirectDraw2ObjectInitialize();
    DirectDraw1ObjectRelease();
  }

  return result;
}

//====================================
// Returns fullscreen mode information
//====================================

DirectDrawFullscreenMode *DirectDrawDevice::FindFullscreenModeById(unsigned int id)
{
  for (auto mode : fullscreen_modes)
  {
    if (mode->id == id)
    {
      return mode;
    }
  }

  return nullptr;
}

string DirectDrawDevice::GetDisplayModeName(const DirectDrawFullscreenMode *displayMode)
{
  ostringstream oss;
  oss << displayMode->width << "Wx" << displayMode->height << "Hx" << displayMode->depth << "BPP";

  if (displayMode->refresh != 0)
  {
    oss << "x" << displayMode->refresh << "Hz";
  }

  return oss.str();
}

//=================================================
// Describes all the found modes to the draw module
//=================================================

list<DisplayMode> DirectDrawDevice::GetDisplayModeList()
{
  list<DisplayMode> displayModes;

  for (auto mode : fullscreen_modes)
  {
    displayModes.emplace_back(DisplayMode(mode->id, mode->width, mode->height, mode->depth, mode->refresh, GetDisplayModeName(mode)));
  }

  return displayModes;
}

//=======================================
// Dump information about available modes
//=======================================

void DirectDrawDevice::LogFullscreenModeInformation()
{
  list<string> logmessages;

  ostringstream summary;
  summary << "gfxdrv: DirectDraw fullscreen modes found: " << fullscreen_modes.size();
  logmessages.emplace_back(summary.str());

  for (auto mode : fullscreen_modes)
  {
    ostringstream modeDescription;
    modeDescription << "gfxdrv: Mode Description: id " << mode->id << " " << mode->width << "Wx" << mode->height << "Hx" << mode->depth << "BPPx" << mode->refresh << "Hz (" << mode->redpos << ", "
                    << mode->redsize << ", " << mode->greenpos << ", " << mode->greensize << ", " << mode->bluepos << ", " << mode->bluesize << ")";

    logmessages.emplace_back(modeDescription.str());
  }

  fellow::api::Service->Log.AddLogList(logmessages);
}

//===============================================================
// Creates a list of available modes on a given DirectDraw device
//===============================================================

bool DirectDrawDevice::InitializeFullscreenModeInformation()
{
  ReleaseFullscreenModeInformation();

  HRESULT err = IDirectDraw2_EnumDisplayModes(lpDD2, DDEDM_REFRESHRATES, nullptr, (LPVOID)this, DirectDrawDevice::EnumerateFullscreenMode);
  if (err != DD_OK)
  {
    LogError("GfxDrvDirectDraw::InitializeFullscreenModeInformation(): ", err);
  }

  if (fullscreen_modes.empty())
  {
    fellow::api::Service->Log.AddLog("GfxDrvDirectDraw::InitializeFullscreenModeInformation(): No valid draw modes found, retry while ignoring refresh rates...\n");

    err = IDirectDraw2_EnumDisplayModes(lpDD2, 0, nullptr, (LPVOID)this, DirectDrawDevice::EnumerateFullscreenMode);
    if (err != DD_OK)
    {
      LogError("GfxDrvDirectDraw::InitializeFullscreenModeInformation(): Fallback enumeration without refresh rate failed ", err);
    }
  }

  LogFullscreenModeInformation();

  return !fullscreen_modes.empty();
}

//=====================================
// Releases the list of available modes
//=====================================

void DirectDrawDevice::ReleaseFullscreenModeInformation()
{
  for (auto fullscreenMode : fullscreen_modes)
  {
    delete fullscreenMode;
  }
  fullscreen_modes.clear();
}

//========================================================
// Callback used when creating the list of available modes
//========================================================

HRESULT WINAPI DirectDrawDevice::EnumerateFullscreenMode(LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext)
{
  DirectDrawDevice *ddrawDevice = (DirectDrawDevice *)lpContext;
  ddrawDevice->AddEnumeratedFullscreenMode(lpDDSurfaceDesc);
  return DDENUMRET_OK;
}

void DirectDrawDevice::AddEnumeratedFullscreenMode(LPDDSURFACEDESC lpDDSurfaceDesc)
{
  if (IsRGBWithSupportedBitcount(lpDDSurfaceDesc->ddpfPixelFormat) && HasSupportedRefreshRate(lpDDSurfaceDesc->dwRefreshRate) && HasSupportedWidth(lpDDSurfaceDesc->dwWidth))
  {

    DirectDrawFullscreenMode *tmpmode = NewFullscreenMode(
        (unsigned int)fullscreen_modes.size(),
        lpDDSurfaceDesc->dwWidth,
        lpDDSurfaceDesc->dwHeight,
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount,
        lpDDSurfaceDesc->dwRefreshRate,
        RGBMaskPos(lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask),
        RGBMaskSize(lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask),
        RGBMaskPos(lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask),
        RGBMaskSize(lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask),
        RGBMaskPos(lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask),
        RGBMaskSize(lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask));

    fullscreen_modes.push_back(tmpmode);
  }
}

constexpr bool DirectDrawDevice::IsRGBWithSupportedBitcount(DDPIXELFORMAT pixelFormat)
{
  return (pixelFormat.dwFlags & DDPF_RGB) && ((pixelFormat.dwRGBBitCount == 16) || (pixelFormat.dwRGBBitCount == 24) || (pixelFormat.dwRGBBitCount == 32));
}

constexpr bool DirectDrawDevice::HasSupportedRefreshRate(DWORD refreshRate)
{
  return refreshRate == 0 || refreshRate >= 50;
}

constexpr bool DirectDrawDevice::HasSupportedWidth(DWORD width)
{
  return width >= 640;
}

ULO DirectDrawDevice::RGBMaskPos(ULO mask)
{
  for (ULO i = 0; i < 32; i++)
  {
    if (mask & (1 << i))
    {
      return i;
    }
  }
  return 0;
}

ULO DirectDrawDevice::RGBMaskSize(ULO mask)
{
  ULO sz = 0;

  for (ULO i = 0; i < 32; i++)
  {
    if (mask & (1 << i))
    {
      sz++;
    }
  }
  return sz;
}

DirectDrawFullscreenMode *DirectDrawDevice::NewFullscreenMode(
    unsigned int id, ULO width, ULO height, ULO depth, ULO refresh, ULO redpos, ULO redsize, ULO greenpos, ULO greensize, ULO bluepos, ULO bluesize)
{
  return new DirectDrawFullscreenMode{
      .id = id,
      .width = width,
      .height = height,
      .depth = depth,
      .refresh = refresh,
      .redpos = redpos,
      .redsize = redsize,
      .greenpos = greenpos,
      .greensize = greensize,
      .bluepos = bluepos,
      .bluesize = bluesize};
}

//======================
// Set cooperative level
//======================

bool DirectDrawDevice::SetCooperativeLevelNormal()
{
  auto setCooperativeLevelResult = IDirectDraw2_SetCooperativeLevel(lpDD2, Hwnd, DDSCL_NORMAL);
  if (setCooperativeLevelResult != DD_OK)
  {
    LogError("GfxDrvDirectDraw::SetCooperativeLevelNormal(): ", setCooperativeLevelResult);
    return false;
  }

  return true;
}

bool DirectDrawDevice::SetCooperativeLevelExclusive()
{
  auto setCooperativeLevelResult = IDirectDraw2_SetCooperativeLevel(lpDD2, Hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
  if (setCooperativeLevelResult != DD_OK)
  {
    LogError("GfxDrvDirectDraw::SetCooperativeLevelExclusive(): ", setCooperativeLevelResult);
    return false;
  }

  return true;
}

bool DirectDrawDevice::SetCooperativeLevel()
{
  if (drawmode.IsWindowed)
  {
    return SetCooperativeLevelNormal();
  }

  return SetCooperativeLevelExclusive();
}

//===========================
// Clear surface with blitter
//===========================

void DirectDrawDevice::SurfaceClear(LPDIRECTDRAWSURFACE surface, RECT *dstrect)
{
  DDBLTFX ddbltfx{};
  ddbltfx.dwSize = sizeof(ddbltfx);
  ddbltfx.dwFillColor = 0;

  auto clearSurfaceBlitResult = IDirectDrawSurface_Blt(surface, dstrect, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
  if (clearSurfaceBlitResult != DD_OK)
  {
    LogError("GfxDrvDirectDraw::SurfaceClear(): ", clearSurfaceBlitResult);
  }

  fellow::api::Service->Log.AddLog("gfxdrv: Clearing surface\n");
}

//==================
// Restore a surface
//==================

HRESULT DirectDrawDevice::SurfaceRestore(LPDIRECTDRAWSURFACE surface)
{
  if (IDirectDrawSurface_IsLost(surface) != DDERR_SURFACELOST)
  {
    fellow::api::Service->Log.AddLog("GfxDrvDirectDraw::SurfaceRestore(): Called but surface was not lost.\n");
    return DD_OK;
  }

  auto surfaceRestoreResult = IDirectDrawSurface_Restore(surface);
  if (surfaceRestoreResult == DD_OK)
  {
    SurfaceClear(surface);
    graphLineDescClear();
  }

  return surfaceRestoreResult;
}

void DirectDrawDevice::CalculateDestinationRectangle(ULO output_width, ULO output_height, RECT &dstwin)
{
  const float srcClipWidth = (float)_chipsetBufferOutputClip.GetWidth();
  const float srcClipHeight = (float)_chipsetBufferOutputClip.GetHeight();
  int upscaled_clip_width;
  int upscaled_clip_height;

  if (displayScale != DisplayScale::Automatic)
  {
    // Fixed scaling
    float upscale_factor = static_cast<float>(_outputScaleFactor) / static_cast<float>(_chipsetBufferScaleFactor);
    upscaled_clip_width = static_cast<int>(srcClipWidth * upscale_factor);
    upscaled_clip_height = static_cast<int>(srcClipHeight * upscale_factor);
  }
  else
  {
    // Automatic best fit in the destination
    float dstWidth = static_cast<float>(output_width);
    float dstHeight = static_cast<float>(output_height);

    float srcAspectRatio = srcClipWidth / srcClipHeight;
    float dstAspectRatio = dstWidth / dstHeight;

    if (dstAspectRatio > srcAspectRatio)
    {
      // Stretch to full height, black vertical borders
      dstWidth = srcClipWidth * dstHeight / srcClipHeight;
    }
    else
    {
      // Stretch to full width, black horisontal borders
      dstHeight = srcClipHeight * dstWidth / srcClipWidth;
    }

    upscaled_clip_width = static_cast<int>(dstWidth);
    upscaled_clip_height = static_cast<int>(dstHeight);
  }

  dstwin.left = (output_width - upscaled_clip_width) / 2;
  dstwin.top = (output_height - upscaled_clip_height) / 2;
  dstwin.right = dstwin.left + upscaled_clip_width;
  dstwin.bottom = dstwin.top + upscaled_clip_height;

  if (drawmode.IsWindowed)
  {
    // Add client rect offset in window mode
    dstwin.left += hwnd_clientrect_screen.left;
    dstwin.top += hwnd_clientrect_screen.top;
    dstwin.right += hwnd_clientrect_screen.left;
    dstwin.bottom += hwnd_clientrect_screen.top;
  }
}

//=============================================
// Blit secondary buffer to the primary surface
//=============================================

void DirectDrawDevice::SurfaceBlit()
{
  // Srcwin is used when we do additional scaling into the primary buffer (3x, 4x, or stretch height for shres chipset buffer)
  // Windowed and fullscreen are the same, using blitter transfer from chipset buffer
  RECT srcwin{.left = (LONG)_chipsetBufferOutputClip.Left, .top = (LONG)_chipsetBufferOutputClip.Top, .right = (LONG)_chipsetBufferOutputClip.Right, .bottom = (LONG)_chipsetBufferOutputClip.Bottom};

  // Destination is always the primary buffer
  LPDIRECTDRAWSURFACE lpDDSDestination{};
  SelectBlitTargetSurface(&lpDDSDestination);

  RECT dstwin{};
  CalculateDestinationRectangle(_outputWidth, _outputHeight, dstwin);

  // This can fail when a surface is lost
  DDBLTFX bltfx{.dwSize = sizeof(DDBLTFX)};

  auto surfaceBlitResult = IDirectDrawSurface_Blt(lpDDSDestination, &dstwin, lpDDSSecondary, &srcwin, DDBLT_ASYNC, &bltfx);
  if (surfaceBlitResult != DD_OK)
  {
    LogError("GfxDrvDirectDraw::SurfaceBlit(): (Blt failed) ", surfaceBlitResult);

    if (surfaceBlitResult != DDERR_SURFACELOST)
    {
      return;
    }

    // Reclaim surface, if that also fails, pass over the blit
    auto primarySurfaceRestoreResult = SurfaceRestore(lpDDSPrimary);
    if (primarySurfaceRestoreResult != DD_OK)
    {
      // Can not provide a buffer pointer
      LogError("GfxDrvDirectDraw::SurfaceBlit(): (Restore primary surface failed) ", primarySurfaceRestoreResult);
      return;
    }

    auto secondarySurfaceRestoreResult = SurfaceRestore(lpDDSSecondary);
    if (secondarySurfaceRestoreResult != DD_OK)
    {
      // Can not provide a buffer pointer
      LogError("GfxDrvDirectDraw::SurfaceBlit(): (Restore secondary surface failed) ", secondarySurfaceRestoreResult);
      return;
    }

    // Restore was successful, do the blit
    auto restoredSurfaceBlitResult = IDirectDrawSurface_Blt(lpDDSDestination, &dstwin, lpDDSSecondary, &srcwin, DDBLT_ASYNC, &bltfx);
    if (restoredSurfaceBlitResult != DD_OK)
    {
      // Failed second time, pass over the blit
      LogError("GfxDrvDirectDraw::SurfaceBlit(): (Blit failed after restore) ", restoredSurfaceBlitResult);
    }
  }
}

//=====================
// Release the surfaces
//=====================

void DirectDrawDevice::SurfacesRelease()
{
  if (lpDDSPrimary != nullptr)
  {
    auto primarySurfaceReleaseResult = IDirectDrawSurface_Release(lpDDSPrimary);
    if (primarySurfaceReleaseResult != DD_OK)
    {
      LogError("GfxDrvDirectDraw::SurfacesRelease(): ", primarySurfaceReleaseResult);
    }
    lpDDSPrimary = nullptr;

    if (drawmode.IsWindowed)
    {
      ClipperRelease();
    }
  }

  if (lpDDSSecondary != nullptr)
  {
    auto secondarySurfaceReleaseResult = IDirectDrawSurface_Release(lpDDSSecondary);
    if (secondarySurfaceReleaseResult != DD_OK)
    {
      LogError("GfxDrvDirectDraw::SurfacesRelease(): ", secondarySurfaceReleaseResult);
    }
    lpDDSSecondary = nullptr;
  }
}

//=====================================================================
// Create a second offscreen buffer
// A second buffer is used when the blitter is used for displaying data
//=====================================================================

const char *DirectDrawDevice::GetVideomemoryLocationDescription(ULO pass)
{
  switch (pass)
  {
    case 0: return "local videomemory (on card)";
    case 1: return "non-local videomemory (AGP shared mem)";
    case 2: return "system memory";
  }
  return "unknown memory";
}

ULO DirectDrawDevice::GetVideomemoryLocationFlags(ULO pass)
{
  switch (pass)
  {
    case 0: return DDSCAPS_VIDEOMEMORY;                          /* Local videomemory (default oncard) */
    case 1: return DDSCAPS_NONLOCALVIDMEM | DDSCAPS_VIDEOMEMORY; /* Shared videomemory */
    case 2: return DDSCAPS_SYSTEMMEMORY;                         /* Plain memory */
  }
  return 0;
}

bool DirectDrawDevice::CreateSecondaryOffscreenSurface(const DimensionsUInt &chipsetBufferSize)
{
  bool result = true;

  ULO pass = 0;
  bool buffer_allocated = false;
  while ((pass < 3) && !buffer_allocated)
  {
    ddsdSecondary.dwSize = sizeof(ddsdSecondary);
    ddsdSecondary.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsdSecondary.ddsCaps.dwCaps = GetVideomemoryLocationFlags(pass) | DDSCAPS_OFFSCREENPLAIN;

    ddsdSecondary.dwHeight = chipsetBufferSize.Height;
    ddsdSecondary.dwWidth = chipsetBufferSize.Width;

    HRESULT err = IDirectDraw2_CreateSurface(lpDD2, &ddsdSecondary, &lpDDSSecondary, nullptr);
    if (err != DD_OK)
    {
      LogError("GfxDrvDirectDraw::CreateSecondaryOffscreenSurface() 0x%x\n", err);
      fellow::api::Service->Log.AddLog("gfxdrv: Failed to allocate second offscreen surface in %s\n", GetVideomemoryLocationDescription(pass));
      result = false;
    }
    else
    {
      buffer_allocated = true;
      fellow::api::Service->Log.AddLog("gfxdrv: Allocated second offscreen surface in %s (%u, %u)\n", GetVideomemoryLocationDescription(pass), chipsetBufferSize.Width, chipsetBufferSize.Height);
      SurfaceClear(lpDDSSecondary);
      result = true;
    }

    pass++;
  }

  return result && buffer_allocated;
}

//====================
// Create the surfaces
//====================

string DirectDrawDevice::GetPixelFlagDescription(DWORD flags)
{
  ostringstream oss;
  if (flags & DDPF_ALPHAPIXELS) oss << "(DDPF_ALPHAPIXELS)";
  if (flags & DDPF_ALPHA) oss << "(DDPF_ALPHA)";
  if (flags & DDPF_FOURCC) oss << "(DDPF_FOURCC)";
  if (flags & DDPF_PALETTEINDEXED4) oss << "(DDPF_PALETTEINDEXED4)";
  if (flags & DDPF_PALETTEINDEXEDTO8) oss << "(DDPF_PALETTEINDEXEDTO8)";
  if (flags & DDPF_PALETTEINDEXED8) oss << "(DDPF_PALETTEINDEXED8)";
  if (flags & DDPF_RGB) oss << "(DDPF_RGB)";
  if (flags & DDPF_COMPRESSED) oss << "(DDPF_COMPRESSED)";
  if (flags & DDPF_RGBTOYUV) oss << "(DDPF_RGBTOYUV)";
  if (flags & DDPF_YUV) oss << "(DDPF_YUV)";
  if (flags & DDPF_ZBUFFER) oss << "(DDPF_ZBUFFER)";
  if (flags & DDPF_PALETTEINDEXED1) oss << "(DDPF_PALETTEINDEXED1)";
  if (flags & DDPF_PALETTEINDEXED2) oss << "(DDPF_PALETTEINDEXED2)";
  if (flags & DDPF_ZPIXELS) oss << "(DDPF_ZPIXELS)";
  if (flags & DDPF_STENCILBUFFER) oss << "(DDPF_STENCILBUFFER)";
  if (flags & DDPF_ALPHAPREMULT) oss << "(DDPF_ALPHAPREMULT)";
  if (flags & DDPF_LUMINANCE) oss << "(DDPF_LUMINANCE)";
  if (flags & DDPF_BUMPLUMINANCE) oss << "(DDPF_BUMPLUMINANCE)";
  if (flags & DDPF_BUMPDUDV) oss << "(DDPF_BUMPDUDV)";
  return oss.str();
}

void DirectDrawDevice::LogPixelFormat(DDPIXELFORMAT ddpf)
{
  fellow::api::Service->Log.AddLog(
      "gfxdrv: Surface has pixelformat flags %s (%.8X), (%d, %d, %d, %d, %d, %d, %d)\n",
      GetPixelFlagDescription(ddpf.dwFlags).c_str(),
      ddpf.dwFlags,
      ddpf.dwRGBBitCount,
      RGBMaskPos(ddpf.dwRBitMask),
      RGBMaskSize(ddpf.dwRBitMask),
      RGBMaskPos(ddpf.dwGBitMask),
      RGBMaskSize(ddpf.dwGBitMask),
      RGBMaskPos(ddpf.dwBBitMask),
      RGBMaskSize(ddpf.dwBBitMask));
}

bool DirectDrawDevice::SurfacesInitialize(const DimensionsUInt &chipsetBufferSize)
{
  if (lpDDSPrimary != nullptr)
  {
    SurfacesRelease();
  }

  bool secondarySurfaceCreated = CreateSecondaryOffscreenSurface(chipsetBufferSize);
  if (!secondarySurfaceCreated)
  {
    return false;
  }

  ddsdPrimary.dwSize = sizeof(ddsdPrimary);
  ddsdPrimary.dwFlags = DDSD_CAPS;
  ddsdPrimary.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

  auto primarySurfaceCreateResult = IDirectDraw2_CreateSurface(lpDD2, &ddsdPrimary, &lpDDSPrimary, nullptr);
  if (primarySurfaceCreateResult != DD_OK)
  {
    LogError("GfxDrvDirectDraw::SurfacesInitialize(): ", primarySurfaceCreateResult);
    fellow::api::Service->Log.AddLog("gfxdrv: Failed to allocate primary surface with backbuffer\n");
    SurfacesRelease();
    return false;
  }

  fellow::api::Service->Log.AddLog("gfxdrv: Allocated primary surface with backbuffer\n");
  _clearBorders = true;

  DDPIXELFORMAT ddpf{};
  ddpf.dwSize = sizeof(ddpf);

  auto primarySurfaceGetPixelFormatResult = IDirectDrawSurface_GetPixelFormat(lpDDSPrimary, &ddpf);
  if (primarySurfaceGetPixelFormatResult != DD_OK)
  {
    LogError("GfxDrvDirectDraw::SurfacesInitialize(): GetPixelFormat() ", primarySurfaceGetPixelFormatResult);
    SurfacesRelease();
    return false;
  }

  if (!IsRGBWithSupportedBitcount(ddpf))
  {
    fellow::api::Service->Log.AddLog("GfxDrvDirectDraw::SurfacesInitialize(): Window mode - unsupported Pixelformat, flags (%.8X)\n", ddpf.dwFlags);
    SurfacesRelease();
    return false;
  }

  LogPixelFormat(ddpf);
  SetColorBitsInformation(ddpf);

  // Set clipper if windowed mode
  if (drawmode.IsWindowed)
  {
    if (!ClipperInitialize())
    {
      SurfacesRelease();
      return false;
    }
  }

  return true;
}

void DirectDrawDevice::ClearWindowBorders()
{
  RECT dstwin{};
  CalculateDestinationRectangle(_outputWidth, _outputHeight, dstwin);

  RECT screenrect{};

  if (drawmode.IsWindowed)
  {
    screenrect = hwnd_clientrect_screen;
  }
  else
  {
    screenrect.left = 0;
    screenrect.top = 0;
    screenrect.right = _outputWidth;
    screenrect.bottom = _outputHeight;
  }

  RECT clearrect{};
  if (screenrect.top < dstwin.top)
  {
    clearrect.left = screenrect.left;
    clearrect.top = screenrect.top;
    clearrect.right = screenrect.right;
    clearrect.bottom = dstwin.top;
    SurfaceClear(lpDDSPrimary, &clearrect);
  }

  if (screenrect.bottom > dstwin.bottom)
  {
    clearrect.left = screenrect.left;
    clearrect.top = dstwin.bottom;
    clearrect.right = screenrect.right;
    clearrect.bottom = screenrect.bottom;
    SurfaceClear(lpDDSPrimary, &clearrect);
  }

  if (screenrect.left < dstwin.left)
  {
    clearrect.left = screenrect.left;
    clearrect.top = screenrect.top;
    clearrect.right = dstwin.left;
    clearrect.bottom = screenrect.bottom;
    SurfaceClear(lpDDSPrimary, &clearrect);
  }

  if (screenrect.right > dstwin.right)
  {
    clearrect.left = dstwin.right;
    clearrect.top = screenrect.top;
    clearrect.right = screenrect.right;
    clearrect.bottom = screenrect.bottom;
    SurfaceClear(lpDDSPrimary, &clearrect);
  }
}

//=================================
// Lock the current drawing surface
//=================================

uint8_t *DirectDrawDevice::SurfaceLock(ptrdiff_t *pitch)
{
  if (_clearBorders)
  {
    _clearBorders = false;
    ClearWindowBorders();
  }

  LPDIRECTDRAWSURFACE lpDDS{};
  LPDDSURFACEDESC lpDDSD{};
  SelectTargetSurface(&lpDDS, &lpDDSD);

  auto primarySurfaceLockResult = IDirectDrawSurface_Lock(lpDDS, nullptr, lpDDSD, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, nullptr);
  if (primarySurfaceLockResult != DD_OK)
  {
    if (primarySurfaceLockResult != DDERR_SURFACELOST)
    {
      // Restore failed for unhandled reason, can not provide a buffer pointer
      LogError("GfxDrvDirectDraw::SurfaceLock(): Unhandled reason for failure to lock surface. ", primarySurfaceLockResult);
      return nullptr;
    }

    // Here we have lost the surface, restore it
    auto primarySurfaceRestoreResult = SurfaceRestore(lpDDS);
    if (primarySurfaceRestoreResult != DD_OK)
    {
      // Restore failed, can not provide a buffer pointer
      LogError("GfxDrvDirectDraw::SurfaceLock(): Failed to restore primary surface. ", primarySurfaceRestoreResult);
      return nullptr;
    }

    // Restore successful, try again to lock the surface
    SelectTargetSurface(&lpDDS, &lpDDSD);
    auto primarySurfaceLockRetryResult = IDirectDrawSurface_Lock(lpDDS, nullptr, lpDDSD, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, nullptr);
    if (primarySurfaceLockRetryResult != DD_OK)
    {
      // Restore failed again, can not provide a buffer pointer
      LogError("GfxDrvDirectDraw::SurfaceLock(): Lock failed after successful restore. ", primarySurfaceLockRetryResult);
      return nullptr;
    }
  }

  *pitch = lpDDSD->lPitch;
  return (UBY *)lpDDSD->lpSurface;
}

//===========================
// Unlock the primary surface
//===========================

void DirectDrawDevice::SurfaceUnlock()
{
  LPDIRECTDRAWSURFACE lpDDS{};
  LPDDSURFACEDESC lpDDSD{};
  SelectTargetSurface(&lpDDS, &lpDDSD);

  auto surfaceUnlockResult = IDirectDrawSurface_Unlock(lpDDS, lpDDSD->lpSurface);
  if (surfaceUnlockResult != DD_OK)
  {
    LogError("GfxDrvDirectDraw::SurfaceUnlock(): ", surfaceUnlockResult);
  }
}

//======================================================================
// Set specified display mode and create the surfaces required for it
// In windowed mode, we don't set a mode, but instead queries the pixel-
// format from the surface
//======================================================================

bool DirectDrawDevice::ActivateDisplayMode(const DimensionsUInt &chipsetBufferDimensions, HWND hwnd)
{
  Hwnd = hwnd;

  bool setCooperativeLevelSuccess = SetCooperativeLevel();
  if (!setCooperativeLevelSuccess)
  {
    return false;
  }

  if (!drawmode.IsWindowed)
  {
    DirectDrawFullscreenMode *mode = FindFullscreenModeById(drawmode.Id);
    DDSURFACEDESC myDDSDesc{};
    myDDSDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;

    auto setDisplayModeResult = IDirectDraw2_SetDisplayMode(lpDD2, mode->width, mode->height, mode->depth, mode->refresh, 0);
    if (setDisplayModeResult != DD_OK)
    {
      LogError("GfxDrvDirectDraw::SetDisplayMode(): ", setDisplayModeResult);
      return false;
    }
  }

  auto sufacesInitialized = SurfacesInitialize(chipsetBufferDimensions);
  if (!sufacesInitialized)
  {
    SetCooperativeLevelNormal();
  }

  return true;
}

void DirectDrawDevice::ClearCurrentBuffer()
{
  LPDIRECTDRAWSURFACE lpDDS{};
  LPDDSURFACEDESC lpDDSD{};

  SelectTargetSurface(&lpDDS, &lpDDSD);
  SurfaceClear(lpDDS);
}

void DirectDrawDevice::SizeChanged(unsigned int width, unsigned int height)
{
  if (drawmode.IsWindowed)
  {
    _outputWidth = width;
    _outputHeight = height;

    fellow::api::Service->Log.AddLog("DDraw Size changed: %u %u\n", width, height);

    FindWindowClientRect();
    _clearBorders = true;
  }
  else
    fellow::api::Service->Log.AddLog("DDraw fullscreen size ignored: %u %u\n", width, height);
}

void DirectDrawDevice::PositionChanged()
{
  if (drawmode.IsWindowed)
  {
    FindWindowClientRect();
    _clearBorders = true;
  }
}

//======================================================================
// Set the configuration to the display mode
// This display mode is used for host output when starting the emulation
//======================================================================

void DirectDrawDevice::SetDisplayMode(const HostRenderRuntimeSettings &hostRenderRuntimeSettings, const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings, DisplayScale scale)
{
  displayScale = scale;
  _chipsetBufferScaleFactor = chipsetBufferRuntimeSettings.ScaleFactor;
  _chipsetBufferOutputClip = chipsetBufferRuntimeSettings.OutputClipPixels;
  _outputScaleFactor = hostRenderRuntimeSettings.OutputScaleFactor;
  drawmode = hostRenderRuntimeSettings.DisplayMode;

  if (drawmode.IsWindowed)
  {
    fullscreen_mode = nullptr;
  }
  else
  {
    fullscreen_mode = FindFullscreenModeById(drawmode.Id);
    _outputWidth = drawmode.Width;
    _outputHeight = drawmode.Height;
    fellow::api::Service->Log.AddLog("gfxdrv: SetMode() - Fullscreen\n");
  }
}

void DirectDrawDevice::SetColorBitsInformation(const DDPIXELFORMAT &ddpf)
{
  _colorBitsInformation.ColorBits = ddpf.dwRGBBitCount;
  _colorBitsInformation.RedPosition = RGBMaskPos(ddpf.dwRBitMask);
  _colorBitsInformation.RedSize = RGBMaskSize(ddpf.dwRBitMask);
  _colorBitsInformation.GreenPosition = RGBMaskPos(ddpf.dwGBitMask);
  _colorBitsInformation.GreenSize = RGBMaskSize(ddpf.dwGBitMask);
  _colorBitsInformation.BluePosition = RGBMaskPos(ddpf.dwBBitMask);
  _colorBitsInformation.BlueSize = RGBMaskSize(ddpf.dwBBitMask);
}

const GfxDrvColorBitsInformation &DirectDrawDevice::GetColorBitsInformation()
{
  return _colorBitsInformation;
}

bool DirectDrawDevice::SaveScreenshotFromSurfaceArea(LPDIRECTDRAWSURFACE surface, DWORD x, DWORD y, DWORD width, DWORD height, unsigned int lDisplayScale, const char *filename)
{
  DDSURFACEDESC ddsd;
  HDC surfDC = nullptr;

  if (!surface) return FALSE;

  ZeroMemory(&ddsd, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  HRESULT hResult = surface->GetSurfaceDesc(&ddsd);
  if (FAILED(hResult)) return FALSE;

  hResult = surface->GetDC(&surfDC);
  if (FAILED(hResult)) return FALSE;

  bool bSuccess = GfxDrvDCScreenshot::SaveScreenshotFromDCArea(surfDC, x, y, width, height, lDisplayScale, ddsd.ddpfPixelFormat.dwRGBBitCount, filename);

  if (surfDC) surface->ReleaseDC(surfDC);

  return bSuccess;
}

bool DirectDrawDevice::SaveScreenshot(bool bTakeFilteredScreenshot, const char *filename)
{
  bool bResult;
  DWORD width = 0, height = 0, x = 0, y = 0;
  unsigned int lDisplayScale;

  if (bTakeFilteredScreenshot)
  {
#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode())
    {
      width = RP.GetScreenWidthAdjusted();
      height = RP.GetScreenHeightAdjusted();
      x = RP.GetClippingOffsetLeftAdjusted();
      y = RP.GetClippingOffsetTopAdjusted();
      lDisplayScale = RP.GetDisplayScale();
    }
    else
#endif
    {
      width = drawmode.Width;
      height = drawmode.Height;
      lDisplayScale = 1;
    }
    bResult = SaveScreenshotFromSurfaceArea(lpDDSSecondary, x, y, width, height, lDisplayScale, filename);
  }
  else
  {
#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode())
    {
      // width and height in RP mode are sized for maximum scale factor
      //  use harcoded RetroPlatform max PAL dimensions from WinUAE
      width = RETRO_PLATFORM_MAX_PAL_LORES_WIDTH * 2;
      height = RETRO_PLATFORM_MAX_PAL_LORES_HEIGHT * 2;
    }
    else
#endif
    {
      width = drawmode.Width;
      height = drawmode.Height;
    }
    bResult = SaveScreenshotFromSurfaceArea(lpDDSSecondary, x, y, width, height, 1, filename);
  }

  fellow::api::Service->Log.AddLogDebug("gfxDrvDDrawSaveScreenshot(filtered=%d, filename='%s') %s.\n", bTakeFilteredScreenshot, filename, bResult ? "successful" : "failed");

  return bResult;
}

bool DirectDrawDevice::Initialize()
{
  bool result = DirectDrawObjectInitialize();
  if (!result)
  {
    return false;
  }

  result = InitializeFullscreenModeInformation();
  if (!result)
  {
    DirectDraw2ObjectRelease();
    return false;
  }

  _clearBorders = false;
  return true;
}

//==============================================
// Release any resources allocated by Initialize
//==============================================

void DirectDrawDevice::Release()
{
  ReleaseFullscreenModeInformation();
  DirectDraw2ObjectRelease();
}
