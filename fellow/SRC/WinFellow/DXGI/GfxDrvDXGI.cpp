#include <InitGuid.h>

#include "fellow/api/defs.h"
#include "GfxDrvDXGI.h"
#include "GfxDrvDXGIAdapterEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "GfxDrvCommon.h"
#include "FELLOW.H"
#include "gfxdrv_directdraw.h"
#include "VirtualHost/Core.h"

#include "VertexShader.h"
#include "PixelShader.h"

#ifdef RETRO_PLATFORM
#include "RetroPlatform.h"
#endif

using namespace DirectX;

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
  if (hDll) {
    FreeLibrary(hDll);
  }
  else
  {
    _core.Log->AddLog("gfxDrvDXGIValidateRequirements() ERROR: d3d11.dll could not be loaded.\n");
    _requirementsValidationResult = false;
    return false;
  }

  hDll = LoadLibrary("dxgi.dll");
  if (hDll) {
    FreeLibrary(hDll);
  }
  else
  {
    _core.Log->AddLog("gfxDrvDXGIValidateRequirements() ERROR: dxgi.dll could not be loaded.\n");
    _requirementsValidationResult = false;
    return false;
  }

  GfxDrvDXGI dxgi;
  bool adaptersFound = dxgi.CreateAdapterList();
  if (!adaptersFound)
  {
    _core.Log->AddLog("gfxDrv ERROR: Direct3D present but no adapters found, falling back to DirectDraw.\n");
    _requirementsValidationResult = false;
    return false;
  }

  _requirementsValidationResult = true;
  return true;
}

bool GfxDrvDXGI::Startup()
{
  _core.Log->AddLog("GfxDrvDXGI: Starting up DXGI driver\n");

  bool success = CreateAdapterList();
  if (success)
  {
    RegisterModes();
  }
  _core.Log->AddLog("GfxDrvDXGI: Startup of DXGI driver %s\n", success ? "successful" : "failed");
  return success;
}

void GfxDrvDXGI::Shutdown()
{
  _core.Log->AddLog("GfxDrvDXGI: Starting to shut down DXGI driver\n");

  DeleteAdapterList();

  _core.Log->AddLog("GfxDrvDXGI: Finished shutdown of DXGI driver\n");
}

// Returns true if adapter enumeration was successful and the DXGI system actually has an adapter. (Like with VirtualBox in some hosts DXGI/D3D11 is present, but no adapters.)
bool GfxDrvDXGI::CreateAdapterList()
{
  DeleteAdapterList();

  IDXGIFactory* enumerationFactory;
  const HRESULT result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&enumerationFactory);
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

const char* GfxDrvDXGI::GetFeatureLevelString(D3D_FEATURE_LEVEL featureLevel)
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
  // VS2013 Update 5 on Windows 10 may require execution of the following
  // command to enable use of the debug layer; this installs the optional
  // "Graphics Tools" feature:
  // dism /online /add-capability /capabilityname:Tools.Graphics.DirectX~~~~0.0.1.0

  creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  hr = D3D11CreateDevice(nullptr,
    D3D_DRIVER_TYPE_HARDWARE,
    nullptr,
    creationFlags,
    nullptr,
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

  IDXGIDevice* dxgiDevice;
  hr = _d3d11device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

  if (FAILED(hr))
  {
    _core.Log->AddLog("Failed to query interface for IDXGIDevice\n");
    return false;
  }

  IDXGIAdapter* dxgiAdapter;
  hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

  if (FAILED(hr))
  {
    ReleaseCOM(&dxgiDevice);
    _core.Log->AddLog("Failed to get IDXGIAdapter via GetParent() on IDXGIDevice\n");
    return false;
  }

  _core.Log->AddLog("The adapter we got was:\n\n");
  GfxDrvDXGIAdapter adapter(dxgiAdapter); // Note: This will eventually release dxgiAdapter in COM. Maybe restructure the enum code later, the code structure ended up not being very practical.
  _core.Log->AddLog("Feature level is: %s\n", GetFeatureLevelString(featureLevelsSupported));

  hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&_dxgiFactory); // Used later to create the swap-chain

  if (FAILED(hr))
  {
    ReleaseCOM(&dxgiDevice);

    _core.Log->AddLog("Failed to get IDXGIFactory via GetParent() on IDXGIAdapter\n");
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
    ID3D11Debug* d3d11Debug;
    _d3d11device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3d11Debug));
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

