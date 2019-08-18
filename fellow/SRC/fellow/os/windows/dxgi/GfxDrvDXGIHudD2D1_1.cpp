//#include <map>
//
//#include "fellow/api/Services.h"
//#include "GfxDrvDXGI.h"
//#include "GfxDrvDXGIUtils.h"
//#include "GfxDrvDXGIErrorLogger.h"
//#include "GfxDrvDXGIHudD2D1_1.h"
//
//#include "FLOPPY.H"
//#include "CIA.H"
//#include "DRAW.H"
//
// using namespace fellow::api;
// using namespace std;
//
//// Direct2D 1.1 - supported on Windows 8 and Platform Update for Windows 7
//// DirectWrite 1.0 - supported on Windows 7, Windows Vista with SP2 and Platform Update for Windows Vista
//// Libs/dlls - dwrite.lib/.dll and d2d1.lib/.dll
//
// extern GfxDrvDXGI* gfxDrvDXGI;
//
// bool GfxDrvDXGIHudD2D1_1::InitializeDeviceContext()
//{
//  D2D1_FACTORY_OPTIONS options{};
//#ifdef _DEBUG
//  options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
//#else
//  options.debugLevel = D2D1_DEBUG_LEVEL_NONE;
//#endif
//
//  ID2D1Factory1* d2d1Factory1{};
//  HRESULT d2d1FactoryResult = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1Factory), &options, (void**)&d2d1Factory1);
//  if (FAILED(d2d1FactoryResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to create ID2D1Factory1", d2d1FactoryResult);
//    return false;
//  }
//
//  IDXGIDevice* dxgiDevice{};
//  HRESULT dxgiDeviceQueryResult = gfxDrvDXGI->_d3d11device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
//  if (FAILED(dxgiDeviceQueryResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to query interface IDXGIDevice on ID3D11Device", dxgiDeviceQueryResult);
//    ReleaseCOM(&d2d1Factory1);
//    return false;
//  }
//
//  ID2D1Device* d2d1Device{};
//  HRESULT createD2D1DeviceResult = d2d1Factory1->CreateDevice(dxgiDevice, &d2d1Device);
//  if (FAILED(createD2D1DeviceResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to create ID2D1Device from IDXGIDevice", createD2D1DeviceResult);
//    ReleaseCOM(&dxgiDevice);
//    ReleaseCOM(&d2d1Factory1);
//    return false;
//  }
//
//  HRESULT d2d1DeviceContextResult = d2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &_d2d1DeviceContext);
//  if (FAILED(createD2D1DeviceResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to create ID2D1DeviceContext from ID2D1Device", d2d1DeviceContextResult);
//    ReleaseCOM(&d2d1Device);
//    ReleaseCOM(&dxgiDevice);
//    ReleaseCOM(&d2d1Factory1);
//    return false;
//  }
//
//  ReleaseCOM(&d2d1Device);
//  ReleaseCOM(&dxgiDevice);
//  ReleaseCOM(&d2d1Factory1);
//  return true;
//}
//
// void GfxDrvDXGIHudD2D1_1::ReleaseDeviceContext()
//{
//  ReleaseCOM(&_d2d1DeviceContext);
//}
//
// bool GfxDrvDXGIHudD2D1_1::InitializeBitmapRenderTarget()
//{
//  ReleaseBitmapRenderTarget(); // In case there is an old one ie. Resize() called this function
//
//  IDXGISurface* dxgiSurface{};
//  HRESULT getBufferResult = gfxDrvDXGI->_swapChain->GetBuffer(0, __uuidof(IDXGISurface), (void**)&dxgiSurface);
//  if (FAILED(getBufferResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to get IDXGISurface from IDXGISwapChain", getBufferResult);
//    return false;
//  }
//
//  D2D1_BITMAP_PROPERTIES1 bp{};
//  bp.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
//  bp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
//  bp.dpiX = 0.0f;
//  bp.dpiY = 0.0f;
//  //bp.dpiX = 96.0f;
//  //bp.dpiY = 96.0f;
//  bp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
//  bp.colorContext = nullptr;
//
//  HRESULT bitmapResult = _d2d1DeviceContext->CreateBitmapFromDxgiSurface(dxgiSurface, &bp, &_targetBitmap);
//  if (FAILED(bitmapResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to create ID2D1Bitmap1 from IDXGISurface", bitmapResult);
//    ReleaseCOM(&dxgiSurface);
//    return false;
//  }
//
//  _d2d1DeviceContext->SetTarget(_targetBitmap);
//  ReleaseCOM(&dxgiSurface);
//  return true;
//}
//
// void GfxDrvDXGIHudD2D1_1::ReleaseBitmapRenderTarget()
//{
//  ReleaseCOM(&_targetBitmap);
//}
//
// bool GfxDrvDXGIHudD2D1_1::InitializeSolidColorBrush(const D2D1::ColorF& color, ID2D1SolidColorBrush** solidColorBrush)
//{
//  HRESULT solidColorBrushResult = _d2d1DeviceContext->CreateSolidColorBrush(color, solidColorBrush);
//  if (FAILED(solidColorBrushResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to create ID2D1SolidColorBrush", solidColorBrushResult);
//    return false;
//  }
//
//  return true;
//}
//
// bool GfxDrvDXGIHudD2D1_1::InitializeSolidColorBrush(D2D1::ColorF::Enum color, ID2D1SolidColorBrush** solidColorBrush)
//{
//  return InitializeSolidColorBrush(D2D1::ColorF(color), solidColorBrush);
//}
//
// typedef std::pair<D2D1::ColorF, ID2D1SolidColorBrush**> ColorBrushPair;
//
// bool GfxDrvDXGIHudD2D1_1::InitializeSolidColorBrushes()
//{
//  ColorBrushPair brushes[] =
//  {
//    ColorBrushPair(D2D1::ColorF::Black, &_blackBrush),
//    ColorBrushPair(D2D1::ColorF::LightGreen, &_lightGreenBrush),
//    ColorBrushPair(D2D1::ColorF::Green, &_greenBrush),
//    ColorBrushPair(D2D1::ColorF::DarkGreen, &_darkGreenBrush),
//    ColorBrushPair(D2D1::ColorF::Gray, &_grayBrush),
//    ColorBrushPair(0x505050, &_darkGrayBrush),
//    ColorBrushPair(D2D1::ColorF::IndianRed, &_lightRedBrush),
//    ColorBrushPair(D2D1::ColorF::Red, &_redBrush),
//    ColorBrushPair(D2D1::ColorF::DarkRed, &_darkRedBrush),
//    ColorBrushPair(D2D1::ColorF::LightYellow, &_lightYellowBrush)
//  };
//
//  for (auto brush : brushes)
//  {
//    if (!InitializeSolidColorBrush(brush.first, brush.second))
//    {
//      ReleaseSolidColorBrushes();
//      return false;
//    }
//  }
//
//  return true;
//}
//
// void GfxDrvDXGIHudD2D1_1::ReleaseSolidColorBrushes()
//{
//  ReleaseCOM(&_lightYellowBrush);
//  ReleaseCOM(&_darkRedBrush);
//  ReleaseCOM(&_redBrush);
//  ReleaseCOM(&_lightRedBrush);
//  ReleaseCOM(&_darkGrayBrush);
//  ReleaseCOM(&_grayBrush);
//  ReleaseCOM(&_darkGreenBrush);
//  ReleaseCOM(&_greenBrush);
//  ReleaseCOM(&_lightGreenBrush);
//  ReleaseCOM(&_blackBrush);
//}
//
// bool GfxDrvDXGIHudD2D1_1::InitializeTextFormats()
//{
//  // Used to create text formats
//  // DWriteCreateFactory support: Windows 7, Windows Vista with SP2 and Platform Update for Windows Vista
//  // IDWriteFactory support: Windows 7, Windows Vista with SP2 and Platform Update for Windows Vista
//  // IDWriteFactory2 support: Windows 8.1
//  // IDWriteFactory2 is used because unknown, trying IDWriteFactory
//  IDWriteFactory* dwriteFactory{};
//  HRESULT dwriteFactoryResult = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwriteFactory);
//  if (FAILED(dwriteFactoryResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to create IDWriteFactory", dwriteFactoryResult);
//    return false;
//  }
//
//  HRESULT textFormatResult = dwriteFactory->CreateTextFormat(
//    _fontname,
//    nullptr,
//    DWRITE_FONT_WEIGHT_LIGHT,
//    DWRITE_FONT_STYLE_NORMAL,
//    DWRITE_FONT_STRETCH_NORMAL,
//    20.0f,
//    _locale,
//    &_textFormat);
//  if (FAILED(textFormatResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to create IDWriteTextFormat", textFormatResult);
//    ReleaseCOM(&dwriteFactory);
//    return false;
//  }
//
//  HRESULT textAlignmentResult = _textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
//  if (FAILED(textAlignmentResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to set text alignment on IDWriteTextFormat", textAlignmentResult);
//    ReleaseTextFormats();
//    ReleaseCOM(&dwriteFactory);
//    return false;
//  }
//
//  HRESULT paragraphAlignmentResult = _textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
//  if (FAILED(paragraphAlignmentResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 failed to set paragraph alignment on IDWriteTextFormat", paragraphAlignmentResult);
//    ReleaseTextFormats();
//    ReleaseCOM(&dwriteFactory);
//    return false;
//  }
//
//  ReleaseCOM(&dwriteFactory);
//  return true;
//}
//
// void GfxDrvDXGIHudD2D1_1::ReleaseTextFormats()
//{
//  ReleaseCOM(&_textFormat);
//}
//
// bool GfxDrvDXGIHudD2D1_1::InitializeAllResources(const DimensionsUInt& hostBufferSize)
//{
//  _hostBufferSize = hostBufferSize;
//
//  bool success = InitializeDeviceContext();
//  if (!success)
//  {
//    return false;
//  }
//
//  success = InitializeSolidColorBrushes();
//  if (!success)
//  {
//    ReleaseDeviceContext();
//    return false;
//  }
//
//  success = InitializeTextFormats();
//  if (!success)
//  {
//    ReleaseSolidColorBrushes();
//    ReleaseDeviceContext();
//  }
//
//  return success;
//}
//
// void GfxDrvDXGIHudD2D1_1::ReleaseAllResources()
//{
//  ReleaseTextFormats();
//  ReleaseSolidColorBrushes();
//  ReleaseBitmapRenderTarget();
//  ReleaseDeviceContext();
//}
//
// D2D1_RECT_F GfxDrvDXGIHudD2D1_1::GetLayoutRectForLED(unsigned int ledIndex)
//{
//  float initialX = 24.0f;
//  float initialY = 16.0f;
//  float width = 96.0f;
//  float height = 32.0f;
//  float spacing = 32.0f;
//
//  return D2D1_RECT_F
//  {
//    .left = initialX + ((width + spacing) * ledIndex),
//    .top = initialY,
//    .right = initialX + ((width + spacing) * ledIndex) + width,
//    .bottom = initialY + height
//  };
//}
//
// D2D1_RECT_F GfxDrvDXGIHudD2D1_1::GetLayoutRectForFPSCounter()
//{
//  float width = 72;
//  float height = 32;
//  float initialX = _hostBufferSize.Width - width;
//  float initialY = 0;
//
//  return D2D1_RECT_F
//  {
//    .left = initialX,
//    .top = initialY,
//    .right = initialX + width,
//    .bottom = initialY + height
//  };
//}
//
// void GfxDrvDXGIHudD2D1_1::DrawFloppyInfo(unsigned int driveIndex, bool enabled, bool motorOn, unsigned int track)
//{
//  unsigned int ledIndex = driveIndex + 1;
//  D2D1_ROUNDED_RECT rect
//  {
//    .rect = GetLayoutRectForLED(ledIndex),
//    .radiusX = 3.0F,
//    .radiusY = 3.0f
//  };
//
//  _d2d1DeviceContext->FillRoundedRectangle(rect, enabled ? (motorOn ? _lightGreenBrush : _greenBrush) : _grayBrush);
//  _d2d1DeviceContext->DrawRoundedRectangle(rect, enabled ? _darkGreenBrush : _darkGrayBrush, 2.0f);
//
//  wchar_t text[32];
//  if (enabled)
//  {
//    swprintf(text, 32, L"DF%u: %u", driveIndex, track);
//  }
//  else
//  {
//    swprintf(text, 32, L"DF%u", driveIndex);
//  }
//
//  _d2d1DeviceContext->DrawText(text, wcslen(text), _textFormat, rect.rect, _blackBrush);
//}
//
// void GfxDrvDXGIHudD2D1_1::DrawPowerLED(bool enabled)
//{
//  unsigned int ledIndex = 0;
//  D2D1_ROUNDED_RECT rect
//  {
//    .rect = GetLayoutRectForLED(ledIndex),
//    .radiusX = 3.0F,
//    .radiusY = 3.0f
//  };
//
//  _d2d1DeviceContext->FillRoundedRectangle(rect, enabled ? _lightRedBrush : _redBrush);
//  _d2d1DeviceContext->DrawRoundedRectangle(rect, _darkRedBrush, 2.0f);
//}
//
// void GfxDrvDXGIHudD2D1_1::DrawFPSCounter()
//{
//  D2D1_RECT_F rect = GetLayoutRectForFPSCounter();
//  wchar_t text[32];
//  swprintf(text, 32, L"%u", drawStatLast50FramesFps());
//
//  _d2d1DeviceContext->DrawText(text, wcslen(text), _textFormat, rect, _lightYellowBrush);
//}
//
// void GfxDrvDXGIHudD2D1_1::PrintHUD()
//{
//  _d2d1DeviceContext->BeginDraw();
//
//  DrawPowerLED(ciaIsPowerLEDEnabled());
//
//  for (unsigned int driveIndex = 0; driveIndex < 4; driveIndex++)
//  {
//    DrawFloppyInfo(driveIndex, floppyGetEnabled(driveIndex), floppyGetMotor(driveIndex), floppyGetTrack(driveIndex));
//  }
//
//  DrawFPSCounter();
//
//  HRESULT endDrawResult = _d2d1DeviceContext->EndDraw();
//  if (FAILED(endDrawResult))
//  {
//    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIHudD2D1_1 ID2D1DeviceContext->EndDraw() had errors", endDrawResult);
//  }
//}
//
// bool GfxDrvDXGIHudD2D1_1::Resize()
//{
//  return InitializeBitmapRenderTarget();
//}
//
// bool GfxDrvDXGIHudD2D1_1::EmulationStart(const DimensionsUInt& hostBufferSize)
//{
//  Service->Log.AddLog("GfxDrvDXGIHudD2D1_1::EmulationStart() Starting\n");
//
//  bool success = InitializeAllResources(hostBufferSize);
//
//  Service->Log.AddLog("GfxDrvDXGIHudD2D1_1::EmulationStart() Finished%s\n", success ? "" : " and failed");
//  return success;
//}
//
// void GfxDrvDXGIHudD2D1_1::EmulationStop()
//{
//  ReleaseAllResources();
//}
