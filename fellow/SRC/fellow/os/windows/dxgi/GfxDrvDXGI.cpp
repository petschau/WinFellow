#include <InitGuid.h>

#include "fellow/api/defs.h"
#include "GfxDrvDXGI.h"
#include "GfxDrvDXGIUtils.h"
#include "GfxDrvDXGIAdapterEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "fellow/api/Drivers.h"
#include "fellow/os/windows/graphics/GraphicsDriver.h"

#include "GfxDrvHudD2D1.h"

#include "fellow/api/Services.h"
#include "fellow/os/windows/directdraw/GfxDrvDCScreenshot.h"

#ifdef RETRO_PLATFORM
#include "fellow/os/windows/retroplatform/RetroPlatform.h"
#endif

using namespace std;
using namespace fellow::api;

bool GfxDrvDXGI::_requirementsValidated = false;
bool GfxDrvDXGI::_requirementsValidationResult = false;

bool GfxDrvDXGI::ValidateRequirements()
{
  if (_requirementsValidated)
  {
    return _requirementsValidationResult;
  }

  _requirementsValidated = true;

  HINSTANCE hDll = LoadLibrary("d3d11.dll");
  if (hDll)
  {
    FreeLibrary(hDll);
  }
  else
  {
    Service->Log.AddLog("GfxDrvDXGI::ValidateRequirements() ERROR: d3d11.dll could not be loaded.\n");
    _requirementsValidationResult = false;
    return false;
  }

  hDll = LoadLibrary("dxgi.dll");
  if (hDll)
  {
    FreeLibrary(hDll);
  }
  else
  {
    Service->Log.AddLog("GfxDrvDXGI::ValidateRequirements() ERROR: dxgi.dll could not be loaded.\n");
    _requirementsValidationResult = false;
    return false;
  }

  GfxDrvDXGI dxgi;
  bool adaptersFound = dxgi.CreateAdapterList();
  if (!adaptersFound)
  {
    Service->Log.AddLog("GfxDrvDXGI::ValidateRequirements() ERROR: Direct3D 11 present but no DXGI capable adapters found, falling back to DirectDraw.\n");
    _requirementsValidationResult = false;
    return false;
  }

  _requirementsValidationResult = true;
  return true;
}

bool GfxDrvDXGI::IsFullscreen() const
{
  return !_displayMode.IsWindowed;
}

bool GfxDrvDXGI::IsWindowed() const
{
  return _displayMode.IsWindowed;
}

// Returns true if adapter enumeration was successful and the DXGI system actually has an adapter. (Like with VirtualBox in some hosts DXGI/D3D11 is present, but no adapters.)
bool GfxDrvDXGI::CreateAdapterList()
{
  DeleteAdapterList();

  IDXGIFactory *enumerationFactory;
  const HRESULT result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&enumerationFactory);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("CreateDXGIFactory failed with the error: ", result);
    return false;
  }

  _adapters = GfxDrvDXGIAdapterEnumerator::EnumerateAdapters(enumerationFactory);

  ReleaseCOM(&enumerationFactory);

  return _adapters != nullptr && !_adapters->empty();
}

void GfxDrvDXGI::DeleteAdapterList()
{
  if (_adapters != nullptr)
  {
    GfxDrvDXGIAdapterEnumerator::DeleteAdapterList(_adapters);
    _adapters = nullptr;
  }
}

const char *GfxDrvDXGI::GetFeatureLevelString(D3D_FEATURE_LEVEL featureLevel) const
{
  switch (featureLevel)
  {
    case D3D_FEATURE_LEVEL_11_0: return "D3D_FEATURE_LEVEL_11_0";
    case D3D_FEATURE_LEVEL_10_1: return "D3D_FEATURE_LEVEL_10_1";
    case D3D_FEATURE_LEVEL_10_0: return "D3D_FEATURE_LEVEL_10_0";
    case D3D_FEATURE_LEVEL_9_3: return "D3D_FEATURE_LEVEL_9_3";
    case D3D_FEATURE_LEVEL_9_2: return "D3D_FEATURE_LEVEL_9_2";
    case D3D_FEATURE_LEVEL_9_1: return "D3D_FEATURE_LEVEL_9_1";
  }
  return "Unknown feature level";
}