bool GfxDrvDXGI::CreateAmigaScreenTexture()
{
  int width = draw_buffer_info.width;
  int height = draw_buffer_info.height;

  for (unsigned int i = 0; i < _amigaScreenTextureCount; i++)
  {
    D3D11_TEXTURE2D_DESC texture2DDesc;
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

    HRESULT hr = _d3d11device->CreateTexture2D(&texture2DDesc, nullptr, &_amigaScreenTexture[i]);

    if (FAILED(hr))
    {
      GfxDrvDXGIErrorLogger::LogError("Failed to create host screen texture.", hr);
      return false;
    }
  }

  unsigned int original_current_amiga_texture_index = _currentAmigaScreenTexture;
  for (unsigned int i = 0; i < _amigaScreenTextureCount; i++)
  {
    _currentAmigaScreenTexture = i;
    ClearCurrentBuffer();
  }
  _currentAmigaScreenTexture = original_current_amiga_texture_index;

  D3D11_TEXTURE2D_DESC texture2DDesc;
  texture2DDesc.Width = width;
  texture2DDesc.Height = height;
  texture2DDesc.MipLevels = 1;
  texture2DDesc.ArraySize = 1;
  texture2DDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  texture2DDesc.SampleDesc.Count = 1;
  texture2DDesc.SampleDesc.Quality = 0;
  texture2DDesc.Usage = D3D11_USAGE_DEFAULT;
  texture2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  texture2DDesc.CPUAccessFlags = 0;
  texture2DDesc.MiscFlags = 0;

  HRESULT hr = _d3d11device->CreateTexture2D(&texture2DDesc, nullptr, &_shaderInputTexture);

  if (FAILED(hr))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create shader input texture.", hr);
    return false;
  }


  return true;
}

void GfxDrvDXGI::DeleteAmigaScreenTexture()
{
  for (unsigned int i = 0; i < _amigaScreenTextureCount; i++)
  {
    ReleaseCOM(&_amigaScreenTexture[i]);
  }
  ReleaseCOM(&_shaderInputTexture);
}

ID3D11Texture2D* GfxDrvDXGI::GetCurrentAmigaScreenTexture()
{
  return _amigaScreenTexture[_currentAmigaScreenTexture];
}

void GfxDrvDXGI::GetBufferInformation(draw_buffer_information* buffer_information)
{
  uint32_t internal_scale_factor = drawGetInternalScaleFactor();
  buffer_information->width = drawGetInternalClip().GetWidth() * internal_scale_factor;
  buffer_information->height = drawGetInternalClip().GetHeight() * internal_scale_factor;
  buffer_information->pitch = 0;  // Replaced later by actual value
  buffer_information->redpos = 16;
  buffer_information->redsize = 8;
  buffer_information->greenpos = 8;
  buffer_information->greensize = 8;
  buffer_information->bluepos = 0;
  buffer_information->bluesize = 8;
  buffer_information->bits = 32;
}

bool GfxDrvDXGI::CreateSwapChain()
{
  int width = _current_draw_mode->width;
  int height = _current_draw_mode->height;

  _resize_swapchain_buffers = false;

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

  SetViewport();

  return true;
}

void GfxDrvDXGI::DeleteSwapChain()
{
  if (!gfxDrvCommon->GetOutputWindowed())
  {
    _swapChain->SetFullscreenState(FALSE, nullptr);
  }

  ReleaseCOM(&_swapChain);
}

void GfxDrvDXGI::SetViewport()
{
  D3D11_VIEWPORT viewPort;
  viewPort.TopLeftX = 0;
  viewPort.TopLeftY = 0;
  viewPort.Width = static_cast<FLOAT>(gfxDrvCommon->GetOutputWidth());
  viewPort.Height = static_cast<FLOAT>(gfxDrvCommon->GetOutputHeight());
  viewPort.MinDepth = 0.0f;
  viewPort.MaxDepth = 1.0f;

  _immediateContext->RSSetViewports(1, &viewPort);
}

