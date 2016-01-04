#include <InitGuid.h>
#include "GfxDrvDXGI.h"
#include "GfxDrvDXGIAdapterEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "GfxDrvCommon.h"
#include "FELLOW.H"
#include "gfxdrv_directdraw.h"

#include <d3d11.h> 

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif


bool GfxDrvDXGI::Startup()
{
  bool bResult;

  fellowAddLog("GfxDrvDXGI: Starting up DXGI driver\n\n");

  if (!CreateEnumerationFactory())
  {
    return false;
  }
  CreateAdapterList();
  DeleteEnumerationFactory();

#ifdef RETRO_PLATFORM
  if (RetroPlatformGetMode())
    gfxDrvRegisterRetroPlatformScreenMode(true);
#endif

  bResult = (_adapters != 0) & (_adapters->size() > 0);

  fellowAddLog("GfxDrvDXGI: Startup of DXGI driver %s\n\n", bResult ? "successful" : "failed");	

  return bResult;
}

void GfxDrvDXGI::Shutdown()
{
  fellowAddLog("GfxDrvDXGI: Starting to shut down DXGI driver\n\n");

  DeleteAdapterList();

  fellowAddLog("GfxDrvDXGI: Finished shutdown of DXGI driver\n\n");
}

void GfxDrvDXGI::CreateAdapterList()
{
  _adapters = GfxDrvDXGIAdapterEnumerator::EnumerateAdapters(_enumerationFactory);
}

void GfxDrvDXGI::DeleteAdapterList()
{
  if (_adapters != 0)
  {
    GfxDrvDXGIAdapterEnumerator::DeleteAdapterList(_adapters);
    _adapters = 0;
  }
}

bool GfxDrvDXGI::CreateEnumerationFactory()
{
  const HRESULT result = CreateDXGIFactory(__uuidof(IDXGIFactory) ,(void**)&_enumerationFactory);
  if (result != S_OK)
  {
    GfxDrvDXGIErrorLogger::LogError("CreateDXGIFactory failed with the error: ", result);
    return false;
  }

  return true;
}

void GfxDrvDXGI::DeleteEnumerationFactory()
{
  if (_enumerationFactory != 0)
  {
    _enumerationFactory->Release();
    _enumerationFactory = 0;
  }
}

STR* GfxDrvDXGI::GetFeatureLevelString(D3D_FEATURE_LEVEL featureLevel)
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
  D3D_FEATURE_LEVEL featureLevelsSupported;
  HRESULT hr;
  UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
  creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  hr = D3D11CreateDevice(NULL,
                         D3D_DRIVER_TYPE_HARDWARE,
                         NULL,
                         creationFlags,
                         NULL,
                         0,
                         D3D11_SDK_VERSION,
                         &_d3d11device,
                         &featureLevelsSupported,
                         &_immediateContext);
  if (FAILED(hr))
  {
    GfxDrvDXGIErrorLogger::LogError("D3D11CreateDevice failed with the error: ", hr);
    return false;
  }

  IDXGIDevice *dxgiDevice;
  hr = _d3d11device->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);

  if (FAILED(hr))
  {
    fellowAddLog("Failed to query interface for IDXGIDevice\n");
    return false;
  }

  IDXGIAdapter *dxgiAdapter;
  hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&dxgiAdapter);

  if (FAILED(hr))
  {
    dxgiDevice->Release();
    dxgiDevice = 0;

    fellowAddLog("Failed to get IDXGIAdapter via GetParent() on IDXGIDevice\n");
    return false;
  }

  fellowAddLog("The adapter we got was:\n\n");
  GfxDrvDXGIAdapter adapter(dxgiAdapter); // Note: This will eventually release dxgiAdapter in COM. Maybe restructure the enum code later, the code structure ended up not being very practical.
  fellowAddLog("Feature level is: %s\n", GetFeatureLevelString(featureLevelsSupported));

  hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&_dxgiFactory); // Used later to create the swap-chain

  if (FAILED(hr))
  {
    dxgiDevice->Release();
    dxgiDevice = 0;

    fellowAddLog("Failed to get IDXGIFactory via GetParent() on IDXGIAdapter\n");
    return false;
  }

  dxgiDevice->Release();
  dxgiDevice = 0;

  return true;
}

void GfxDrvDXGI::DeleteD3D11Device()
{
  if (_d3d11device != 0)
  {
#ifdef _DEBUG
    ID3D11Debug *d3d11Debug;
    _d3d11device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3d11Debug));
    d3d11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
    d3d11Debug->Release();
    d3d11Debug = 0;