bool GfxDrvDXGI::CreateD3D11Device()
{
  D3D_FEATURE_LEVEL featureLevelsSupported{};
  UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
  // VS2013 Update 5 on Windows 10 may require execution of the following
  // command to enable use of the debug layer; this installs the optional
  // "Graphics Tools" feature:
  // dism /online /add-capability /capabilityname:Tools.Graphics.DirectX~~~~0.0.1.0

  creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, nullptr, 0, D3D11_SDK_VERSION, &_d3d11device, &featureLevelsSupported, &_immediateContext);
  if (FAILED(hr))
  {
    GfxDrvDXGIErrorLogger::LogError("D3D11CreateDevice failed with the error: ", hr);
    return false;
  }

  IDXGIDevice *dxgiDevice;
  hr = _d3d11device->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);

  if (FAILED(hr))
  {
    Service->Log.AddLog("Failed to query interface for IDXGIDevice\n");
    return false;
  }

  IDXGIAdapter *dxgiAdapter;
  hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&dxgiAdapter);

  if (FAILED(hr))
  {
    ReleaseCOM(&dxgiDevice);
    Service->Log.AddLog("Failed to get IDXGIAdapter via GetParent() on IDXGIDevice\n");
    return false;
  }

  Service->Log.AddLog("The adapter we got was:\n\n");
  GfxDrvDXGIAdapter adapter(dxgiAdapter); // Note: This will eventually release dxgiAdapter in COM. Maybe restructure the enum code later, the code structure ended up not being very practical.
  Service->Log.AddLog("Feature level is: %s\n", GetFeatureLevelString(featureLevelsSupported));

  hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&_dxgiFactory); // Used later to create the swap-chain

  if (FAILED(hr))
  {
    ReleaseCOM(&dxgiDevice);

    Service->Log.AddLog("Failed to get IDXGIFactory via GetParent() on IDXGIAdapter\n");
    return false;
  }

  ReleaseCOM(&dxgiDevice);
  return true;
}

void GfxDrvDXGI::DeleteD3D11Device()
{
#ifdef _DEBUG
  if (_d3d11device != nullptr)
  {
    ID3D11Debug *d3d11Debug;
    _d3d11device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void **>(&d3d11Debug));
    if (d3d11Debug != nullptr)
    {
      d3d11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
      ReleaseCOM(&d3d11Debug);
    }
  }
#endif

  ReleaseCOM(&_d3d11device);
}

void GfxDrvDXGI::DeleteImmediateContext()
{
  ReleaseCOM(&_immediateContext);
}

void GfxDrvDXGI::DeleteDXGIFactory()
{
  ReleaseCOM(&_dxgiFactory);
}

bool GfxDrvDXGI::CreateChipsetTextures(const DimensionsUInt &chipsetBufferSize)
{
  for (unsigned int i = 0; i < _amigaScreenTextureCount; i++)
  {
    bool result = _amigaScreenTexture[i].Configure(_d3d11device, _immediateContext, chipsetBufferSize);
    if (!result)
    {
      DeleteChipsetTextures();
      return false;
    }
  }

  return true;
}

void GfxDrvDXGI::DeleteChipsetTextures()
{
  for (unsigned int i = 0; i < _amigaScreenTextureCount; i++)
  {
    _amigaScreenTexture[i].Uninitialize();
  }
}

bool GfxDrvDXGI::CreateAmigaScreenRenderer()
{
  const GfxDrvDXGIAmigaScreenTexture &chipsetTexture = GetActiveChipsetTexture();
  return _amigaScreenRenderer.Configure(_d3d11device, _immediateContext, ((GraphicsDriver&)Driver->Graphics).GetHostBufferSize(), chipsetTexture.GetSize(), _chipsetBufferOutputClipPixels);
}

void GfxDrvDXGI::DeleteAmigaScreenRenderer()
{
  _amigaScreenRenderer.ReleaseAllocatedResources();
}

GfxDrvDXGIAmigaScreenTexture &GfxDrvDXGI::GetActiveChipsetTexture()
{
  return _amigaScreenTexture[_currentAmigaScreenTexture];
}

GfxDrvColorBitsInformation GfxDrvDXGI::GetColorBitsInformation() const
{
  // Fixed values, DXGI backbuffer is always 32-bit
  return GfxDrvColorBitsInformation{.ColorBits = 32, .RedSize = 8, .RedPosition = 16, .GreenSize = 8, .GreenPosition = 8, .BlueSize = 8, .BluePosition = 0};
}