bool GfxDrvDXGI::InitiateSwitchToFullScreen()
{
  _core.Log->AddLog("GfxDrvDXGI::InitiateSwitchToFullScreen()\n");

  DXGI_MODE_DESC* modeDescription = GetDXGIMode(_current_draw_mode->id);
  if (modeDescription == nullptr)
  {
    _core.Log->AddLog("Selected fullscreen mode was not found.\n");
    return false;
  }
  HRESULT result = _swapChain->SetFullscreenState(TRUE, nullptr); // TODO: Set which screen to use.
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to set full-screen.", result);
    return false;
  }

  _swapChain->ResizeTarget(modeDescription);
  return true;
}

DXGI_MODE_DESC* GfxDrvDXGI::GetDXGIMode(unsigned int id)
{
  if (_adapters->empty())
  {
    return nullptr;
  }
  GfxDrvDXGIAdapter* firstAdapter = _adapters->front();
  if (firstAdapter->GetOutputs().empty())
  {
    return nullptr;
  }

  GfxDrvDXGIOutput* firstOutput = firstAdapter->GetOutputs().front();
  for (GfxDrvDXGIMode* mode : firstOutput->GetModes())
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
  _core.Log->AddLog("GfxDrvDXGI::NotifyActiveStatus(%s)\n", active ? "TRUE" : "FALSE");
  if (!gfxDrvCommon->GetOutputWindowed() && _swapChain != nullptr)
  {
    _swapChain->SetFullscreenState(active, nullptr);
    if (!active) gfxDrvCommon->HideWindow();
  }
}

void GfxDrvDXGI::SizeChanged(unsigned int width, unsigned int height)
{
  // Don't execute the resize here, do it in the thread that renders
  _resize_swapchain_buffers = true;
}

void GfxDrvDXGI::PositionChanged()
{
}

void GfxDrvDXGI::ResizeSwapChainBuffers()
{
  _core.Log->AddLog("GfxDrvDXGI: ResizeSwapChainBuffers()\n");

  _resize_swapchain_buffers = false;

  HRESULT result = _swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to resize buffers of swap chain in response to WM_SIZE:", result);
  }

  // Output size could have been changed
  SetViewport();
  if (!CreateVertexAndIndexBuffers())
  {
    _core.Log->AddLog("GfxDrvDXGI::ResizeSwapChainBuffers() - Failed to re-create vertex and index buffers\n");
  }
}

unsigned char* GfxDrvDXGI::ValidateBufferPointer()
{
  if (_resize_swapchain_buffers)
  {
    ResizeSwapChainBuffers();
  }

  ID3D11Texture2D* hostBuffer = GetCurrentAmigaScreenTexture();
  D3D11_MAPPED_SUBRESOURCE mappedRect;
  D3D11_MAP mapFlag = D3D11_MAP_WRITE;

  HRESULT mapResult = _immediateContext->Map(hostBuffer, 0, mapFlag, 0, &mappedRect);

  if (FAILED(mapResult))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to map amiga screen texture:", mapResult);
    return nullptr;
  }

  draw_buffer_info.pitch = mappedRect.RowPitch;
  return (unsigned char*)mappedRect.pData;
}

void GfxDrvDXGI::InvalidateBufferPointer()
{
  ID3D11Texture2D* hostBuffer = GetCurrentAmigaScreenTexture();
  if (hostBuffer != nullptr)
  {
    _immediateContext->Unmap(hostBuffer, 0);
  }
}

