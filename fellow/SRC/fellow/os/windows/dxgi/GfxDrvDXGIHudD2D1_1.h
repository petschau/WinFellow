//#pragma once
//
//#include <dwrite.h>
//#include <d2d1_1.h>
//
// class GfxDrvDXGIHudD2D1_1
//{
// private:
//  ID2D1DeviceContext* _d2d1DeviceContext{};
//  ID2D1Bitmap1* _targetBitmap{};
//  ID2D1SolidColorBrush* _blackBrush{};
//  ID2D1SolidColorBrush* _lightGreenBrush{};
//  ID2D1SolidColorBrush* _greenBrush{};
//  ID2D1SolidColorBrush* _darkGreenBrush{};
//  ID2D1SolidColorBrush* _lightRedBrush{};
//  ID2D1SolidColorBrush* _redBrush{};
//  ID2D1SolidColorBrush* _darkRedBrush{};
//  ID2D1SolidColorBrush* _grayBrush{};
//  ID2D1SolidColorBrush* _darkGrayBrush{};
//  ID2D1SolidColorBrush* _lightYellowBrush{};
//  IDWriteTextFormat* _textFormat{};
//
//  DimensionsUInt _hostBufferSize{};
//
//  const wchar_t *_locale = L"en-GB";
//  const wchar_t* _fontname = L"Consolas";
//
//  bool InitializeDeviceContext();
//  void ReleaseDeviceContext();
//  bool InitializeBitmapRenderTarget();
//  void ReleaseBitmapRenderTarget();
//
//  bool InitializeSolidColorBrush(const D2D1::ColorF& color, ID2D1SolidColorBrush** solidColorBrush);
//  bool InitializeSolidColorBrush(D2D1::ColorF::Enum color, ID2D1SolidColorBrush** solidColorBrush);
//  bool InitializeSolidColorBrushes();
//  void ReleaseSolidColorBrushes();
//
//  bool InitializeTextFormats();
//  void ReleaseTextFormats();
//
//  bool InitializeAllResources(const DimensionsUInt& hostBufferSize);
//  void ReleaseAllResources();
//
//  D2D1_RECT_F GetLayoutRectForLED(unsigned int ledIndex);
//  D2D1_RECT_F GetLayoutRectForFPSCounter();
//
//  void DrawFloppyInfo(unsigned int driveIndex, bool enabled, bool motorOn, unsigned int track);
//  void DrawPowerLED(bool enabled);
//  void DrawFPSCounter();
//
// public:
//  void PrintHUD();
//
//  bool Resize();
//
//  bool EmulationStart(const DimensionsUInt& hostBufferSize);
//  void EmulationStop();
//};
