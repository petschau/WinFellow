#pragma once

#include <ddraw.h>
#include <string>
#include <list>

#include "fellow/application/DisplayMode.h"
#include "fellow/application/HostRenderConfiguration.h"
#include "fellow/application/HostRenderRuntimeSettings.h"
#include "fellow/chipset/ChipsetBufferRuntimeSettings.h"
#include "fellow/api/drivers/GfxDrvColorBitsInformation.h"

struct DirectDrawFullscreenMode
{
  unsigned int id;
  unsigned int width;
  unsigned int height;
  unsigned int depth;
  unsigned int refresh;
  unsigned int redpos;
  unsigned int redsize;
  unsigned int greenpos;
  unsigned int greensize;
  unsigned int bluepos;
  unsigned int bluesize;
  unsigned int pitch;
};

struct DirectDrawDevice
{
private:
  HWND Hwnd;
  LPDIRECTDRAW lpDD;
  LPDIRECTDRAW2 lpDD2;
  LPDIRECTDRAWSURFACE lpDDSPrimary;   /*!< Primary display surface        */
  LPDIRECTDRAWSURFACE lpDDSBack;      /*!< Current backbuffer for Primary */
  LPDIRECTDRAWSURFACE lpDDSSecondary; /*!< Source surface in blitmode     */
  DDSURFACEDESC ddsdPrimary;
  DDSURFACEDESC ddsdBack;
  DDSURFACEDESC ddsdSecondary;
  LPDIRECTDRAWCLIPPER lpDDClipper;
  std::list<DirectDrawFullscreenMode *> fullscreen_modes;
  DirectDrawFullscreenMode *fullscreen_mode;
  RECT hwnd_clientrect_screen;
  RECT hwnd_clientrect_win;
  DisplayMode drawmode;
  DisplayScale displayScale;
  unsigned int _outputScaleFactor;
  unsigned int _chipsetBufferScaleFactor;
  bool can_stretch_y;
  bool no_dd_hardware;
  unsigned int _outputWidth;
  unsigned int _outputHeight;
  bool _clearBorders;
  GfxDrvColorBitsInformation _colorBitsInformation;
  RectPixels _chipsetBufferOutputClip;

  void LogError(const char *header, HRESULT err);

  void FindWindowClientRect();
  void SelectTargetSurface(LPDIRECTDRAWSURFACE *lpDDS, LPDDSURFACEDESC *lpDDSD);
  void SelectBlitTargetSurface(LPDIRECTDRAWSURFACE *lpDDS);
  void ClipperRelease();
  bool ClipperInitialize();
  bool DirectDraw1ObjectInitialize();
  bool DirectDraw1ObjectRelease();
  bool DirectDraw2ObjectInitialize();
  bool DirectDraw2ObjectRelease();
  bool DirectDrawObjectInitialize();
  DirectDrawFullscreenMode *FindFullscreenModeById(unsigned int id);
  static std::string GetDisplayModeName(const DirectDrawFullscreenMode *displayMode);
  void LogFullscreenModeInformation();
  bool InitializeFullscreenModeInformation();
  void ReleaseFullscreenModeInformation();
  static HRESULT WINAPI EnumerateFullscreenMode(LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext);
  void AddEnumeratedFullscreenMode(LPDDSURFACEDESC lpDDSurfaceDesc);
  static DirectDrawFullscreenMode *NewFullscreenMode(unsigned int id, ULO width, ULO height, ULO depth, ULO refresh, ULO redpos, ULO redsize, ULO greenpos, ULO greensize, ULO bluepos, ULO bluesize);
  constexpr static bool IsRGBWithSupportedBitcount(DDPIXELFORMAT pixelFormat);
  constexpr static bool HasSupportedRefreshRate(DWORD refreshRate);
  constexpr static bool HasSupportedWidth(DWORD width);
  static ULO RGBMaskPos(ULO mask);
  static ULO RGBMaskSize(ULO mask);
  bool SetCooperativeLevelExclusive();
  bool SetCooperativeLevel();
  void SurfaceClear(LPDIRECTDRAWSURFACE surface, RECT *dstrect = nullptr);
  HRESULT SurfaceRestore(LPDIRECTDRAWSURFACE surface);
  void CalculateDestinationRectangle(ULO output_width, ULO output_height, RECT &dstwin);
  bool CreateSecondaryOffscreenSurface(const DimensionsUInt &chipsetBufferSize);
  static const char *GetVideomemoryLocationDescription(ULO pass);
  static ULO GetVideomemoryLocationFlags(ULO pass);
  static void LogPixelFormat(DDPIXELFORMAT ddpf);
  bool SurfacesInitialize(const DimensionsUInt &chipsetBufferSize);
  void ClearWindowBorders();
  static std::string GetPixelFlagDescription(DWORD flags);
  void SetColorBitsInformation(const DDPIXELFORMAT &ddpf);
  bool SaveScreenshotFromSurfaceArea(LPDIRECTDRAWSURFACE surface, DWORD x, DWORD y, DWORD width, DWORD height, unsigned int lDisplayScale, const char *filename);

public:
  GUID GUID;
  std::string DriverDescription;
  std::string DriverName;

  uint8_t *SurfaceLock(ULO *pitch);
  void SurfaceUnlock();
  void ClearCurrentBuffer();

  void SizeChanged(unsigned int width, unsigned int height);
  void PositionChanged();

  std::list<DisplayMode> GetDisplayModeList();

  void SurfaceBlit();
  void SurfacesRelease();
  bool ActivateDisplayMode(const DimensionsUInt &chipsetBufferDimensions, HWND hwnd);
  bool SetCooperativeLevelNormal();
  void SetDisplayMode(const HostRenderRuntimeSettings &hostRenderRuntimeSettings, const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings, DisplayScale scale);
  const GfxDrvColorBitsInformation &GetColorBitsInformation();
  bool SaveScreenshot(bool bTakeFilteredScreenshot, const char *filename);
  bool Initialize();
  void Release();
};