bool GfxDrvDXGI::CreateVertexShader()
{
  HRESULT result = _d3d11device->CreateVertexShader(vertex_shader, sizeof(vertex_shader), nullptr, &_vertexShader);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create vertex shader.", result);
    return false;
  }

  D3D11_INPUT_ELEMENT_DESC polygonLayout[2];

  // Create the vertex input layout description.
  // This setup needs to match the VertexType stucture in the shader.
  polygonLayout[0].SemanticName = "POSITION";
  polygonLayout[0].SemanticIndex = 0;
  polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
  polygonLayout[0].InputSlot = 0;
  polygonLayout[0].AlignedByteOffset = 0;
  polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
  polygonLayout[0].InstanceDataStepRate = 0;

  polygonLayout[1].SemanticName = "TEXCOORD";
  polygonLayout[1].SemanticIndex = 0;
  polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
  polygonLayout[1].InputSlot = 0;
  polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
  polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
  polygonLayout[1].InstanceDataStepRate = 0;

  UINT numElements = 2;

  result = _d3d11device->CreateInputLayout(polygonLayout, numElements, vertex_shader, sizeof(vertex_shader), &_polygonLayout);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create polygon layout.", result);
    return false;
  }

  return true;
}

void GfxDrvDXGI::DeleteVertexShader()
{
  ReleaseCOM(&_polygonLayout);
  ReleaseCOM(&_vertexShader);
}

struct MatrixBufferType
{
  XMMATRIX world;
  XMMATRIX view;
  XMMATRIX projection;
};

bool GfxDrvDXGI::CreatePixelShader()
{
  HRESULT result = _d3d11device->CreatePixelShader(pixel_shader, sizeof(pixel_shader), nullptr, &_pixelShader);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create pixel shader.", result);
    return false;
  }

  // Create a texture sampler state description.
  D3D11_SAMPLER_DESC samplerDesc;
  //  samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  samplerDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
  samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.MipLODBias = 0.0f;
  samplerDesc.MaxAnisotropy = 1;
  samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
  samplerDesc.BorderColor[0] = 0;
  samplerDesc.BorderColor[1] = 0;
  samplerDesc.BorderColor[2] = 0;
  samplerDesc.BorderColor[3] = 0;
  samplerDesc.MinLOD = 0;
  samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

  // Create the texture sampler state.
  result = _d3d11device->CreateSamplerState(&samplerDesc, &_samplerState);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create sampler state.", result);
    return false;
  }

  // Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
  D3D11_BUFFER_DESC matrixBufferDesc;
  matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
  matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  matrixBufferDesc.MiscFlags = 0;
  matrixBufferDesc.StructureByteStride = 0;

  // Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
  result = _d3d11device->CreateBuffer(&matrixBufferDesc, nullptr, &_matrixBuffer);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create matrix buffer.", result);
    return false;
  }
  return true;
}

void GfxDrvDXGI::DeletePixelShader()
{
  ReleaseCOM(&_matrixBuffer);
  ReleaseCOM(&_samplerState);
  ReleaseCOM(&_pixelShader);
}

struct VertexType
{
  XMFLOAT3 position;
  XMFLOAT2 texture;
};

void GfxDrvDXGI::CalculateDestinationRectangle(uint32_t output_width, uint32_t output_height, float& dstHalfWidth, float& dstHalfHeight)
{
  float srcClipWidth = drawGetBufferClipWidthAsFloat();
  float srcClipHeight = drawGetBufferClipHeightAsFloat();

  if (drawGetDisplayScale() != DISPLAYSCALE::DISPLAYSCALE_AUTO)
  {
    float internalScaleFactor = static_cast<float>(drawGetInternalScaleFactor());
    float outputScaleFactor = static_cast<float>(drawGetOutputScaleFactor());
    float scaleRatio = outputScaleFactor / internalScaleFactor;

    dstHalfWidth = srcClipWidth * scaleRatio * 0.5f;
    dstHalfHeight = srcClipHeight * scaleRatio * 0.5f;
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
      dstHalfWidth = 0.5f * srcClipWidth * dstHeight / srcClipHeight;
      dstHalfHeight = dstHeight * 0.5f;
    }
    else
    {
      // Stretch to full width, black horisontal borders
      dstHalfWidth = dstWidth * 0.5f;
      dstHalfHeight = 0.5f * srcClipHeight * dstWidth / srcClipWidth;
    }
  }
}