#endif

    _d3d11device->Release();
    _d3d11device = 0;
  }
}

void GfxDrvDXGI::DeleteImmediateContext()
{
  if (_immediateContext != 0)
  {
    _immediateContext->Release();
    _immediateContext = 0;
  }
}

void GfxDrvDXGI::DeleteDXGIFactory()
{
  if (_dxgiFactory != 0)
  {
    _dxgiFactory->Release();
    _dxgiFactory = 0;
  }
}

bool GfxDrvDXGI::CreateAmigaScreenTexture()
{
  int width = _current_draw_mode->width;
  int height = _current_draw_mode->height;

  for (unsigned int i = 0; i < _amigaScreenTextureCount; i++)
  {
    D3D11_TEXTURE2D_DESC texture2DDesc = { 0 };
    texture2DDesc.Width = width;
    texture2DDesc.Height = height;
    texture2DDesc.MipLevels = 1;
    texture2DDesc.ArraySize = 1;
    texture2DDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texture2DDesc.SampleDesc.Count = 1;
    texture2DDesc.SampleDesc.Quality = 0;
    texture2DDesc.Usage = D3D11_USAGE_STAGING;
    texture2DDesc.BindFlags = 0;
    texture2DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    texture2DDesc.MiscFlags = 0;

    HRESULT hr = _d3d11device->CreateTexture2D(&texture2DDesc, 0, &_amigaScreenTexture[i]);

    if (FAILED(hr))
    {
      GfxDrvDXGIErrorLogger::LogError("Failed to create host screen texture.", hr);
      return false;
    }
  }

  return true;
}

void GfxDrvDXGI::DeleteAmigaScreenTexture()
{
  for (unsigned int i = 0; i < _amigaScreenTextureCount; i++)
  {
    if (_amigaScreenTexture[i] != 0)
    {
      _amigaScreenTexture[i]->Release();
      _amigaScreenTexture[i] = 0;
    }
  }
}

ID3D11Texture2D *GfxDrvDXGI::GetCurrentAmigaScreenTexture()
{
  return _amigaScreenTexture[_currentAmigaScreenTexture];
}

bool GfxDrvDXGI::CreateSwapChain()
{
  int width = _current_draw_mode->width;
  int height = _current_draw_mode->height;

#ifdef RETRO_PLATFORM
  if (RetroPlatformGetMode())
  {
    width = RetroPlatformGetScreenWidthAdjusted() / RetroPlatformGetDisplayScale();
    height = RetroPlatformGetScreenHeightAdjusted() / RetroPlatformGetDisplayScale();
  }
#endif

  DXGI_SWAP_CHAIN_DESC swapChainDescription = { 0 };
  DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_DISCARD;

  swapChainDescription.BufferCount = BackBufferCount;
  swapChainDescription.BufferDesc.Width = width;
  swapChainDescription.BufferDesc.Height = height;
  swapChainDescription.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDescription.OutputWindow = gfxDrvCommon->GetHWND();
  swapChainDescription.SampleDesc.Count = 1;
  swapChainDescription.SampleDesc.Quality = 0;
  swapChainDescription.Windowed = TRUE;
  swapChainDescription.SwapEffect = swapEffect;
  swapChainDescription.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;

  HRESULT hr = _dxgiFactory->CreateSwapChain(_d3d11device, &swapChainDescription, &_swapChain);

  if (FAILED(hr))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create swap chain.", hr);
    return false;
  }

  D3D11_VIEWPORT viewPort;
  viewPort.TopLeftX = 0;
  viewPort.TopLeftY = 0;
  viewPort.Width = (FLOAT) width;
  viewPort.Height = (FLOAT) height;
  viewPort.MinDepth = 0.0f;
  viewPort.MaxDepth = 1.0f;
 
  _immediateContext->RSSetViewports(1, &viewPort);
  
  return true;
}

void GfxDrvDXGI::DeleteSwapChain()
{
  if (_swapChain != 0)
  {
    _swapChain->Release();
    _swapChain = 0;
  }
}

void GfxDrvDXGI::SizeChanged()
{
}