bool GfxDrvDXGI::CreateSwapChain()
{
  UINT width = _displayMode.Width;
  UINT height = _displayMode.Height;

  DXGI_SWAP_CHAIN_DESC swapChainDescription = {};
  DXGI_SWAP_EFFECT swapEffect;

  if (UseFlipMode)
  {
    swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  }
  else
  {
    swapEffect = DXGI_SWAP_EFFECT_DISCARD;
  }

  swapChainDescription.BufferCount = BackBufferCount;
  swapChainDescription.BufferDesc.Width = width;
  swapChainDescription.BufferDesc.Height = height;
  swapChainDescription.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDescription.OutputWindow = ((GraphicsDriver &)Driver->Graphics).GetHWND();
  swapChainDescription.SampleDesc.Count = 1;
  swapChainDescription.SampleDesc.Quality = 0;
  swapChainDescription.Windowed = TRUE;
  swapChainDescription.SwapEffect = swapEffect;
  swapChainDescription.Flags = GetSwapChainFlags();

  HRESULT hr = _dxgiFactory->CreateSwapChain(_d3d11device, &swapChainDescription, &_swapChain);

  if (FAILED(hr))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create swap chain.", hr);
    return false;
  }

  SetViewport();

  return true;
}

void GfxDrvDXGI::DeleteSwapChain()
{
  if (IsFullscreen() && _swapChain != nullptr)
  {
    _swapChain->SetFullscreenState(FALSE, nullptr);
  }

  ReleaseCOM(&_swapChain);
}

void GfxDrvDXGI::SetViewport()
{
  D3D11_VIEWPORT viewPort{};
  viewPort.TopLeftX = 0;
  viewPort.TopLeftY = 0;

  // TODO: Double check the source, might not be displayMode
  viewPort.Width = (FLOAT)_displayMode.Width;
  viewPort.Height = (FLOAT)_displayMode.Height;
  viewPort.MinDepth = 0.0f;
  viewPort.MaxDepth = 1.0f;

  _immediateContext->RSSetViewports(1, &viewPort);
}

bool GfxDrvDXGI::InitiateSwitchToFullScreen()
{
  Service->Log.AddLog("GfxDrvDXGI::InitiateSwitchToFullScreen()\n");

  // Match configured displaymode to the mode description provided by the adapter and output
  DXGI_MODE_DESC *modeDescription = GetDXGIMode(_displayMode.Id);
  if (modeDescription == nullptr)
  {
    Service->Log.AddLog("GfxDrvDXGI::InitiateSwitchToFullScreen() - Selected fullscreen mode was not found in the list of supported modes by the adapter and output\n");
    return false;
  }

  // According to DXGI best practice, ResizeTarget should be called first
  // Then SetFullscreenState and finally again ResizeTarget with the refresh-rate parameter zeroed out

  HRESULT resizeTargetResult = _swapChain->ResizeTarget(modeDescription);
  if (FAILED(resizeTargetResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::InitiateSwitchToFullScreen() - Failed to resize target", resizeTargetResult);
    return false;
  }

  HRESULT fullscreenResult = _swapChain->SetFullscreenState(TRUE, nullptr);
  if (FAILED(fullscreenResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::InitiateSwitchToFullScreen() - Failed to set full-screen", fullscreenResult);
    return false;
  }

  modeDescription->RefreshRate.Denominator = modeDescription->RefreshRate.Numerator = 0;
  HRESULT resizeTargetResultAgain = _swapChain->ResizeTarget(modeDescription);
  if (FAILED(resizeTargetResultAgain))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::InitiateSwitchToFullScreen() - Failed to resize target with refresh 0", resizeTargetResultAgain);
    return false;
  }

  if (!ResizeSwapChainBuffers())
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::InitiateSwitchToFullScreen() - Failed to resize swap chain buffers", resizeTargetResultAgain);
    return false;
  }

  return true;
}

DXGI_MODE_DESC *GfxDrvDXGI::GetDXGIMode(unsigned int id)
{
  if (_adapters->empty())
  {
    return nullptr;
  }

  GfxDrvDXGIAdapter *firstAdapter = _adapters->front();
  if (firstAdapter->GetOutputs().empty())
  {
    return nullptr;
  }

  GfxDrvDXGIOutput *firstOutput = firstAdapter->GetOutputs().front();
  for (GfxDrvDXGIMode *mode : firstOutput->GetModes())
  {
    if (mode->GetId() == id)
    {
      return mode->GetDXGIModeDescription();
    }
  }

  return nullptr;
}

