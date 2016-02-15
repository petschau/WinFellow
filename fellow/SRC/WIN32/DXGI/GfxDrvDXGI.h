#ifndef GfxDrvDXGI_H
#define GfxDrvDXGI_H

#include <DXGI.h>
#include <D3D11.h>
#include <DirectXMath.h>

#include "DEFS.H"
#include "GfxDrvDXGIAdapter.h"
#include "DRAW.H"

using namespace DirectX;

const static unsigned int AmigaScreenTextureCount = 1;
const static unsigned int BackBufferCount = 2;

template<typename T> void ReleaseCOM(T **comobject)
{
  if (*comobject != nullptr)
  {
    (*comobject)->Release();
    *comobject = nullptr;
  }
}

class GfxDrvDXGI
{
private:
  // Information
  GfxDrvDXGIAdapterList* _adapters;
  IDXGIFactory *_enumerationFactory;

  // Current session
  ID3D11Device *_d3d11device;
  ID3D11DeviceContext *_immediateContext;
  IDXGIFactory *_dxgiFactory;
  IDXGISwapChain *_swapChain;
  ID3D11VertexShader *_vertexShader;
  ID3D11PixelShader *_pixelShader;
  ID3D11Buffer *_vertexBuffer;
  ID3D11InputLayout* _polygonLayout;
  ID3D11Buffer *_indexBuffer;
  ID3D11Buffer* _matrixBuffer;
  ID3D11Texture2D *_shaderInputTexture;
  ID3D11Texture2D *_amigaScreenTexture[AmigaScreenTextureCount];
  ID3D11DepthStencilState* _depthDisabledStencil;
  ID3D11SamplerState* _samplerState;

  unsigned int _amigaScreenTextureCount;
  unsigned int _currentAmigaScreenTexture;
  draw_mode *_current_draw_mode;

  bool _resize_swapchain_buffers;

private:
  void CreateAdapterList();
  void DeleteAdapterList();

  bool CreateEnumerationFactory();
  void DeleteEnumerationFactory();

  void RegisterMode(unsigned int id, unsigned int width, unsigned int height, unsigned int refreshRate = 60, bool isWindowed = true);
  void RegisterModes();
  void AddFullScreenModes();
  void AddWindowModes();

  bool CreateD3D11Device();
  void DeleteD3D11Device();
  
  void DeleteDXGIFactory();
  void DeleteImmediateContext();

  bool CreateSwapChain();
  void DeleteSwapChain();
  bool InitiateSwitchToFullScreen();
  void ResizeSwapChainBuffers();
  DXGI_MODE_DESC* GetDXGIMode(unsigned int id);

  bool CreateAmigaScreenTexture();
  void DeleteAmigaScreenTexture();
  ID3D11Texture2D *GetCurrentAmigaScreenTexture();

  bool RenderAmigaScreenToBackBuffer();

  void FlipTexture();

  STR* GetFeatureLevelString(D3D_FEATURE_LEVEL featureLevel);

  bool CreatePixelShader();
  void DeletePixelShader();

  bool CreateVertexShader();
  void DeleteVertexShader();

  bool CreateVertexAndIndexBuffers();
  void DeleteVertexAndIndexBuffers();

  bool CreateDepthDisabledStencil();
  void DeleteDepthDisabledStencil();

  bool SetShaderParameters(const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

  void CalculateDestinationRectangle(float& dstHalfWidth, float& dstHalfHeight);
  void CalculateSourceRectangle(float& srcLeft, float& srcTop, float& srcRight, float& srcBottom);

public:

  void ClearCurrentBuffer();
  void SetMode(draw_mode *dm);
  void SizeChanged();
  void NotifyActiveStatus(bool active);

  bool EmulationStart(unsigned int maxbuffercount);
  unsigned int EmulationStartPost();
  void EmulationStop();

  bool Startup();
  void Shutdown();

  unsigned char *ValidateBufferPointer();
  void InvalidateBufferPointer();
  void GetBufferInformation(draw_mode *mode, draw_buffer_information *buffer_information);
  void Flip();

  void RegisterRetroPlatformScreenMode(const bool, const ULO, const ULO, const ULO);
  bool SaveScreenshot(const bool, const STR *);

  GfxDrvDXGI();
  virtual ~GfxDrvDXGI();
};

#endif