void GfxDrvDXGI::CalculateSourceRectangle(float& srcLeft, float& srcTop, float& srcRight, float& srcBottom)
{
  // Source clip rectangle in 0.0 to 1.0 coordinates...
  float baseWidth = static_cast<float>(draw_buffer_info.width);
  float baseHeight = static_cast<float>(draw_buffer_info.height);

  srcLeft = drawGetBufferClipLeftAsFloat();
  srcTop = drawGetBufferClipTopAsFloat();
  srcRight = srcLeft + drawGetBufferClipWidthAsFloat();
  srcBottom = srcTop + drawGetBufferClipHeightAsFloat();

  srcLeft /= baseWidth;
  srcRight /= baseWidth;
  srcTop /= baseHeight;
  srcBottom /= baseHeight;
}

bool GfxDrvDXGI::CreateVertexAndIndexBuffers()
{
  DeleteVertexAndIndexBuffers();
  VertexType vertices[6];
  unsigned long indices[6];
  D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
  D3D11_SUBRESOURCE_DATA vertexData, indexData;

  for (int i = 0; i < 6; i++)
  {
    indices[i] = i;
  }

  float dstHalfWidth, dstHalfHeight;
  float srcLeft, srcTop, srcRight, srcBottom;

  CalculateDestinationRectangle(gfxDrvCommon->GetOutputWidth(), gfxDrvCommon->GetOutputHeight(), dstHalfWidth, dstHalfHeight);
  CalculateSourceRectangle(srcLeft, srcTop, srcRight, srcBottom);

  // First triangle.
  vertices[0].position = XMFLOAT3(-dstHalfWidth, dstHalfHeight, 0.0f);  // Top left.
  vertices[0].texture = XMFLOAT2(srcLeft, srcTop);

  vertices[1].position = XMFLOAT3(dstHalfWidth, -dstHalfHeight, 0.0f);  // Bottom right.
  vertices[1].texture = XMFLOAT2(srcRight, srcBottom);

  vertices[2].position = XMFLOAT3(-dstHalfWidth, -dstHalfHeight, 0.0f);  // Bottom left.
  vertices[2].texture = XMFLOAT2(srcLeft, srcBottom);

  // Second triangle.
  vertices[3].position = XMFLOAT3(-dstHalfWidth, dstHalfHeight, 0.0f);  // Top left.
  vertices[3].texture = XMFLOAT2(srcLeft, srcTop);

  vertices[4].position = XMFLOAT3(dstHalfWidth, dstHalfHeight, 0.0f);  // Top right.
  vertices[4].texture = XMFLOAT2(srcRight, srcTop);

  vertices[5].position = XMFLOAT3(dstHalfWidth, -dstHalfHeight, 0.0f);  // Bottom right.
  vertices[5].texture = XMFLOAT2(srcRight, srcBottom);

  // Set up the description of the static vertex buffer.
  vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  vertexBufferDesc.ByteWidth = sizeof(vertices);
  vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  vertexBufferDesc.MiscFlags = 0;
  vertexBufferDesc.StructureByteStride = 0;

  // Give the subresource structure a pointer to the vertex data.
  vertexData.pSysMem = vertices;
  vertexData.SysMemPitch = 0;
  vertexData.SysMemSlicePitch = 0;

  // Now create the vertex buffer.
  HRESULT result = _d3d11device->CreateBuffer(&vertexBufferDesc, &vertexData, &_vertexBuffer);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create vertex buffer.", result);
    return false;
  }

  // Index buffer

  indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
  indexBufferDesc.ByteWidth = sizeof(indices);
  indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  indexBufferDesc.CPUAccessFlags = 0;
  indexBufferDesc.MiscFlags = 0;
  indexBufferDesc.StructureByteStride = 0;

  indexData.pSysMem = indices;
  indexData.SysMemPitch = 0;
  indexData.SysMemSlicePitch = 0;

  result = _d3d11device->CreateBuffer(&indexBufferDesc, &indexData, &_indexBuffer);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create index buffer.", result);
    ReleaseCOM(&_vertexBuffer);
    return false;
  }
  return true;

}