void GfxDrvDXGI::NotifyActiveStatus(bool active)
{
  Service->Log.AddLog("GfxDrvDXGI::NotifyActiveStatus(%s)\n", active ? "TRUE" : "FALSE");

  if (IsFullscreen() && _swapChain != nullptr)
  {
    // This can flag some debug-messages from direct3d when tabbing in and out of fullscreen.
    // Does not seem to matter.
    // Tried using a mutex on the swap chain, unfortunately it seems that it is the swapchain Present that
    // triggers the activate message, so it would deadlock

    _fullscreenAndSizeChangeMutex.lock();
    _fullscreenActiveChanged = true;
    _fullscreenActive = active;
    _fullscreenAndSizeChangeMutex.unlock();

    if (!_fullscreenActive)
    {
      ((GraphicsDriver &)Driver->Graphics).HideWindow(); // Sends WM_SIZE
    }
  }
}

// Runs on the message thread, or maybe not always? Looks like the swapchain can also call this one. Driver dependent?
void GfxDrvDXGI::SizeChanged(unsigned int width, unsigned int height)
{
  Service->Log.AddLog("GfxDrvDXGI: SizeChanged(%u, %u)\n", width, height);

  // TODO: Either remove or improve, this did not lead to any improvements as it is now
  if (_swapChain != nullptr)
  {
    _fullscreenAndSizeChangeMutex.lock();
    _sizeChanged = true;
    _fullscreenAndSizeChangeMutex.unlock();
  }
}

void GfxDrvDXGI::PositionChanged()
{
}

UINT GfxDrvDXGI::GetSwapChainFlags()
{
  UINT flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;
  if (UseFlipMode)
  {
    flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  }
  return flags;
}

bool GfxDrvDXGI::ResizeSwapChainBuffers()
{
  Service->Log.AddLog("GfxDrvDXGI: ResizeSwapChainBuffers()\n");

  _hud.Resizing();

  HRESULT result = _swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, GetSwapChainFlags());
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to resize buffers of swap chain in response to WM_SIZE:", result);
    return false;
  }

  // Output size could have been changed
  SetViewport();

  // Recalculate coordinates for output geometry, will only do work if size actually changed
  if (!CreateAmigaScreenRenderer())
  {
    Service->Log.AddLog("GfxDrvDXGI::ResizeSwapChainBuffers() - Failed to re-create amiga screen renderer\n");
    return false;
  }

  if (!_hud.Resized(_displayMode.Width, _displayMode.Height, _swapChain))
  {
    Service->Log.AddLog("GfxDrvDXGI::ResizeSwapChainBuffers() - Failed to re-size the HUD renderer\n");
    return false;
  }

  return true;
}

GfxDrvMappedBufferPointer GfxDrvDXGI::MapChipsetFramebuffer()
{
  const GfxDrvDXGIMapResult mapResult = GetActiveChipsetTexture().MapAmigaScreen();
  return GfxDrvMappedBufferPointer{.Buffer = mapResult.Buffer, .Pitch = mapResult.Pitch, .IsValid = mapResult.Buffer != nullptr};
}

void GfxDrvDXGI::UnmapChipsetFramebuffer()
{
  GetActiveChipsetTexture().UnmapAmigaScreen();
}

UINT GfxDrvDXGI::GetPresentFlags()
{
  UINT flags = 0;
  if (UseFlipMode && IsWindowed())
  {
    flags |= DXGI_PRESENT_ALLOW_TEARING;
  }
  return flags;
}

void GfxDrvDXGI::ExecuteFullscreenAndSizeChanges()
{
  _fullscreenAndSizeChangeMutex.lock();

  if (_fullscreenActiveChanged)
  {
    Service->Log.AddLog("Execute fullscreen active changed\n");
    _swapChain->SetFullscreenState(_fullscreenActive, nullptr);
    _fullscreenActiveChanged = false;
    _sizeChanged = true;
  }

  if (_sizeChanged)
  {
    Service->Log.AddLog("Execute size changed active changed\n");
    ResizeSwapChainBuffers();
    _sizeChanged = false;
  }
  _fullscreenAndSizeChangeMutex.unlock();
}

