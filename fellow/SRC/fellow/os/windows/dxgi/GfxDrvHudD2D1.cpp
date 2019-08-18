#include <dwrite.h>

#include "fellow/api/Services.h"
#include "GfxDrvDXGIUtils.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "GfxDrvHudD2D1.h"

using namespace fellow::api;
using namespace std;

constexpr unsigned int TextBufferCount = 32;

// Requirements:
// Direct2D 1.0 - supported on Windows 7+ and Windows Vista with SP2 and Platform Update for Windows Vista
// DirectWrite 1.0 - supported on Windows 7+ and Windows Vista with SP2 and Platform Update for Windows Vista
// Libs/dlls - dwrite.lib/.dll and d2d1.lib/.dll

bool GfxDrvHudD2D1::_requirementsValidated = false;
bool GfxDrvHudD2D1::_requirementsValidationResult = false;

bool GfxDrvHudD2D1::ValidateRequirements()
{
  if (_requirementsValidated)
  {
    return _requirementsValidationResult;
  }

  _requirementsValidated = true;

  HINSTANCE hDll = LoadLibrary("d2d1.dll");
  if (hDll)
  {
    FreeLibrary(hDll);
  }
  else
  {
    Service->Log.AddLog("GfxDrvHudD2D1::ValidateRequirements() ERROR: d2d1.dll could not be loaded.\n");
    _requirementsValidationResult = false;
    return false;
  }

  hDll = LoadLibrary("dwrite.dll");
  if (hDll)
  {
    FreeLibrary(hDll);
  }
  else
  {
    Service->Log.AddLog("GfxDrvHudD2D1::ValidateRequirements() ERROR: dwrite.dll could not be loaded.\n");
    _requirementsValidationResult = false;
    return false;
  }

  _requirementsValidationResult = true;
  return true;
}

bool GfxDrvHudD2D1::InitializeFactory()
{
  D2D1_FACTORY_OPTIONS options{};
#ifdef _DEBUG
  options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
  options.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif

  HRESULT d2d1FactoryResult = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory), &options, (void **)&_d2d1Factory);
  if (FAILED(d2d1FactoryResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvHudD2D1 failed to create ID2D1Factory", d2d1FactoryResult);
    return false;
  }

  return true;
}

void GfxDrvHudD2D1::ReleaseFactory()
{
  ReleaseCOM(&_d2d1Factory);
}