void GfxDrvDXGI::DeleteVertexAndIndexBuffers()
{
  ReleaseCOM(&_vertexBuffer);
  ReleaseCOM(&_indexBuffer);
}

bool GfxDrvDXGI::CreateDepthDisabledStencil()
{
  D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDescription = {};

  depthDisabledStencilDescription.DepthEnable = false;
  depthDisabledStencilDescription.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depthDisabledStencilDescription.DepthFunc = D3D11_COMPARISON_LESS;
  depthDisabledStencilDescription.StencilEnable = true;
  depthDisabledStencilDescription.StencilReadMask = 0xFF;
  depthDisabledStencilDescription.StencilWriteMask = 0xFF;
  depthDisabledStencilDescription.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthDisabledStencilDescription.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
  depthDisabledStencilDescription.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthDisabledStencilDescription.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
  depthDisabledStencilDescription.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthDisabledStencilDescription.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
  depthDisabledStencilDescription.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthDisabledStencilDescription.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  HRESULT result = _d3d11device->CreateDepthStencilState(&depthDisabledStencilDescription, &_depthDisabledStencil);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create depth disabled stencil.", result);
    return false;
  }
  return true;
}

void GfxDrvDXGI::DeleteDepthDisabledStencil()
{
  ReleaseCOM(&_depthDisabledStencil);
}

bool GfxDrvDXGI::SetShaderParameters(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
  D3D11_MAPPED_SUBRESOURCE mappedResource;

  XMMATRIX worldTransposed = XMMatrixTranspose(worldMatrix);
  XMMATRIX viewTransposed = XMMatrixTranspose(viewMatrix);
  XMMATRIX projectionTransposed = XMMatrixTranspose(projectionMatrix);

  // Lock the constant buffer so it can be written to.
  HRESULT result = _immediateContext->Map(_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to map matrix buffer.", result);
    return false;
  }

  // Get a pointer to the data in the constant buffer.
  MatrixBufferType* dataPtr = (MatrixBufferType*)mappedResource.pData;

  // Copy the matrices into the constant buffer.
  dataPtr->world = worldMatrix;
  dataPtr->view = viewMatrix;
  dataPtr->projection = projectionMatrix;

  // Unlock the constant buffer.
  _immediateContext->Unmap(_matrixBuffer, 0);

  // Set the position of the constant buffer in the vertex shader.
  unsigned int bufferNumber = 0;

  // Now set the constant buffer in the vertex shader with the updated values.
  _immediateContext->VSSetConstantBuffers(bufferNumber, 1, &_matrixBuffer);

  // Set shader texture resource in the pixel shader.
  ID3D11ShaderResourceView* shaderResourceView;
  result = _d3d11device->CreateShaderResourceView(_shaderInputTexture, nullptr, &shaderResourceView);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to create shader resource view.", result);
    return false;
  }
  _immediateContext->PSSetShaderResources(0, 1, &shaderResourceView);
  ReleaseCOM(&shaderResourceView);

  // Set the vertex input layout.
  _immediateContext->IASetInputLayout(_polygonLayout);

  // Set the vertex and pixel shaders that will be used to render this triangle.
  _immediateContext->VSSetShader(_vertexShader, nullptr, 0);
  _immediateContext->PSSetShader(_pixelShader, nullptr, 0);

  // Set the sampler state in the pixel shader.
  _immediateContext->PSSetSamplers(0, 1, &_samplerState);

  // Render the triangle.
  UINT indexCount = 6;
  _immediateContext->DrawIndexed(indexCount, 0, 0);

  return true;
}

bool GfxDrvDXGI::RenderAmigaScreenToBackBuffer()
{
  float width = static_cast<float>(gfxDrvCommon->GetOutputWidth());
  float height = static_cast<float>(gfxDrvCommon->GetOutputHeight());

  XMMATRIX orthogonalMatrix = XMMatrixOrthographicLH(width, height, 1000.0f, 0.1f);
  XMMATRIX identityMatrix = XMMatrixIdentity();

  // Switch to 2D
  _immediateContext->OMSetDepthStencilState(_depthDisabledStencil, 1);

  unsigned int stride = sizeof(VertexType);
  unsigned int offset = 0;

  _immediateContext->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
  _immediateContext->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
  _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  bool shaderResult = SetShaderParameters(identityMatrix, identityMatrix, orthogonalMatrix);

  return shaderResult;
}