ID3D11Texture2D *GfxDrvDXGI::GetBackbuffer()
{
  ID3D11Texture2D *backbuffer;
  HRESULT getBackbufferResult = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&backbuffer);
  if (FAILED(getBackbufferResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::GetCurrentBackbuffer() - Failed to get back buffer", getBackbufferResult);
    return nullptr;
  }

  return backbuffer;
}

void GfxDrvDXGI::FlipTexture()
{
  ExecuteFullscreenAndSizeChanges();

  ID3D11Texture2D *backbuffer = GetBackbuffer();
  if (backbuffer != nullptr)
  {
    _amigaScreenRenderer.Render(GetActiveChipsetTexture(), backbuffer);

    ReleaseCOM(&backbuffer);

    _hud.DrawHUD(Service->HUD.GetHudConfiguration());

    HRESULT presentResult = _swapChain->Present(0, GetPresentFlags());
    if (FAILED(presentResult))
    {
      GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::FlipTexture() - Failed to present", presentResult);
    }
  }
}

void GfxDrvDXGI::Flip()
{
  FlipTexture();

  _currentAmigaScreenTexture++;
  if (_currentAmigaScreenTexture >= _amigaScreenTextureCount)
  {
    _currentAmigaScreenTexture = 0;
  }
}

string GfxDrvDXGI::GetDisplayModeName(unsigned int width, unsigned int height, unsigned int bits, unsigned int refreshRate)
{
  char hz[16];
  if (refreshRate != 0)
  {
    sprintf(hz, "%uHZ", refreshRate);
  }
  else
  {
    hz[0] = '\0';
  }

  char name[64];
  sprintf(name, "%uWx%uHx%uBPPx%s", width, height, bits, hz);

  return name;
}

list<DisplayMode> GfxDrvDXGI::GetDisplayModeList()
{
  return _displayModes;
}

void GfxDrvDXGI::RegisterMode(int id, unsigned int width, unsigned int height, unsigned int refreshRate)
{
  _displayModes.emplace_back(DisplayMode(id, width, height, 32, refreshRate, GetDisplayModeName(width, height, 32, refreshRate)));
}

void GfxDrvDXGI::AddFullScreenModes()
{
  _displayModes.clear();

  if (_adapters->empty())
  {
    return;
  }

  GfxDrvDXGIAdapter *firstAdapter = _adapters->front();
  if (firstAdapter->GetOutputs().empty())
  {
    return;
  }

  GfxDrvDXGIOutput *firstOutput = firstAdapter->GetOutputs().front();
  for (GfxDrvDXGIMode *mode : firstOutput->GetModes())
  {
    if (mode->CanUseMode())
    {
      RegisterMode(mode->GetId(), mode->GetWidth(), mode->GetHeight(), mode->GetRefreshRate());
    }
  }
}

void GfxDrvDXGI::RegisterModes()
{
  AddFullScreenModes();
}

void GfxDrvDXGI::ClearCurrentBuffer()
{
  GetActiveChipsetTexture().ClearAmigaScreen();
}

void GfxDrvDXGI::RenderHud()
{
  // if (!UseOverlay) return;

  // IDXGISurface1* overlaySurface{};
  // HRESULT hr = GetCurrentOverlayTexture()->QueryInterface(__uuidof(IDXGISurface1), (void**)&overlaySurface);
  // if (FAILED(hr))
  //{
  //   GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::RenderHud(): Failed to obtain IDXGISurface1 interface for overlay surface.", hr);
  //   return;
  // }
  // HDC hdc = 0;
  // HRESULT result = overlaySurface->GetDC(FALSE, &hdc);
  // if (FAILED(result))
  //{
  //   Service->Log.AddLog("GfxDrvDXGI::RenderHud() - Failed to get DC for overlaySurface\n");
  //   return;
  // }

  //// Drawing hud on the final backbuffer, text must be scaled according to actual window size
  // GfxDrvHud::RenderHud(hdc, drawGetOutputScaleFactor());

  // overlaySurface->ReleaseDC(NULL);
  // ReleaseCOM(&overlaySurface);
}

void GfxDrvDXGI::Uninitialize()
{
  if (_immediateContext != nullptr)
  {
    _immediateContext->ClearState();
  }

  DeleteChipsetTextures();
  DeleteAmigaScreenRenderer();
  DeleteSwapChain();
  DeleteDXGIFactory();
  DeleteImmediateContext();
  DeleteD3D11Device();
}

