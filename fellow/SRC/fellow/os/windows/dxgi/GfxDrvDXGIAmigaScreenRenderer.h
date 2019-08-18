#pragma once

#include <D3D11.h>
#include "GfxDrvDXGIAmigaScreenTexture.h"
#include "fellow/application/DrawDimensionTypes.h"

class GfxDrvDXGIAmigaScreenRenderer
{
private:
  bool _initialized{};

  // Global context
  ID3D11Device *_d3d11device{};
  ID3D11DeviceContext *_immediateContext{};
  DimensionsUInt _hostBufferSize{};
  DimensionsUInt _chipsetBufferSize{};
  RectPixels _chipsetOutputClipPixels{};

  // Static resources for the renderer
  ID3D11DepthStencilState *_depthDisabledStencil{};
  ID3D11Buffer *_vertexBuffer{};
  ID3D11Buffer *_indexBuffer{};
  ID3D11InputLayout *_polygonLayout{};
  ID3D11VertexShader *_vertexShader{};
  ID3D11PixelShader *_pixelShader{};
  ID3D11PixelShader *_overlayPixelShader{};
  ID3D11Buffer *_matrixBuffer{};
  ID3D11SamplerState *_samplerState{};
  ID3D11SamplerState *_overlaySamplerState{};

  bool CreateDepthDisabledStencil();
  void DeleteDepthDisabledStencil();

  bool CreateVertexAndIndexBuffers();
  void DeleteVertexAndIndexBuffers();

  bool CreateVertexShader();
  void DeleteVertexShader();

  bool CreatePixelShaders();
  void DeletePixelShaders();
  bool UpdateMatrixBuffer();

  RectFloat CalculateSourceRectangle();
  DimensionsFloat CalculateDestinationRectangle();

  void CopyAmigaScreenToShaderInput(GfxDrvDXGIAmigaScreenTexture &amigaScreen);
  bool SetRenderTarget(ID3D11Texture2D *backbuffer);
  void SetRenderGeometry();
  bool ExecuteShaders(GfxDrvDXGIAmigaScreenTexture &amigaScreen);

  bool IsSameConfiguration(const DimensionsUInt &hostBufferSize, const DimensionsUInt &chipsetBufferSize, const RectPixels &chipsetOutputClipPixels);

public:
  bool Render(GfxDrvDXGIAmigaScreenTexture &amigaScreen, ID3D11Texture2D *backbuffer);

  bool Configure(ID3D11Device *d3d11device, ID3D11DeviceContext *immediateContext, const DimensionsUInt &hostBufferSize, const DimensionsUInt &chipsetBufferSize, const RectPixels &chipsetOutputClip);
  void ReleaseAllocatedResources();
};
