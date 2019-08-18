#include "GfxDrvDXGIAmigaScreenTexture.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "GfxDrvDXGIUtils.h"

GfxDrvDXGIMapResult GfxDrvDXGIAmigaScreenTexture::MapAmigaScreen()
{
  D3D11_MAPPED_SUBRESOURCE mappedRect{};
  UINT subResource = 0;
  D3D11_MAP mapType = D3D11_MAP_WRITE;
  UINT mapFlags = 0;

  HRESULT mapResult = _immediateContext->Map(_amigaScreenTexture, subResource, mapType, mapFlags, &mappedRect);
  if (FAILED(mapResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenTexture::MapAmigaScreen() - Failed to map amiga screen texture:", mapResult);
    return GfxDrvDXGIMapResult{.Buffer = nullptr, .Pitch = 0};
  }

  return GfxDrvDXGIMapResult{.Buffer = (unsigned char *)mappedRect.pData, .Pitch = mappedRect.RowPitch};
}

void GfxDrvDXGIAmigaScreenTexture::UnmapAmigaScreen()
{
  if (_amigaScreenTexture != nullptr)
  {
    _immediateContext->Unmap(_amigaScreenTexture, 0);
  }
}

bool GfxDrvDXGIAmigaScreenTexture::CreateAmigaScreen()
{
  D3D11_TEXTURE2D_DESC amigaScreenDesc{};
  amigaScreenDesc.Width = _size.Width;
  amigaScreenDesc.Height = _size.Height;
  amigaScreenDesc.MipLevels = 1;
  amigaScreenDesc.ArraySize = 1;
  amigaScreenDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  amigaScreenDesc.SampleDesc.Count = 1;
  amigaScreenDesc.SampleDesc.Quality = 0;
  amigaScreenDesc.Usage = D3D11_USAGE_STAGING;
  amigaScreenDesc.BindFlags = 0;
  amigaScreenDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  amigaScreenDesc.MiscFlags = 0;

  HRESULT amigaScreenTextureResult = _d3d11device->CreateTexture2D(&amigaScreenDesc, nullptr, &_amigaScreenTexture);
  if (FAILED(amigaScreenTextureResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenTexture::CreateAmigaScreen() - Failed to create amiga screen texture", amigaScreenTextureResult);
    return false;
  }

#ifdef _DEBUG
  GfxDrvDXGISetDebugName(_amigaScreenTexture, "Amiga screen texture");
#endif

  return true;
}

void GfxDrvDXGIAmigaScreenTexture::DeleteAmigaScreen()
{
  ReleaseCOM(&_amigaScreenTexture);
}

bool GfxDrvDXGIAmigaScreenTexture::ClearAmigaScreen()
{
  GfxDrvDXGIMapResult mapResult = MapAmigaScreen();
  if (mapResult.Buffer == nullptr)
  {
    return false;
  }

  uint8_t *bufferPtr = mapResult.Buffer;
  for (unsigned int y = 0; y < _size.Height; y++)
  {
    uint32_t *linePtr = (uint32_t *)bufferPtr;
    for (unsigned int x = 0; x < _size.Width; x++)
    {
      *linePtr++ = 0;
    }
    bufferPtr += mapResult.Pitch;
  }

  UnmapAmigaScreen();
  return true;
}

bool GfxDrvDXGIAmigaScreenTexture::CreateShaderInput()
{
  D3D11_TEXTURE2D_DESC shaderInputDesc{};
  shaderInputDesc.Width = _size.Width;
  shaderInputDesc.Height = _size.Height;
  shaderInputDesc.MipLevels = 1;
  shaderInputDesc.ArraySize = 1;
  shaderInputDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  shaderInputDesc.SampleDesc.Count = 1;
  shaderInputDesc.SampleDesc.Quality = 0;
  shaderInputDesc.Usage = D3D11_USAGE_DEFAULT;
  shaderInputDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  shaderInputDesc.CPUAccessFlags = 0;
  shaderInputDesc.MiscFlags = 0;

  HRESULT shaderInputTextureResult = _d3d11device->CreateTexture2D(&shaderInputDesc, nullptr, &_shaderInputTexture);
  if (FAILED(shaderInputTextureResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenTexture::CreateShaderInput() - Failed to create shader input texture", shaderInputTextureResult);
    return false;
  }

  return true;
}

void GfxDrvDXGIAmigaScreenTexture::DeleteShaderInput()
{
  ReleaseCOM(&_shaderInputTexture);
}

bool GfxDrvDXGIAmigaScreenTexture::CreateOverlay()
{
  D3D11_TEXTURE2D_DESC overlayDesc{};
  overlayDesc.Width = _size.Width;
  overlayDesc.Height = _size.Height;
  overlayDesc.MipLevels = 1;
  overlayDesc.ArraySize = 1;
  overlayDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  overlayDesc.SampleDesc.Count = 1;
  overlayDesc.SampleDesc.Quality = 0;
  overlayDesc.Usage = D3D11_USAGE_DEFAULT;
  overlayDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  overlayDesc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

  HRESULT overlayTextureResult = _d3d11device->CreateTexture2D(&overlayDesc, 0, &_overlayTexture);
  if (FAILED(overlayTextureResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenTexture::CreateOverlay() - Failed to create overlay texture", overlayTextureResult);
    return false;
  }

  return true;
}

void GfxDrvDXGIAmigaScreenTexture::DeleteOverlay()
{
  ReleaseCOM(&_overlayTexture);
}

ID3D11Texture2D *GfxDrvDXGIAmigaScreenTexture::GetAmigaScreenD3D11Texture()
{
  return _amigaScreenTexture;
}

ID3D11Texture2D *GfxDrvDXGIAmigaScreenTexture::GetShaderInputTexture()
{
  return _shaderInputTexture;
}

ID3D11Texture2D *GfxDrvDXGIAmigaScreenTexture::GetOverlayTexture()
{
  return _overlayTexture;
}

const DimensionsUInt &GfxDrvDXGIAmigaScreenTexture::GetSize() const
{
  return _size;
}

bool GfxDrvDXGIAmigaScreenTexture::Configure(ID3D11Device *d3d11device, ID3D11DeviceContext *immediateContext, const DimensionsUInt &chipsetBufferSize)
{
  _d3d11device = d3d11device;
  _immediateContext = immediateContext;
  _size = chipsetBufferSize;

  bool success = CreateAmigaScreen();
  if (success) success = ClearAmigaScreen();
  if (success) success = CreateShaderInput();
  if (success) success = CreateOverlay();

  return success;
}

void GfxDrvDXGIAmigaScreenTexture::Uninitialize()
{
  DeleteOverlay();
  DeleteShaderInput();
  DeleteAmigaScreen();

  _immediateContext = nullptr;
  _d3d11device = nullptr;
  _size = DimensionsUInt();
}