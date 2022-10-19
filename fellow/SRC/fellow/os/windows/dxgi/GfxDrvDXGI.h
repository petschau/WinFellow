#pragma once

#include <DXGI.h>
#include <D3D11.h>
#include <DirectXMath.h>
#include <mutex>
#include <string>

#include "fellow/api/defs.h"
#include "GfxDrvDXGIAdapter.h"
#include "GfxDrvDXGIAmigaScreenTexture.h"
#include "GfxDrvDXGIAmigaScreenRenderer.h"
#include "fellow/hud/HudPropertyProvider.h"
#include "GfxDrvHudD2D1.h"
#include "fellow/application/GfxDrvMappedBufferPointer.h"
#include "fellow/application/HostRenderer.h"

using namespace DirectX;

constexpr unsigned int AmigaScreenTextureCount = 1;
constexpr unsigned int BackBufferCount = 2;
constexpr bool UseFlipMode = true;

class GfxDrvDXGI
{
private:
  static bool _requirementsValidated;
  static bool _requirementsValidationResult;

  std::recursive_mutex _fullscreenAndSizeChangeMutex;

  // Adapter meta-data
  GfxDrvDXGIAdapterList *_adapters{};
  GfxDrvHudD2D1 _hud;
  ID3D11DeviceContext *_immediateContext{};
  IDXGIFactory *_dxgiFactory{};
  GfxDrvDXGIAmigaScreenRenderer _amigaScreenRenderer;
  GfxDrvDXGIAmigaScreenTexture _amigaScreenTexture[AmigaScreenTextureCount];

  std::list<DisplayMode> _displayModes;

  unsigned int _amigaScreenTextureCount = AmigaScreenTextureCount;
  unsigned int _currentAmigaScreenTexture{};

  DisplayMode _displayMode{};
  RectPixels _chipsetBufferOutputClipPixels;

  bool _sizeChanged{};
  bool _fullscreenActiveChanged{};
  bool _fullscreenActive{};

  bool CreateAdapterList();
  void DeleteAdapterList();

  std::string GetDisplayModeName(unsigned int width, unsigned int height, unsigned int bits, unsigned int refreshRate);
  void RegisterMode(int id, unsigned int width, unsigned int height, unsigned int refreshRate = 60);
  void RegisterModes();
  void AddFullScreenModes();

  bool CreateD3D11Device();
  void DeleteD3D11Device();

  void DeleteDXGIFactory();
  void DeleteImmediateContext();

  UINT GetPresentFlags();
  UINT GetSwapChainFlags();

  bool CreateSwapChain();
  void DeleteSwapChain();
  bool InitiateSwitchToFullScreen();
  bool ResizeSwapChainBuffers();
  void SetViewport();
  void ExecuteFullscreenAndSizeChanges();

  DXGI_MODE_DESC *GetDXGIMode(unsigned int id);

  bool CreateChipsetTextures(const DimensionsUInt &chipsetBufferSize);
  void DeleteChipsetTextures();
  GfxDrvDXGIAmigaScreenTexture &GetActiveChipsetTexture();

  bool CreateAmigaScreenRenderer();
  void DeleteAmigaScreenRenderer();

  void FlipTexture();

  const char *GetFeatureLevelString(D3D_FEATURE_LEVEL featureLevel) const;

  ID3D11Texture2D *GetBackbuffer();

  void Uninitialize();

  bool IsFullscreen();
  bool IsWindowed();

public:
  ID3D11Device *_d3d11device{};
  IDXGISwapChain *_swapChain{};

  void ClearCurrentBuffer();
  void SetMode(const DisplayMode *dm, bool windowed);
  void SizeChanged(unsigned int width, unsigned int height);
  void PositionChanged();
  void NotifyActiveStatus(bool active);

  std::list<DisplayMode> GetDisplayModeList();

  bool EmulationStart(
      const HostRenderConfiguration &hostRenderConfiguration,
      const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings,
      const DisplayMode &displayMode,
      HudPropertyProvider *hudPropertyProvider);
  unsigned int EmulationStartPost();
  void EmulationStop();

  bool Startup();
  void Shutdown();

  GfxDrvMappedBufferPointer MapChipsetFramebuffer();
  void UnmapChipsetFramebuffer();

  GfxDrvColorBitsInformation GetColorBitsInformation() const;
  void Flip();
  void RenderHud();

  bool SaveScreenshot(bool, const STR *);

  static bool ValidateRequirements();

  GfxDrvDXGI();
  ~GfxDrvDXGI();
};
