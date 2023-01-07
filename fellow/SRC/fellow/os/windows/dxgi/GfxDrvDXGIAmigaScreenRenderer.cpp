#include "GfxDrvDXGIAmigaScreenRenderer.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "GfxDrvDXGIUtils.h"

#include "fellow/os/windows/dxgi/VertexShader.h"
#include "fellow/os/windows/dxgi/PixelShader.h"

#include <DirectXMath.h>
using namespace DirectX;

#include "fellow/api/defs.h"

constexpr static bool UseOverlay = false;

struct VertexType
{
  XMFLOAT3 position;
  XMFLOAT2 texture;
};

struct MatrixBufferType
{
  XMMATRIX world;
  XMMATRIX view;
  XMMATRIX projection;
};

bool GfxDrvDXGIAmigaScreenRenderer::CreateDepthDisabledStencil()
{
  D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDescription{};
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
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::CreateDepthDisabledStencil() - Failed to create depth disabled stencil", result);
    return false;
  }

  return true;
}

void GfxDrvDXGIAmigaScreenRenderer::DeleteDepthDisabledStencil()
{
  ReleaseCOM(&_depthDisabledStencil);
}

bool GfxDrvDXGIAmigaScreenRenderer::CreateVertexAndIndexBuffers()
{
  // outputSize might not be 1.0 in both directions in case of aspect ratio mismatch between chipset output clip and host buffer size
  DimensionsFloat outputSize = CalculateDestinationRectangle();
  float halfWidth = outputSize.Width * 0.5f;
  float halfHeight = outputSize.Height * 0.5f;

  RectFloat sourceRect = CalculateSourceRectangle();

  VertexType vertices[6];

  // First triangle.
  vertices[0].position = XMFLOAT3(-halfWidth, halfHeight, 0.0f); // Top left.
  vertices[0].texture = XMFLOAT2(sourceRect.Left, sourceRect.Top);

  vertices[1].position = XMFLOAT3(halfWidth, -halfHeight, 0.0f); // Bottom right.
  vertices[1].texture = XMFLOAT2(sourceRect.Right, sourceRect.Bottom);

  vertices[2].position = XMFLOAT3(-halfWidth, -halfHeight, 0.0f); // Bottom left.
  vertices[2].texture = XMFLOAT2(sourceRect.Left, sourceRect.Bottom);

  // Second triangle.
  vertices[3].position = XMFLOAT3(-halfWidth, halfHeight, 0.0f); // Top left.
  vertices[3].texture = XMFLOAT2(sourceRect.Left, sourceRect.Top);

  vertices[4].position = XMFLOAT3(halfWidth, halfHeight, 0.0f); // Top right.
  vertices[4].texture = XMFLOAT2(sourceRect.Right, sourceRect.Top);

  vertices[5].position = XMFLOAT3(halfWidth, -halfHeight, 0.0f); // Bottom right.
  vertices[5].texture = XMFLOAT2(sourceRect.Right, sourceRect.Bottom);

  // Set up the description of the static vertex buffer.
  D3D11_BUFFER_DESC vertexBufferDesc;
  vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  vertexBufferDesc.ByteWidth = sizeof(vertices);
  vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  vertexBufferDesc.MiscFlags = 0;
  vertexBufferDesc.StructureByteStride = 0;

  // Give the subresource structure a pointer to the vertex data.
  D3D11_SUBRESOURCE_DATA vertexData;
  vertexData.pSysMem = vertices;
  vertexData.SysMemPitch = 0;
  vertexData.SysMemSlicePitch = 0;

  // Create the vertex buffer.
  HRESULT vertexBufferResult = _d3d11device->CreateBuffer(&vertexBufferDesc, &vertexData, &_vertexBuffer);
  if (FAILED(vertexBufferResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::CreateVertexAndIndexBuffers() - Failed to create vertex buffer", vertexBufferResult);
    return false;
  }

  // Index buffer
  unsigned long indices[6] = {0, 1, 2, 3, 4, 5};
  D3D11_BUFFER_DESC indexBufferDesc;
  indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
  indexBufferDesc.ByteWidth = sizeof(indices);
  indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  indexBufferDesc.CPUAccessFlags = 0;
  indexBufferDesc.MiscFlags = 0;
  indexBufferDesc.StructureByteStride = 0;

  D3D11_SUBRESOURCE_DATA indexData;
  indexData.pSysMem = indices;
  indexData.SysMemPitch = 0;
  indexData.SysMemSlicePitch = 0;

  HRESULT indexBufferResult = _d3d11device->CreateBuffer(&indexBufferDesc, &indexData, &_indexBuffer);
  if (FAILED(indexBufferResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::CreateVertexAndIndexBuffers() - Failed to create index buffer", indexBufferResult);
    return false;
  }

  return true;
}

void GfxDrvDXGIAmigaScreenRenderer::DeleteVertexAndIndexBuffers()
{
  ReleaseCOM(&_vertexBuffer);
  ReleaseCOM(&_indexBuffer);
}

// Calculates the placement of the chipset buffer output clip area in the host buffer (ie. the actual window or fullscreen buffer)
// Returns the actual width and height used by the chipset buffer output clip area on the host screen
DimensionsFloat GfxDrvDXGIAmigaScreenRenderer::CalculateDestinationRectangle()
{
  const float coreBufferClippedWidth = (float)_chipsetOutputClipPixels.GetWidth();
  const float coreBufferClippedHeight = (float)_chipsetOutputClipPixels.GetHeight();

  float outputWidth{}, outputHeight{};

  // if (_displayscale != DISPLAYSCALE::DISPLAYSCALE_AUTO)
  //{
  //   // In non-automatic mode, assume the emulator has opened a window with a size that is an exact match for the scale and chipset display clip parameters,

  //  float coreBufferScaleFactor = (float)Draw.GetChipsetBufferScaleFactor(); // Can be 1X (hires sized) or 2X (shres sized). This relates to the chipset buffer
  //  float outputWindowScaleFactor = (float)Draw.GetOutputScaleFactor(); // Can be 1X, 2X, 3X or 4X, this relates to the host-window buffer
  //  float scaleRatio = outputWindowScaleFactor / coreBufferScaleFactor; // Actual scale difference between the chipset buffer and the host window. The GPU performs this scaling. For instance
  //  0.5, 1.0, 1.5, 2, 3, 4.

  //  outputWidth = coreBufferClippedWidth * scaleRatio;
  //  outputHeight = coreBufferClippedHeight * scaleRatio;

  //  // In manual scale mode, these should be identical. If they are not, we get black borders, or over-zoom
  //  F_ASSERT(outputWidth == (float)_hostBufferSize.Width);
  //  F_ASSERT(outputHeight == (float)_hostBufferSize.Height);
  //}
  // else
  //{
  // Automatic best fit in the output window
  // In non-automatic scaling mode, assume the emulator has opened a window with a size that is an exact match for the scale and chipset display clip parameters
  float windowWidthAsFloat = (float)_hostBufferSize.Width;
  float windowHeightAsFloat = (float)_hostBufferSize.Height;

  float coreBufferAspectRatio = coreBufferClippedWidth / coreBufferClippedHeight;
  float outputWindowAspectRatio = windowWidthAsFloat / windowHeightAsFloat;

  if (outputWindowAspectRatio > coreBufferAspectRatio)
  {
    // Stretch to full height, black vertical borders
    outputWidth = coreBufferClippedWidth * windowHeightAsFloat / coreBufferClippedHeight;
    outputHeight = windowHeightAsFloat;
  }
  else
  {
    // Stretch to full width, black horisontal borders
    outputWidth = windowWidthAsFloat;
    outputHeight = coreBufferClippedHeight * windowWidthAsFloat / coreBufferClippedWidth;
  }
  //  }

  return DimensionsFloat(outputWidth, outputHeight);
}

// Normalizes chipsetOutputClip to coordinates in 0.0 to 1.0 range
// TODO: This can be done once after Configure()
RectFloat GfxDrvDXGIAmigaScreenRenderer::CalculateSourceRectangle()
{
  const float baseWidth = (float)_chipsetBufferSize.Width;
  const float baseHeight = (float)_chipsetBufferSize.Height;

  return RectFloat(
      ((float)_chipsetOutputClipPixels.Left) / baseWidth,
      ((float)_chipsetOutputClipPixels.Top) / baseHeight,
      ((float)_chipsetOutputClipPixels.Right) / baseWidth,
      ((float)_chipsetOutputClipPixels.Bottom) / baseHeight);
}

bool GfxDrvDXGIAmigaScreenRenderer::CreateVertexShader()
{
  HRESULT vertexShaderResult = _d3d11device->CreateVertexShader(vertex_shader, sizeof(vertex_shader), nullptr, &_vertexShader);
  if (FAILED(vertexShaderResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::CreateVertexShader() - Failed to create vertex shader", vertexShaderResult);
    return false;
  }

  D3D11_INPUT_ELEMENT_DESC polygonLayout[2];

  // Create the vertex input layout description, must match the VertexType stucture in the shader
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

  HRESULT polygonLayoutResult = _d3d11device->CreateInputLayout(polygonLayout, numElements, vertex_shader, sizeof(vertex_shader), &_polygonLayout);
  if (FAILED(polygonLayoutResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::CreateVertexShader() - Failed to create polygon layout", polygonLayoutResult);
    return false;
  }

  return true;
}

void GfxDrvDXGIAmigaScreenRenderer::DeleteVertexShader()
{
  ReleaseCOM(&_polygonLayout);
  ReleaseCOM(&_vertexShader);
}

bool GfxDrvDXGIAmigaScreenRenderer::CreatePixelShaders()
{
  HRESULT pixelShaderResult = _d3d11device->CreatePixelShader(pixel_shader, sizeof(pixel_shader), nullptr, &_pixelShader);
  if (FAILED(pixelShaderResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::CreatePixelShaders() - Failed to create pixel shader", pixelShaderResult);
    return false;
  }

  // Create a texture sampler state description.
  D3D11_SAMPLER_DESC samplerDesc;
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
  HRESULT samplerStateResult = _d3d11device->CreateSamplerState(&samplerDesc, &_samplerState);
  if (FAILED(samplerStateResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::CreatePixelShaders() - Failed to create sampler state", samplerStateResult);
    return false;
  }

  // Create the overlay sampler state.
  samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  HRESULT overlaySamplerStateResult = _d3d11device->CreateSamplerState(&samplerDesc, &_overlaySamplerState);
  if (FAILED(overlaySamplerStateResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::CreatePixelShaders() - Failed to create overlay sampler state", overlaySamplerStateResult);
    return false;
  }

  // Setup the description of the matrix constant buffer that is in the vertex shader.
  D3D11_BUFFER_DESC matrixBufferDesc;
  matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
  matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
  matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  matrixBufferDesc.MiscFlags = 0;
  matrixBufferDesc.StructureByteStride = 0;

  // Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
  HRESULT matrixBufferResult = _d3d11device->CreateBuffer(&matrixBufferDesc, nullptr, &_matrixBuffer);
  if (FAILED(matrixBufferResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::CreatePixelShaders() - Failed to create matrix buffer", matrixBufferResult);
    return false;
  }

  UpdateMatrixBuffer();
  return true;
}

void GfxDrvDXGIAmigaScreenRenderer::DeletePixelShaders()
{
  ReleaseCOM(&_matrixBuffer);
  ReleaseCOM(&_samplerState);
  ReleaseCOM(&_overlaySamplerState);
  ReleaseCOM(&_pixelShader);
  ReleaseCOM(&_overlayPixelShader);
}

bool GfxDrvDXGIAmigaScreenRenderer::UpdateMatrixBuffer()
{
  const float width = (float)_chipsetBufferSize.Width;
  const float height = (float)_chipsetBufferSize.Height;

  XMMATRIX orthogonalMatrix = XMMatrixOrthographicLH(width, height, 1000.0f, 0.1f);
  XMMATRIX identityMatrix = XMMatrixIdentity();

  // XMMATRIX worldTransposed = XMMatrixTranspose(identityMatrix);
  // XMMATRIX viewTransposed = XMMatrixTranspose(identityMatrix);
  // XMMATRIX projectionTransposed = XMMatrixTranspose(orthogonalMatrix);

  // Lock the constant buffer so it can be written to.
  D3D11_MAPPED_SUBRESOURCE mappedResource{};
  HRESULT result = _immediateContext->Map(_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::UpdateMatrixBuffer() - Failed to map matrix buffer", result);
    return false;
  }

  // Get a pointer to the data in the constant buffer.
  MatrixBufferType *dataPtr = (MatrixBufferType *)mappedResource.pData;

  // Copy the matrices into the constant buffer.
  dataPtr->world = identityMatrix;
  dataPtr->view = identityMatrix;
  dataPtr->projection = orthogonalMatrix;

  // Unlock the constant buffer.
  _immediateContext->Unmap(_matrixBuffer, 0);

  return true;
}

void GfxDrvDXGIAmigaScreenRenderer::CopyAmigaScreenToShaderInput(GfxDrvDXGIAmigaScreenTexture &amigaScreen)
{
  _immediateContext->CopyResource(amigaScreen.GetShaderInputTexture(), amigaScreen.GetAmigaScreenD3D11Texture());
}

bool GfxDrvDXGIAmigaScreenRenderer::SetRenderTarget(ID3D11Texture2D *backbuffer)
{
  ID3D11RenderTargetView *renderTargetView{};

  HRESULT createRenderTargetViewResult = _d3d11device->CreateRenderTargetView(backbuffer, nullptr, &renderTargetView);
  if (FAILED(createRenderTargetViewResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenTexture::SetRenderTarget() - Failed to create render target view", createRenderTargetViewResult);
    return false;
  }

  _immediateContext->OMSetRenderTargets(1, &renderTargetView, nullptr);
  ReleaseCOM(&renderTargetView);
  return true;
}

void GfxDrvDXGIAmigaScreenRenderer::SetRenderGeometry()
{
  // Switch to 2D
  _immediateContext->OMSetDepthStencilState(_depthDisabledStencil, 1);

  // Set geometry for a rectangle as two triangles
  unsigned int stride = sizeof(VertexType);
  unsigned int offset = 0;

  _immediateContext->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
  _immediateContext->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
  _immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

bool GfxDrvDXGIAmigaScreenRenderer::ExecuteShaders(GfxDrvDXGIAmigaScreenTexture &amigaScreen)
{
  // Set the matrix buffer at slot 0 in the vertex shader
  UINT bufferNumber = 0;
  _immediateContext->VSSetConstantBuffers(bufferNumber, 1, &_matrixBuffer);

  // Add the shader input texture to the shader resource view
  ID3D11ShaderResourceView *shaderResourceView[2]{};
  ID3D11SamplerState *samplers[2]{};
  UINT numViews = 1;

  HRESULT result = _d3d11device->CreateShaderResourceView(amigaScreen.GetShaderInputTexture(), nullptr, &shaderResourceView[0]);
  if (FAILED(result))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::ExecuteShaders() - Failed to create shader resource view for the amiga screen shader input texture", result);
    return false;
  }

  samplers[0] = _samplerState;

  if (UseOverlay)
  {
    result = _d3d11device->CreateShaderResourceView(amigaScreen.GetOverlayTexture(), NULL, &shaderResourceView[1]);
    if (FAILED(result))
    {
      GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIAmigaScreenRenderer::ExecuteShaders() - Failed to create shader resource view for the overlay texture", result);
      return false;
    }
    samplers[1] = _overlaySamplerState;
    numViews++;
  }

  _immediateContext->PSSetShaderResources(0, numViews, shaderResourceView);

  for (unsigned int i = 0; i < numViews; i++)
  {
    ReleaseCOM(&shaderResourceView[i]);
  }

  // Set the vertex input layout.
  _immediateContext->IASetInputLayout(_polygonLayout);

  // Set the vertex and pixel shaders that will be used to render this triangle.
  _immediateContext->VSSetShader(_vertexShader, nullptr, 0);
  if (UseOverlay)
  {
    _immediateContext->PSSetShader(_overlayPixelShader, nullptr, 0);
  }
  else
  {
    _immediateContext->PSSetShader(_pixelShader, nullptr, 0);
  }

  // Set the sampler state in the pixel shader.
  _immediateContext->PSSetSamplers(0, numViews, samplers);

  // Render the two triangles
  constexpr UINT indexCount = 6;
  _immediateContext->DrawIndexed(indexCount, 0, 0);

  return true;
}

bool GfxDrvDXGIAmigaScreenRenderer::Render(GfxDrvDXGIAmigaScreenTexture &amigaScreen, ID3D11Texture2D *backbuffer)
{
  CopyAmigaScreenToShaderInput(amigaScreen);
  SetRenderTarget(backbuffer);
  SetRenderGeometry();
  return ExecuteShaders(amigaScreen);
}

bool GfxDrvDXGIAmigaScreenRenderer::IsSameConfiguration(const DimensionsUInt &hostBufferSize, const DimensionsUInt &chipsetBufferSize, const RectPixels &chipsetOutputClipPixels)
{
  return _initialized && hostBufferSize == _hostBufferSize && chipsetBufferSize == _chipsetBufferSize && chipsetOutputClipPixels == _chipsetOutputClipPixels;
}

bool GfxDrvDXGIAmigaScreenRenderer::Configure(
    ID3D11Device *d3d11device, ID3D11DeviceContext *immediateContext, const DimensionsUInt &hostBufferSize, const DimensionsUInt &chipsetBufferSize, const RectPixels &chipsetOutputClipPixels)
{
  if (IsSameConfiguration(hostBufferSize, chipsetBufferSize, chipsetOutputClipPixels))
  {
    return true;
  }

  ReleaseAllocatedResources();

  _d3d11device = d3d11device;
  _immediateContext = immediateContext;
  _hostBufferSize = hostBufferSize;
  _chipsetBufferSize = chipsetBufferSize;
  _chipsetOutputClipPixels = chipsetOutputClipPixels;

  bool success = CreateDepthDisabledStencil();
  if (success) success = CreateVertexAndIndexBuffers();
  if (success) success = CreateVertexShader();
  if (success) success = CreatePixelShaders();

  if (!success)
  {
    ReleaseAllocatedResources();
    return false;
  }

  _initialized = true;
  return true;
}

void GfxDrvDXGIAmigaScreenRenderer::ReleaseAllocatedResources()
{
  DeletePixelShaders();
  DeleteVertexShader();
  DeleteVertexAndIndexBuffers();
  DeleteDepthDisabledStencil();

  _hostBufferSize = DimensionsUInt();
  _chipsetBufferSize = DimensionsUInt();
  _chipsetOutputClipPixels = RectPixels();
  _immediateContext = nullptr;
  _d3d11device = nullptr;
  _initialized = false;
}