void GfxDrvDXGI::FlipTexture()
{
  ID3D11Texture2D* amigaScreenBuffer = GetCurrentAmigaScreenTexture();
  _immediateContext->CopyResource(_shaderInputTexture, amigaScreenBuffer);

  ID3D11Texture2D* backBuffer;
  HRESULT getBufferResult = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
  if (FAILED(getBufferResult))
  {
    GfxDrvDXGIErrorLogger::LogError("Failed to get back buffer.", getBufferResult);
    return;
  }

  ID3D11RenderTargetView* renderTargetView;
  HRESULT createRenderTargetViewResult = _d3d11device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);

  if (FAILED(createRenderTargetViewResult))
  {
    ReleaseCOM(&backBuffer);
    GfxDrvDXGIErrorLogger::LogError("Failed to create render target view.", createRenderTargetViewResult);
    return;
  }

  _immediateContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

  ReleaseCOM(&renderTargetView);

  RenderAmigaScreenToBackBuffer();

  ReleaseCOM(&backBuffer);

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

void GfxDrvDXGI::SetMode(draw_mode* dm, bool windowed)
{
  _current_draw_mode = dm;
}

void GfxDrvDXGI::RegisterMode(unsigned int id, unsigned int width, unsigned int height, unsigned int refreshRate)
{
  draw_mode* mode = new draw_mode();

  if (mode != nullptr)
  {
    mode->width = width;
    mode->height = height;
    mode->bits = 32;
    mode->refresh = refreshRate;
    mode->id = id;
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
    drawAddMode(mode);
  }
}

void GfxDrvDXGI::AddFullScreenModes()
{
  if (_adapters->empty())
  {
    return;
  }
  GfxDrvDXGIAdapter* firstAdapter = _adapters->front();
  if (firstAdapter->GetOutputs().empty())
  {
    return;
  }

  GfxDrvDXGIOutput* firstOutput = firstAdapter->GetOutputs().front();
  for (GfxDrvDXGIMode* mode : firstOutput->GetModes())
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
  uint8_t* buffer = ValidateBufferPointer();

  if (buffer != nullptr)
  {
    for (unsigned int y = 0; y < draw_buffer_info.height; y++)
    {
      uint32_t* line_ptr = (uint32_t*)buffer;
      for (unsigned int x = 0; x < draw_buffer_info.width; x++)
      {
        *line_ptr++ = 0;
      }
      buffer += draw_buffer_info.pitch;
    }
    InvalidateBufferPointer();
  }
}

bool GfxDrvDXGI::EmulationStart(unsigned int maxbuffercount)
{
  if (!CreateD3D11Device())
  {
    _core.Log->AddLog("GfxDrvDXGI::EmulationStart() - Failed to create d3d11 device for host window\n");
    return false;
  }

  if (!CreateSwapChain())
  {
    _core.Log->AddLog("GfxDrvDXGI::EmulationStart() - Failed to create swap chain for host window\n");
    return false;
  }

  if (!CreateAmigaScreenTexture())
  {
    _core.Log->AddLog("GfxDrvDXGI::EmulationStart() - Failed to create amiga screen texture\n");
    return false;
  }

  if (!CreatePixelShader())
  {
    _core.Log->AddLog("GfxDrvDXGI::EmulationStart() - Failed to create pixel shader\n");
    return false;
  }

  if (!CreateVertexShader())
  {
    _core.Log->AddLog("GfxDrvDXGI::EmulationStart() - Failed to create vertex shader\n");
    return false;
  }

  if (!CreateDepthDisabledStencil())
  {
    _core.Log->AddLog("GfxDrvDXGI::EmulationStart() - Failed to create depth disabled stencil\n");
    return false;
  }
  return true;
}

