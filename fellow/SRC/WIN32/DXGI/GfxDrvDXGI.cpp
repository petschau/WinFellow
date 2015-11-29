#include "GfxDrvDXGI.h"
#include "GfxDrvDXGIAdapterEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "GfxDrvCommon.h"
#include "FELLOW.H"

#include <d3d11.h> 

bool GfxDrvDXGI::Startup()
{
  fellowAddLog("GfxDrvDXGI: Starting up DXGI driver\n\n");

  if (!CreateEnumerationFactory())
  {
    return false;
  }
  CreateAdapterList();
  DeleteEnumerationFactory();

  fellowAddLog("GfxDrvDXGI: Finished starting up DXGI driver\n\n");	

  return _adapters != 0;
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
  D3D_FEATURE_LEVEL  featureLevelsSupported;

  HRESULT hr = D3D11CreateDevice(NULL,
				 D3D_DRIVER_TYPE_HARDWARE,
				 NULL,
				 0,
				 NULL,
				 0,
				 D3D11_SDK_VERSION,
				 &_d3d11device,
				 &featureLevelsSupported,
				 &_immediateContext);

  if (hr != S_OK)
  {
    GfxDrvDXGIErrorLogger::LogError("D3D11CreateDevice failed with the error: ", hr);
    return false;
  }

  IDXGIDevice *dxgiDevice;
  hr = _d3d11device->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);
      
  IDXGIAdapter *dxgiAdapter;
  hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&dxgiAdapter);

  fellowAddLog("The adapter we got was:\n\n");
  GfxDrvDXGIAdapter adapter(dxgiAdapter);
  fellowAddLog("Feature level is: %s\n", GetFeatureLevelString(featureLevelsSupported));

  dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&_dxgiFactory);

  dxgiAdapter->Release();
  dxgiDevice->Release();

  return true;
}

void GfxDrvDXGI::DeleteD3D11Device()
{
  if (_d3d11device != 0)
  {
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
    D3D11_TEXTURE2D_DESC texture2DDesc;
    ZeroMemory(&texture2DDesc, sizeof(texture2DDesc));

    D3D11_USAGE usage = D3D11_USAGE_STAGING;
    UINT bindFlag = 0;

    texture2DDesc.Width = width;
    texture2DDesc.Height = height;
    texture2DDesc.MipLevels = 1;
    texture2DDesc.ArraySize = 1;
    texture2DDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texture2DDesc.SampleDesc.Count = 1;
    texture2DDesc.SampleDesc.Quality = 0;
    texture2DDesc.Usage = usage;
    texture2DDesc.BindFlags = bindFlag;
    texture2DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    texture2DDesc.MiscFlags = 0;

    HRESULT hr = _d3d11device->CreateTexture2D(&texture2DDesc, 0, &_amigaScreenTexture[i]);

    if (hr != S_OK)
    {
      GfxDrvDXGIErrorLogger::LogError("Failed to create host screen texture1:", hr);
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

  DXGI_SWAP_CHAIN_DESC swapChainDescription;
  ZeroMemory( &swapChainDescription, sizeof(swapChainDescription));

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

  HRESULT hr = _dxgiFactory->CreateSwapChain(_d3d11device, &swapChainDescription, &_swapChain);

  if (hr != S_OK)
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
  D3D11_MAPPED_SUBRESOURCE mappedRect;
  ZeroMemory(&mappedRect, sizeof(mappedRect));
  D3D11_MAP mapFlag = D3D11_MAP_WRITE;

  HRESULT mapResult = _immediateContext->Map(hostBuffer, 0, mapFlag, 0, &mappedRect);

  if (mapResult != S_OK)
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to map host screen texture:", mapResult);
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
  if (getBufferResult != S_OK)
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to get back buffer.", getBufferResult);
    return;
  }

  ID3D11RenderTargetView *renderTargetView;
  HRESULT createRenderTargetViewResult = _d3d11device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);
  backBuffer->Release();

  if (createRenderTargetViewResult != S_OK)
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create render target view.", createRenderTargetViewResult);
    return;
  }

  _immediateContext->OMSetRenderTargets(1, &renderTargetView, NULL);
  renderTargetView->Release();

  _immediateContext->CopyResource(backBuffer, amigaScreenBuffer);
  backBuffer->Release();

  HRESULT presentResult = _swapChain->Present(0, 0);

  if (presentResult != S_OK)
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

  DeleteDXGIFactory();
  DeleteAmigaScreenTexture();
  DeleteSwapChain();
  DeleteImmediateContext();
  //DeleteD3D11Device(); This crashes, unsure why
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