bool GfxDrvDXGI::EmulationStart(
    const HostRenderConfiguration &hostRenderConfiguration, const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings, const DisplayMode &displayMode, HudPropertyProvider *hudPropertyProvider)
{
  _displayMode = displayMode;
  _chipsetBufferOutputClipPixels = chipsetBufferRuntimeSettings.OutputClipPixels;

  if (!CreateD3D11Device())
  {
    Service->Log.AddLog("GfxDrvDXGI::EmulationStart() - Failed to create d3d11 device for host window\n");
    return false;
  }

  if (!CreateSwapChain())
  {
    Service->Log.AddLog("GfxDrvDXGI::EmulationStart() - Failed to create swap chain for host window\n");
    Uninitialize();
    return false;
  }

  if (!CreateChipsetTextures(chipsetBufferRuntimeSettings.Dimensions))
  {
    Service->Log.AddLog("GfxDrvDXGI::EmulationStart() - Failed to create amiga screen textures\n");
    Uninitialize();
    return false;
  }

  _hud.EmulationStart(_chipsetBufferOutputClipPixels.GetWidth(), _chipsetBufferOutputClipPixels.GetHeight(), _swapChain, hudPropertyProvider);

  return true;
}

unsigned int GfxDrvDXGI::EmulationStartPost()
{
  // At this point the size of the window is known

  Service->Log.AddLog("GfxDrvDXGI::EmulationStartPost() - Output is %s\n", IsFullscreen() ? "fullscreen" : "windowed");

  if (!CreateAmigaScreenRenderer())
  {
    Service->Log.AddLog("GfxDrvDXGI::EmulationStartPost() - Failed to create amiga screen renderer\n");
    Uninitialize();
    return false;
  }

  if (IsFullscreen())
  {
    bool fullscreenOk = InitiateSwitchToFullScreen();

    if (!fullscreenOk)
    {
      Uninitialize();
      return 0;
    }
  }

  return _amigaScreenTextureCount;
}

void GfxDrvDXGI::EmulationStop()
{
  if (_immediateContext != nullptr)
  {
    _immediateContext->ClearState();
  }

  _hud.EmulationStop();

  DeleteChipsetTextures();
  DeleteAmigaScreenRenderer();
  DeleteSwapChain();
  DeleteDXGIFactory();
  DeleteImmediateContext();
  DeleteD3D11Device();
}

bool GfxDrvDXGI::Startup()
{
  Service->Log.AddLog("GfxDrvDXGI: Starting up DXGI driver\n");

  bool success = CreateAdapterList();
  if (success)
  {
    RegisterModes();
  }
  Service->Log.AddLog("GfxDrvDXGI: Startup of DXGI driver %s\n", success ? "successful" : "failed");
  return success;
}

void GfxDrvDXGI::Shutdown()
{
  Service->Log.AddLog("GfxDrvDXGI: Starting to shut down DXGI driver\n");

  DeleteAdapterList();

  Service->Log.AddLog("GfxDrvDXGI: Finished shutdown of DXGI driver\n");
}

GfxDrvDXGI::GfxDrvDXGI()
{
}

GfxDrvDXGI::~GfxDrvDXGI()
{
  Shutdown();
}