bool GfxDrvHudD2D1::InitializeRenderTarget(unsigned int width, unsigned int height, IDXGISwapChain *swapChain)
{
  IDXGISurface *dxgiSurface{};
  HRESULT getBufferResult = swapChain->GetBuffer(0, __uuidof(IDXGISurface), (void **)&dxgiSurface);
  if (FAILED(getBufferResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvHudD2D1 failed to get buffer from IDXGISwapChain", getBufferResult);
    return false;
  }

  D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties{.type = D2D1_RENDER_TARGET_TYPE_DEFAULT,
                                                       .pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
                                                       .dpiX = 0,
                                                       .dpiY = 0,
                                                       .usage = D2D1_RENDER_TARGET_USAGE_NONE,
                                                       .minLevel = D2D1_FEATURE_LEVEL::D2D1_FEATURE_LEVEL_DEFAULT};

  HRESULT d2d1RenderTargetResult = _d2d1Factory->CreateDxgiSurfaceRenderTarget(dxgiSurface, renderTargetProperties, &_d2d1RenderTarget);
  if (FAILED(d2d1RenderTargetResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvHudD2D1 failed to create ID2D1RenderTarget from ID2D1Device", d2d1RenderTargetResult);
    ReleaseCOM(&dxgiSurface);
    return false;
  }

  ReleaseCOM(&dxgiSurface);
  SetRenderTargetSize(width, height);
  return true;
}

void GfxDrvHudD2D1::ReleaseRenderTarget()
{
  ReleaseCOM(&_d2d1RenderTarget);
}

bool GfxDrvHudD2D1::InitializeSolidColorBrush(const D2D1::ColorF &color, ID2D1SolidColorBrush **solidColorBrush) const
{
  HRESULT solidColorBrushResult = _d2d1RenderTarget->CreateSolidColorBrush(color, solidColorBrush);
  if (FAILED(solidColorBrushResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvHudD2D1 failed to create ID2D1SolidColorBrush", solidColorBrushResult);
    return false;
  }

  return true;
}

bool GfxDrvHudD2D1::InitializeSolidColorBrush(D2D1::ColorF::Enum color, ID2D1SolidColorBrush **solidColorBrush) const
{
  return InitializeSolidColorBrush(D2D1::ColorF(color), solidColorBrush);
}

typedef std::pair<D2D1::ColorF, ID2D1SolidColorBrush **> ColorBrushPair;

bool GfxDrvHudD2D1::InitializeSolidColorBrushes()
{
  ColorBrushPair brushes[] = {ColorBrushPair(D2D1::ColorF::Black, &_blackBrush),        ColorBrushPair(D2D1::ColorF::LightGreen, &_lightGreenBrush),
                              ColorBrushPair(D2D1::ColorF::Green, &_greenBrush),        ColorBrushPair(D2D1::ColorF::DarkGreen, &_darkGreenBrush),
                              ColorBrushPair(D2D1::ColorF::Gray, &_grayBrush),          ColorBrushPair(0x505050, &_darkGrayBrush),
                              ColorBrushPair(D2D1::ColorF::IndianRed, &_lightRedBrush), ColorBrushPair(D2D1::ColorF::Red, &_redBrush),
                              ColorBrushPair(D2D1::ColorF::DarkRed, &_darkRedBrush),    ColorBrushPair(D2D1::ColorF::LightYellow, &_lightYellowBrush)};

  for (auto brush : brushes)
  {
    if (!InitializeSolidColorBrush(brush.first, brush.second))
    {
      ReleaseSolidColorBrushes();
      return false;
    }
  }

  return true;
}

void GfxDrvHudD2D1::ReleaseSolidColorBrushes()
{
  ReleaseCOM(&_lightYellowBrush);
  ReleaseCOM(&_darkRedBrush);
  ReleaseCOM(&_redBrush);
  ReleaseCOM(&_lightRedBrush);
  ReleaseCOM(&_darkGrayBrush);
  ReleaseCOM(&_grayBrush);
  ReleaseCOM(&_darkGreenBrush);
  ReleaseCOM(&_greenBrush);
  ReleaseCOM(&_lightGreenBrush);
  ReleaseCOM(&_blackBrush);
}

bool GfxDrvHudD2D1::InitializeTextFormats()
{
  IDWriteFactory *dwriteFactory{};
  HRESULT dwriteFactoryResult = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown **)&dwriteFactory);
  if (FAILED(dwriteFactoryResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvHudD2D1 failed to create IDWriteFactory", dwriteFactoryResult);
    return false;
  }

  HRESULT textFormatResult = dwriteFactory->CreateTextFormat(_fontname, nullptr, DWRITE_FONT_WEIGHT_LIGHT, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20.0f, _locale, &_textFormat);
  if (FAILED(textFormatResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvHudD2D1 failed to create IDWriteTextFormat", textFormatResult);
    ReleaseCOM(&dwriteFactory);
    return false;
  }

  HRESULT textAlignmentResult = _textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
  if (FAILED(textAlignmentResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvHudD2D1 failed to set text alignment on IDWriteTextFormat", textAlignmentResult);
    ReleaseTextFormats();
    ReleaseCOM(&dwriteFactory);
    return false;
  }

  HRESULT paragraphAlignmentResult = _textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  if (FAILED(paragraphAlignmentResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvHudD2D1 failed to set paragraph alignment on IDWriteTextFormat", paragraphAlignmentResult);
    ReleaseTextFormats();
    ReleaseCOM(&dwriteFactory);
    return false;
  }

  ReleaseCOM(&dwriteFactory);
  return true;
}

void GfxDrvHudD2D1::ReleaseTextFormats()
{
  ReleaseCOM(&_textFormat);
}

bool GfxDrvHudD2D1::InitializeAllResources(unsigned int width, unsigned int height, IDXGISwapChain *swapChain, HudPropertyProvider *hudPropertyProvider)
{
  _hudPropertyProvider = hudPropertyProvider;

  bool success = InitializeFactory();
  if (!success)
  {
    return false;
  }

  success = InitializeRenderTarget(width, height, swapChain);
  if (!success)
  {
    ReleaseFactory();
    return false;
  }

  success = InitializeSolidColorBrushes();
  if (!success)
  {
    ReleaseRenderTarget();
    ReleaseFactory();
    return false;
  }

  success = InitializeTextFormats();
  if (!success)
  {
    ReleaseSolidColorBrushes();
    ReleaseRenderTarget();
    ReleaseFactory();
  }

  return success;
}

void GfxDrvHudD2D1::ReleaseAllResources()
{
  ReleaseTextFormats();
  ReleaseSolidColorBrushes();
  ReleaseRenderTarget();
  ReleaseFactory();
  _hudPropertyProvider = nullptr; // No delete, does not own this
}

D2D1_RECT_F GfxDrvHudD2D1::GetLayoutRectForLED(unsigned int ledIndex) const
{
  const float offsetX = _ledConfiguration.InitialX + (_ledConfiguration.Width + _ledConfiguration.Spacing) * (float)ledIndex;
  const float offsetY = _ledConfiguration.InitialY;

  return D2D1_RECT_F{.left = offsetX, .top = offsetY, .right = offsetX + _ledConfiguration.Width, .bottom = offsetY + _ledConfiguration.Height};
}

D2D1_RECT_F GfxDrvHudD2D1::GetLayoutRectForFPSCounter() const
{
  // Placement: upper right corner
  float width = 72;
  float height = 32;
  float initialX = _renderTargetWidth - width;
  float initialY = 0;

  return D2D1_RECT_F{.left = initialX, .top = initialY, .right = initialX + width, .bottom = initialY + height};
}

void GfxDrvHudD2D1::DrawFloppyDriveInfo(unsigned int floppyDriveIndex, HudFloppyDriveStatus hudFloppyDriveStatus) const
{
  unsigned int ledIndex = floppyDriveIndex + 1;
  D2D1_ROUNDED_RECT rect{.rect = GetLayoutRectForLED(ledIndex), .radiusX = _ledConfiguration.RoundedCornerXRadius, .radiusY = _ledConfiguration.RoundedCornerYRadius};

  _d2d1RenderTarget->FillRoundedRectangle(rect, hudFloppyDriveStatus.DriveExists ? (hudFloppyDriveStatus.MotorOn ? _lightGreenBrush : _greenBrush) : _grayBrush);
  _d2d1RenderTarget->DrawRoundedRectangle(rect, hudFloppyDriveStatus.DriveExists ? _darkGreenBrush : _darkGrayBrush, _ledConfiguration.BorderStrokeWidth);

  wchar_t text[TextBufferCount]{};
  if (hudFloppyDriveStatus.DriveExists)
  {
    swprintf(text, TextBufferCount, L"DF%u: %u", floppyDriveIndex, hudFloppyDriveStatus.Track);
  }
  else
  {
    swprintf(text, TextBufferCount, L"DF%u", floppyDriveIndex);
  }

  _d2d1RenderTarget->DrawText(text, (UINT32)wcslen(text), _textFormat, rect.rect, _blackBrush);
}

void GfxDrvHudD2D1::DrawPowerLED(bool enabled) const
{
  unsigned int ledIndex = 0;
  D2D1_ROUNDED_RECT rect{.rect = GetLayoutRectForLED(ledIndex), .radiusX = _ledConfiguration.RoundedCornerXRadius, .radiusY = _ledConfiguration.RoundedCornerYRadius};

  _d2d1RenderTarget->FillRoundedRectangle(rect, enabled ? _lightRedBrush : _redBrush);
  _d2d1RenderTarget->DrawRoundedRectangle(rect, _darkRedBrush, _ledConfiguration.BorderStrokeWidth);
}

void GfxDrvHudD2D1::DrawFPSCounter(unsigned int fps) const
{
  D2D1_RECT_F rect = GetLayoutRectForFPSCounter();
  wchar_t text[TextBufferCount];
  swprintf(text, TextBufferCount, L"%u", fps);

  _d2d1RenderTarget->DrawText(text, (UINT32)wcslen(text), _textFormat, rect, _lightYellowBrush);
}

void GfxDrvHudD2D1::DrawHUD(const HudConfiguration &hudConfiguration) const
{
  if (!hudConfiguration.LEDEnabled && !hudConfiguration.FPSEnabled)
  {
    return;
  }

  _d2d1RenderTarget->BeginDraw();

  if (hudConfiguration.LEDEnabled)
  {
    DrawPowerLED(_hudPropertyProvider->GetPowerLedOn());

    for (unsigned int floppyDriveIndex = 0; floppyDriveIndex < 4; floppyDriveIndex++)
    {
      DrawFloppyDriveInfo(floppyDriveIndex, _hudPropertyProvider->GetFloppyDriveStatus(floppyDriveIndex));
    }
  }

  if (hudConfiguration.FPSEnabled)
  {
    DrawFPSCounter(_hudPropertyProvider->GetFps());
  }

  HRESULT endDrawResult = _d2d1RenderTarget->EndDraw();
  if (FAILED(endDrawResult))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvHudD2D1 ID2D1DeviceContext->EndDraw() had errors", endDrawResult);
  }
}

void GfxDrvHudD2D1::SetRenderTargetSize(unsigned int width, unsigned int height)
{
  _renderTargetWidth = (float)width;
  _renderTargetHeight = (float)height;
}

void GfxDrvHudD2D1::Resizing()
{
  ReleaseRenderTarget();
}

bool GfxDrvHudD2D1::Resized(unsigned int newWidth, unsigned int newHeight, IDXGISwapChain *swapChain)
{
  return InitializeRenderTarget(newWidth, newHeight, swapChain);
}

bool GfxDrvHudD2D1::EmulationStart(unsigned int initialWidth, unsigned int initialHeight, IDXGISwapChain *swapChain, HudPropertyProvider* hudPropertyProvider)
{
  Service->Log.AddLog("GfxDrvHudD2D1::EmulationStart() Starting\n");

  bool success = InitializeAllResources(initialWidth, initialHeight, swapChain, hudPropertyProvider);

  Service->Log.AddLog("GfxDrvHudD2D1::EmulationStart() Finished%s\n", success ? "" : " and failed");
  return success;
}

void GfxDrvHudD2D1::EmulationStop()
{
  ReleaseAllResources();
}

GfxDrvHudD2D1::GfxDrvHudD2D1() : _renderTargetWidth(0.0f), _renderTargetHeight(0.0f)
{
}