unsigned int GfxDrvDXGI::EmulationStartPost()
{
  if (!CreateVertexAndIndexBuffers())
  {
    _core.Log->AddLog("GfxDrvDXGI::EmulationStart() - Failed to create vertex and index buffers\n");
    return false;
  }

  if (!gfxDrvCommon->GetOutputWindowed())
  {
    bool fullscreenOk = InitiateSwitchToFullScreen();

    if (!fullscreenOk)
    {
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

  DeleteDepthDisabledStencil();
  DeleteVertexAndIndexBuffers();
  DeleteVertexShader();
  DeletePixelShader();
  DeleteAmigaScreenTexture();
  DeleteSwapChain();
  DeleteDXGIFactory();
  DeleteImmediateContext();
  DeleteD3D11Device();
}

GfxDrvDXGI::GfxDrvDXGI() :
  _adapters(nullptr),
  _d3d11device(nullptr),
  _immediateContext(nullptr),
  _dxgiFactory(nullptr),
  _swapChain(nullptr),
  _vertexShader(nullptr),
  _pixelShader(nullptr),
  _vertexBuffer(nullptr),
  _polygonLayout(nullptr),
  _indexBuffer(nullptr),
  _matrixBuffer(nullptr),
  _shaderInputTexture(nullptr),
  _depthDisabledStencil(nullptr),
  _samplerState(nullptr),
  _amigaScreenTextureCount(AmigaScreenTextureCount),
  _currentAmigaScreenTexture(0),
  _resize_swapchain_buffers(false)
{
  for (unsigned int i = 0; i < _amigaScreenTextureCount; i++)
  {
    _amigaScreenTexture[i] = nullptr;
  }
}

GfxDrvDXGI::~GfxDrvDXGI()
{
  Shutdown();
}

bool GfxDrvDXGI::SaveScreenshot(const bool bSaveFilteredScreenshot, const char* filename)
{
  bool bResult = false;
  DWORD width = 0, height = 0, x = 0, y = 0;
  uint32_t lDisplayScale;
  IDXGISurface1* pSurface1 = nullptr;
  HDC hDC = nullptr;

#ifdef _DEBUG
  _core.Log->AddLog("GfxDrvDXGI::SaveScreenshot(filtered=%s, filename=%s)\n",
    bSaveFilteredScreenshot ? "true" : "false", filename);
#endif

  if (bSaveFilteredScreenshot)
  {
    HRESULT hr = _swapChain->GetBuffer(0, __uuidof(IDXGISurface1), (void**)&pSurface1);
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
    if (RP.GetHeadlessMode())
    {
      width = RP.GetScreenWidthAdjusted();
      height = RP.GetScreenHeightAdjusted();
      lDisplayScale = RP.GetDisplayScale();
    }
    else
#endif
    {
      width = _current_draw_mode->width;
      height = _current_draw_mode->height;
      lDisplayScale = 1;
    }

    bResult = gfxDrvDDrawSaveScreenshotFromDCArea(hDC, x, y, width, height, 1, 32, filename);
  }
  else
  {  // save unfiltered screenshot
    // width  = _current_draw_mode->width;
    // height = _current_draw_mode->height;
    width = draw_buffer_info.width;
    height = draw_buffer_info.height;
    ID3D11Texture2D* hostBuffer = GetCurrentAmigaScreenTexture();
    ID3D11Texture2D* screenshotTexture = nullptr;
    D3D11_TEXTURE2D_DESC texture2DDesc;
    texture2DDesc.Width = width;
    texture2DDesc.Height = height;
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

    hr = screenshotTexture->QueryInterface(__uuidof(IDXGISurface1), (void**)&pSurface1);
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

    ReleaseCOM(&screenshotTexture);
  }

  _core.Log->AddLog("GfxDrvDXGI::SaveScreenshot(filtered=%s, filename='%s') %s.\n",
    bSaveFilteredScreenshot ? "true" : "false", filename, bResult ? "successful" : "failed");

  pSurface1->ReleaseDC(nullptr);

  ReleaseCOM(&pSurface1);

  return bResult;
}