unsigned char *GfxDrvDXGI::ValidateBufferPointer()
{
  ID3D11Texture2D *hostBuffer = GetCurrentAmigaScreenTexture();
  D3D11_MAPPED_SUBRESOURCE mappedRect = { 0 };
  D3D11_MAP mapFlag = D3D11_MAP_WRITE;
  
  HRESULT mapResult = _immediateContext->Map(hostBuffer, 0, mapFlag, 0, &mappedRect);

  if (FAILED(mapResult))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to map amiga screen texture:", mapResult);
    return 0;
  }

  _current_draw_mode->pitch = mappedRect.RowPitch;

  return (unsigned char*)mappedRect.pData;
}

void GfxDrvDXGI::InvalidateBufferPointer()
{
  ID3D11Texture2D *hostBuffer = GetCurrentAmigaScreenTexture();
  if (hostBuffer != 0)
  {
    _immediateContext->Unmap(hostBuffer, 0);
  }
}

void GfxDrvDXGI::FlipTexture()
{
  ID3D11Texture2D *amigaScreenBuffer = GetCurrentAmigaScreenTexture();
  ID3D11Texture2D *backBuffer;
  HRESULT getBufferResult = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
  if (FAILED(getBufferResult))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to get back buffer.", getBufferResult);
    return;
  }

  ID3D11RenderTargetView *renderTargetView;
  HRESULT createRenderTargetViewResult = _d3d11device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);

  if (FAILED(createRenderTargetViewResult))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create render target view.", createRenderTargetViewResult);
    return;
  }

  _immediateContext->OMSetRenderTargets(1, &renderTargetView, NULL);
  renderTargetView->Release();
  renderTargetView = 0;

#ifdef RETRO_PLATFORM
  if (RetroPlatformGetMode())
  {
    D3D11_BOX sourceRegion;
    sourceRegion.left = RetroPlatformGetClippingOffsetLeftAdjusted();
    sourceRegion.right = RetroPlatformGetClippingOffsetLeftAdjusted() + RetroPlatformGetScreenWidthAdjusted() / RetroPlatformGetDisplayScale();
    sourceRegion.top = RetroPlatformGetClippingOffsetTopAdjusted();
    sourceRegion.bottom = RetroPlatformGetClippingOffsetTopAdjusted() + RetroPlatformGetScreenHeightAdjusted() / RetroPlatformGetDisplayScale();
    sourceRegion.front = 0;
    sourceRegion.back = 1;

    _immediateContext->CopySubresourceRegion(backBuffer, 0, 0, 0, 0, amigaScreenBuffer, 0, &sourceRegion);
  }
  else
#endif
  _immediateContext->CopyResource(backBuffer, amigaScreenBuffer);

  backBuffer->Release();
  backBuffer = 0;

  HRESULT presentResult = _swapChain->Present(0, 0);

  if (FAILED(presentResult))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to present buffer.", presentResult);
    return;
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

void GfxDrvDXGI::SetMode(draw_mode *dm)
{
  _current_draw_mode = dm;
}

void GfxDrvDXGI::RegisterMode(int width, int height)
{
  ULO id = 0;
  draw_mode *mode = (draw_mode *)malloc(sizeof(draw_mode));

  if (mode)
  {

    mode->width = width;
    mode->height = height;
    mode->bits = 32;
    mode->windowed = TRUE;
    mode->refresh = 60;
    mode->redpos = 16;
    mode->redsize = 8;
    mode->greenpos = 8;
    mode->greensize = 8;
    mode->bluepos = 0;
    mode->bluesize = 8;
    mode->pitch = width * 4;

    mode->id = id;
    if (!mode->windowed)
    {
      char hz[16];
      if (mode->refresh != 0)
      {
	sprintf(hz, "%uHZ", mode->refresh);
      }
      else
      {
	hz[0] = 0;
      }
      sprintf(mode->name, "%uWx%uHx%uBPPx%s", mode->width, mode->height, mode->bits, hz);
    }
    else
    {
      sprintf(mode->name, "%uWx%uHxWindow", mode->width, mode->height);
    }
    drawModeAdd(mode);
  }
}

