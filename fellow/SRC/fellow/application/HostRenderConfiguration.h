#pragma once

#include "fellow/application/DrawDimensionTypes.h"

enum class DisplayOutputKind
{
  Windowed,
  Fullscreen
};

enum class DisplayColorDepth
{
  Unassigned,
  Color16Bits,
  Color24Bits,
  Color32Bits
};

enum class DisplayScale
{
  Automatic,
  FixedScale1X,
  FixedScale2X,
  FixedScale3X,
  FixedScale4X
};

enum class DisplayScaleStrategy
{
  Solid,
  Scanlines
};

enum class DisplayDriver
{
  DirectDraw,
  Direct3D11
};

struct HostRenderConfiguration
{
  DimensionsUInt Size;
  DisplayOutputKind OutputKind;
  DisplayColorDepth ColorDepth;
  unsigned int RefreshRate;
  DisplayScale Scale;
  DisplayScaleStrategy ScaleStrategy;
  DisplayDriver Driver;
  RectShresi ChipsetOutputClip;
  unsigned int FrameskipRatio; // Frame-skip ratio, 1 / (ratio + 1)
  bool Deinterlace;

  void Log();

  static const char *ToString(DisplayOutputKind displayOutputKind);
  static const char *ToString(DisplayColorDepth displayColorDepth);
  static const char *ToString(DisplayScale scale);
  static const char *ToString(DisplayScaleStrategy scaleStrategy);
  static const char *ToString(DisplayDriver driver);
};