/**
* Saves the current Amiga screen to an image file
*
* A filtered screenshot comes from the DXGI swapchain.
* This image will have the same size and aspect ratio as the current window.
* All scaling and clipping effects have been applied.
*
* A non-filtered screenshot comes from the internal backbuffer where the emulator first creates the image of the Amiga screen as emulation progresses.
* It is always "hires x 512i" or "shres x 512i" in scale.
* Simple scaling is already applied to this buffer (solid, scanline, deinterlacing)
* It can contain a larger part of the Amiga screen than is finally output in the emulation window. Depends on the config.
* Config created by the UI will usually ensure internal and external buffer limits match.
* Retroplatform mode always renders an overscan area that has parts that might not be transferred to the window.
* Does not have final scaling or clipping done by the GPU (upscale to 3x, 4x or height aspect ratio adjustment from a shres sized internal buffer)
*
* @param bSaveFilteredScreenshot Selects the source buffer for the image.
* @param filename File to save to

*/
bool GfxDrvDXGI::SaveScreenshot(const bool bSaveFilteredScreenshot, const char *filename)
{
  bool bResult = false;
  DWORD x = 0, y = 0;
  unsigned int lDisplayScale;
  IDXGISurface1 *pSurface1 = nullptr;
  HDC hDC = nullptr;

#ifdef _DEBUG
  Service->Log.AddLog("GfxDrvDXGI::SaveScreenshot(filtered=%s, filename=%s)\n", bSaveFilteredScreenshot ? "true" : "false", filename);
#endif

  if (bSaveFilteredScreenshot)
  {
    HRESULT hr = _swapChain->GetBuffer(0, __uuidof(IDXGISurface1), (void **)&pSurface1);
    if (FAILED(hr))
    {
      GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::SaveScreenshot(): Failed to obtain IDXGISurface1 interface for filtered screenshot.", hr);
      return false;
    }

    hr = pSurface1->GetDC(FALSE, &hDC);
    if (FAILED(hr))
    {
      GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::SaveScreenshot(): Failed to obtain GDI compatible device context for filtered screenshot.", hr);
      return false;
    }

    DWORD width = 0, height = 0;

#ifdef RETRO_PLATFORM
    if (RP.GetHeadlessMode())
    {
      width = RP.GetScreenWidthAdjusted();
      height = RP.GetScreenHeightAdjusted();
      lDisplayScale = RP.GetDisplayScale();
    }
    else
#endif
    {
      width = _displayMode.Width;
      height = _displayMode.Height;
      lDisplayScale = 1;
    }

    bResult = GfxDrvDCScreenshot::SaveScreenshotFromDCArea(hDC, x, y, width, height, 1, 32, filename);
  }
  else
  { // save unfiltered screenshot
    // width  = _current_draw_mode->width;
    // height = _current_draw_mode->height;
    GfxDrvDXGIAmigaScreenTexture &chipsetScreenTexture = GetActiveChipsetTexture();
    ID3D11Texture2D *hostBuffer = chipsetScreenTexture.GetAmigaScreenD3D11Texture();
    ID3D11Texture2D *screenshotTexture = nullptr;
    D3D11_TEXTURE2D_DESC texture2DDesc;
    const auto &chipsetSize = chipsetScreenTexture.GetSize();
    texture2DDesc.Width = chipsetSize.Width;
    texture2DDesc.Height = chipsetSize.Height;
    texture2DDesc.MipLevels = 1;
    texture2DDesc.ArraySize = 1;
    texture2DDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texture2DDesc.SampleDesc.Count = 1;
    texture2DDesc.SampleDesc.Quality = 0;
    texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
    texture2DDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
    // CheckFeatureSupport() API with D3D11_FEATURE_D3D11_OPTIONS2 must report support for MapOnDefaultTextures for a D3D11_USAGE_DEFAULT Texture Resource to have CPUAccessFlags set.
    texture2DDesc.CPUAccessFlags = 0; // D3D11_CPU_ACCESS_WRITE;
    texture2DDesc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

    HRESULT hr = _d3d11device->CreateTexture2D(&texture2DDesc, nullptr, &screenshotTexture);
    if (FAILED(hr))
    {
      GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::SaveScreenshot(): Failed to create screenshot texture.", hr);
      return false;
    }

    // ID3D11DeviceContext::CopyResource: Cannot invoke CopyResource when the source and destination are not the same Resource type, nor have equivalent dimensions.
    _immediateContext->CopyResource(screenshotTexture, hostBuffer);

    hr = screenshotTexture->QueryInterface(__uuidof(IDXGISurface1), (void **)&pSurface1);
    if (FAILED(hr))
    {
      GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::SaveScreenshot(): Failed to obtain IDXGISurface1 interface for unfiltered screenshot.", hr);
      return false;
    }

    hr = pSurface1->GetDC(FALSE, &hDC);
    if (FAILED(hr))
    {
      GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGI::SaveScreenshot(): Failed to obtain GDI compatible device context for unfiltered screenshot.", hr);
      return false;
    }

    bResult = GfxDrvDCScreenshot::SaveScreenshotFromDCArea(hDC, x, y, chipsetSize.Width, chipsetSize.Height, 1, 32, filename);

    ReleaseCOM(&screenshotTexture);
  }

  Service->Log.AddLog("GfxDrvDXGI::SaveScreenshot(filtered=%s, filename='%s') %s.\n", bSaveFilteredScreenshot ? "true" : "false", filename, bResult ? "successful" : "failed");

  pSurface1->ReleaseDC(nullptr);

  ReleaseCOM(&pSurface1);

  return bResult;
}