void GfxDrvDXGI::RegisterModes()
{
  #define GFXWIDTH_NORMAL 640
  #define GFXWIDTH_OVERSCAN 752
  #define GFXWIDTH_MAXOVERSCAN 768

  #define GFXHEIGHT_NTSC 400
  #define GFXHEIGHT_PAL 512
  #define GFXHEIGHT_OVERSCAN 576

  // 1X
  RegisterMode(GFXWIDTH_NORMAL, GFXHEIGHT_NTSC);
  RegisterMode(GFXWIDTH_NORMAL, GFXHEIGHT_PAL);
  RegisterMode(GFXWIDTH_NORMAL, GFXHEIGHT_OVERSCAN);
  RegisterMode(GFXWIDTH_OVERSCAN, GFXHEIGHT_NTSC);
  RegisterMode(GFXWIDTH_OVERSCAN, GFXHEIGHT_PAL);
  RegisterMode(GFXWIDTH_OVERSCAN, GFXHEIGHT_OVERSCAN);
  RegisterMode(GFXWIDTH_MAXOVERSCAN, GFXHEIGHT_OVERSCAN);

  // 2X
  RegisterMode(2 * GFXWIDTH_NORMAL, 2 * GFXHEIGHT_NTSC);
  RegisterMode(2 * GFXWIDTH_NORMAL, 2 * GFXHEIGHT_PAL);
  RegisterMode(2 * GFXWIDTH_NORMAL, 2 * GFXHEIGHT_OVERSCAN);
  RegisterMode(2 * GFXWIDTH_OVERSCAN, 2 * GFXHEIGHT_NTSC);
  RegisterMode(2 * GFXWIDTH_OVERSCAN, 2 * GFXHEIGHT_PAL);
  RegisterMode(2 * GFXWIDTH_OVERSCAN, 2 * GFXHEIGHT_OVERSCAN);
  RegisterMode(2 * GFXWIDTH_MAXOVERSCAN, 2 * GFXHEIGHT_OVERSCAN);

  // 3X
  RegisterMode(3 * GFXWIDTH_NORMAL, 3 * GFXHEIGHT_NTSC);
  RegisterMode(3 * GFXWIDTH_NORMAL, 3 * GFXHEIGHT_PAL);
  RegisterMode(3 * GFXWIDTH_NORMAL, 3 * GFXHEIGHT_OVERSCAN);
  RegisterMode(3 * GFXWIDTH_OVERSCAN, 3 * GFXHEIGHT_NTSC);
  RegisterMode(3 * GFXWIDTH_OVERSCAN, 3 * GFXHEIGHT_PAL);
  RegisterMode(3 * GFXWIDTH_OVERSCAN, 3 * GFXHEIGHT_OVERSCAN);
  RegisterMode(3 * GFXWIDTH_MAXOVERSCAN, 3 * GFXHEIGHT_OVERSCAN);

  // 4X
  RegisterMode(4 * GFXWIDTH_NORMAL, 4 * GFXHEIGHT_NTSC);
  RegisterMode(4 * GFXWIDTH_NORMAL, 4 * GFXHEIGHT_PAL);
  RegisterMode(4 * GFXWIDTH_NORMAL, 4 * GFXHEIGHT_OVERSCAN);
  RegisterMode(4 * GFXWIDTH_OVERSCAN, 4 * GFXHEIGHT_NTSC);
  RegisterMode(4 * GFXWIDTH_OVERSCAN, 4 * GFXHEIGHT_PAL);
  RegisterMode(4 * GFXWIDTH_OVERSCAN, 4 * GFXHEIGHT_OVERSCAN);
  RegisterMode(4 * GFXWIDTH_MAXOVERSCAN, 4 * GFXHEIGHT_OVERSCAN);
}

#ifdef RETRO_PLATFORM

void GfxDrvDXGI::RegisterRetroPlatformScreenMode(const bool bStartup, const ULO lWidth, const ULO lHeight, const ULO lDisplayScale)
{
  fellowAddLog("GfxDrvDXGI: operating in RetroPlatform %ux Direct3D mode, insert resolution %ux%u into list of valid screen resolutions...\n",
    lDisplayScale, lWidth, lHeight);

  RegisterMode(lHeight, lWidth);

  drawSetMode(cfgGetScreenWidth(gfxDrvCommon->rp_startup_config),
    cfgGetScreenHeight(gfxDrvCommon->rp_startup_config),
    cfgGetScreenColorBits(gfxDrvCommon->rp_startup_config),
    cfgGetScreenRefresh(gfxDrvCommon->rp_startup_config),
    cfgGetScreenWindowed(gfxDrvCommon->rp_startup_config));
}

#endif

void GfxDrvDXGI::ClearCurrentBuffer()
{
  UBY* buffer = ValidateBufferPointer();

  if (buffer != 0)
  {
    for (unsigned int y = 0; y < _current_draw_mode->height; y++)
    {
      ULO *line_ptr = (ULO *)buffer;
      for (unsigned int x = 0; x < _current_draw_mode->width; x++)
      {
        *line_ptr++ = 0;
      }
      buffer += _current_draw_mode->pitch;
    }
    InvalidateBufferPointer();
  }
}

