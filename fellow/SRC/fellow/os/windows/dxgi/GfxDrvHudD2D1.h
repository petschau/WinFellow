#pragma once

#include <d2d1.h>

#include "fellow/api/configuration/HudConfiguration.h"
#include "fellow/hud/HudPropertyProvider.h"

struct GfxDrvHudD2D1LedConfiguration
{
  float InitialX = 24.0f;
  float InitialY = 16.0f;
  float Width = 96.0f;
  float Height = 32.0f;
  float Spacing = 32.0f;
  float RoundedCornerXRadius = 3.0f;
  float RoundedCornerYRadius = 3.0f;
  float BorderStrokeWidth = 2.0f;
};

class GfxDrvHudD2D1
{
private:
  static bool _requirementsValidated;
  static bool _requirementsValidationResult;

  HudPropertyProvider *_hudPropertyProvider;
  GfxDrvHudD2D1LedConfiguration _ledConfiguration;

  ID2D1Factory *_d2d1Factory{};
  ID2D1RenderTarget *_d2d1RenderTarget{};
  ID2D1SolidColorBrush *_blackBrush{};
  ID2D1SolidColorBrush *_lightGreenBrush{};
  ID2D1SolidColorBrush *_greenBrush{};
  ID2D1SolidColorBrush *_darkGreenBrush{};
  ID2D1SolidColorBrush *_lightRedBrush{};
  ID2D1SolidColorBrush *_redBrush{};
  ID2D1SolidColorBrush *_darkRedBrush{};
  ID2D1SolidColorBrush *_grayBrush{};
  ID2D1SolidColorBrush *_darkGrayBrush{};
  ID2D1SolidColorBrush *_lightYellowBrush{};
  IDWriteTextFormat *_textFormat{};

  const wchar_t *_locale = L"en-GB";
  const wchar_t *_fontname = L"Consolas";

  float _renderTargetWidth;
  float _renderTargetHeight;

  bool InitializeFactory();
  void ReleaseFactory();

  bool InitializeRenderTarget(unsigned int initialWidth, unsigned int initialHeight, IDXGISwapChain *swapChain);
  void ReleaseRenderTarget();
  void SetRenderTargetSize(unsigned int width, unsigned int height);

  bool InitializeSolidColorBrush(const D2D1::ColorF &color, ID2D1SolidColorBrush **solidColorBrush) const;
  bool InitializeSolidColorBrush(D2D1::ColorF::Enum color, ID2D1SolidColorBrush **solidColorBrush) const;
  bool InitializeSolidColorBrushes();
  void ReleaseSolidColorBrushes();

  bool InitializeTextFormats();
  void ReleaseTextFormats();

  bool InitializeAllResources(unsigned int initialWidth, unsigned int initialHeight, IDXGISwapChain *swapChain, HudPropertyProvider *hudPropertyProvider);
  void ReleaseAllResources();

  D2D1_RECT_F GetLayoutRectForLED(unsigned int ledIndex) const;
  D2D1_RECT_F GetLayoutRectForFPSCounter() const;

  void DrawFloppyDriveInfo(unsigned int floppyDriveIndex, HudFloppyDriveStatus hudFloppyDriveStatus) const;
  void DrawPowerLED(bool enabled) const;
  void DrawFPSCounter(unsigned int fps) const;

public:
  void DrawHUD(const HudConfiguration &hudConfiguration) const;

  void Resizing();
  bool Resized(unsigned int newWidth, unsigned int newHeight, IDXGISwapChain *swapChain);

  static bool ValidateRequirements();

  bool EmulationStart(unsigned int initialWidth, unsigned int initialHeight, IDXGISwapChain *swapChain, HudPropertyProvider *hudPropertyProvider);
  void EmulationStop();

  GfxDrvHudD2D1();
};
