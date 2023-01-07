#pragma once

#include <D3D11.h>
#include <map>

#include "fellow/application/DrawDimensionTypes.h"

struct GfxDrvDXGIMapResult
{
  unsigned char *Buffer;
  ptrdiff_t Pitch;
};

class GfxDrvDXGIAmigaScreenTexture
{
private:
  ID3D11Device *_d3d11device{};
  ID3D11DeviceContext *_immediateContext{};
  DimensionsUInt _size;

  ID3D11Texture2D *_amigaScreenTexture{}; // CPU write buffer
  ID3D11Texture2D *_shaderInputTexture{}; // On-card shader input buffer
  ID3D11Texture2D *_overlayTexture{};     // Overlay for drive leds, frame counter etc.

  bool CreateAmigaScreen();
  void DeleteAmigaScreen();

  bool CreateShaderInput();
  void DeleteShaderInput();

  bool CreateOverlay();
  void DeleteOverlay();

public:
  bool Configure(ID3D11Device *d3d11device, ID3D11DeviceContext *immediateContext, const DimensionsUInt &chipsetBufferSize);
  void Uninitialize();

  GfxDrvDXGIMapResult MapAmigaScreen();
  void UnmapAmigaScreen();
  bool ClearAmigaScreen();

  void CopyAmigaScreenToShaderInput();
  bool SetRenderTargetToBackbuffer(ID3D11Texture2D *backbuffer);
  void RenderToBackbuffer();

  ID3D11Texture2D *GetAmigaScreenD3D11Texture();
  ID3D11Texture2D *GetShaderInputTexture();
  ID3D11Texture2D *GetOverlayTexture();

  const DimensionsUInt &GetSize() const;

  GfxDrvDXGIAmigaScreenTexture() = default;
  ~GfxDrvDXGIAmigaScreenTexture() = default;
};