bool GfxDrvDXGI::EmulationStart(unsigned int maxbuffercount)
{
  if (!CreateD3D11Device())
  {
    fellowAddLog("Failed to create d3d11 device for host window\n");
    return false;
  }

  if (!CreateSwapChain())
  {
    fellowAddLog("Failed to create swap chain for host window\n");
    return false;
  }

  if (!CreateAmigaScreenTexture())
  {
    fellowAddLog("Failed to create amiga screen texture\n");
    return false;
  }
  return true;
}

unsigned int GfxDrvDXGI::EmulationStartPost()
{
  return _amigaScreenTextureCount;
}

void GfxDrvDXGI::EmulationStop()
{
  if (_immediateContext != 0)
  {
    _immediateContext->ClearState();
  }

  DeleteAmigaScreenTexture();
  DeleteSwapChain();
  DeleteDXGIFactory();
  DeleteImmediateContext();
  DeleteD3D11Device();
}

GfxDrvDXGI::GfxDrvDXGI()
  : _enumerationFactory(0), 
    _adapters(0), 
    _dxgiFactory(0), 
    _swapChain(0), 
    _d3d11device(0), 
    _immediateContext(0), 
    _amigaScreenTextureCount(AmigaScreenTextureCount),
    _currentAmigaScreenTexture(0)
{
  for (unsigned int i = 0; i < _amigaScreenTextureCount; i++)
  {
    _amigaScreenTexture[i] = 0;
  }

  RegisterModes();
}

GfxDrvDXGI::~GfxDrvDXGI()
{
  Shutdown();
}

bool GfxDrvDXGI::SaveScreenshot(const bool bSaveFilteredScreenshot, const STR *filename)
{
  bool bResult = false;
  HRESULT hr;
  DWORD width = 0, height = 0, x = 0, y = 0;
  ULO lDisplayScale;
  IDXGISurface1* pSurface1 = NULL;
  HDC hDC = NULL;
  
  if (bSaveFilteredScreenshot) 
  {
    hr = _swapChain->GetBuffer(0, __uuidof(IDXGISurface1), (void**)&pSurface1);
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

#ifdef RETRO_PLATFORM
    if (RetroPlatformGetMode())
    {
      width = RetroPlatformGetScreenWidthAdjusted();
      height = RetroPlatformGetScreenHeightAdjusted();
      lDisplayScale = RetroPlatformGetDisplayScale();
    }
    else
#endif
    {
      width = _current_draw_mode->width;
      height = _current_draw_mode->height;
      lDisplayScale = 1;
    }

    bResult = gfxDrvDDrawSaveScreenshotFromDCArea(hDC, x, y, width, height, lDisplayScale, 32, filename);
  }
  else
  {
    width = _current_draw_mode->width;
    height = _current_draw_mode->height;
    ID3D11Texture2D *hostBuffer = GetCurrentAmigaScreenTexture();
    ID3D11Texture2D *screenshotTexture = NULL;
    D3D11_TEXTURE2D_DESC texture2DDesc = { 0 };
    texture2DDesc.Width = width;
    texture2DDesc.Height = height;
    texture2DDesc.MipLevels = 1;
    texture2DDesc.ArraySize = 1;
    texture2DDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texture2DDesc.SampleDesc.Count = 1;
    texture2DDesc.SampleDesc.Quality = 0;
    texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
    texture2DDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
    texture2DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    texture2DDesc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

    HRESULT hr = _d3d11device->CreateTexture2D(&texture2DDesc, 0, &screenshotTexture);
    if (FAILED(hr))
    {
      GfxDrvDXGIErrorLogger::LogError("Failed to create screenshot texture.", hr);
      return false;
    }

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
    
    bResult = gfxDrvDDrawSaveScreenshotFromDCArea(hDC, x, y, width, height, 1, 32, filename);

    screenshotTexture->Release();
    screenshotTexture = 0;
  }

  fellowAddLog("GfxDrvDXGI::SaveScreenshot(filtered=%d, filename='%s') %s.\n", bSaveFilteredScreenshot, filename,
    bResult ? "successful" : "failed");

  pSurface1->ReleaseDC(NULL);
  pSurface1->Release();
  pSurface1 = 0;

  return bResult;
}
