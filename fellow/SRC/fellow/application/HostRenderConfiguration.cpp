#include "fellow/application/HostRenderConfiguration.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

void HostRenderConfiguration::Log()
{
  const char *prefix = "HostRenderConfiguration -";

  Service->Log.AddLog("%s Display size and output kind is %s %s\n", prefix, Size.ToString().c_str(), ToString(OutputKind));
  if (OutputKind == DisplayOutputKind::Fullscreen)
  {
    Service->Log.AddLog("%s Fullscreen color depth and refresh rate is %s %u Hz\n", prefix, ToString(ColorDepth), RefreshRate);
  }
  Service->Log.AddLog("%s Display scale and strategy is %s %s\n", prefix, ToString(Scale), ToString(ScaleStrategy));
  Service->Log.AddLog("%s Display driver is %s\n", prefix, ToString(Driver));
  Service->Log.AddLog("%s Frameskip ratio is 1/%u\n", prefix, FrameskipRatio + 1);
  Service->Log.AddLog("%s Deinterlace is %s\n", prefix, Deinterlace ? "enabled" : "disabled");
  Service->Log.AddLog("%s OutputClip is %s\n", prefix, ChipsetOutputClip.ToString().c_str());
}

const char *HostRenderConfiguration::ToString(DisplayOutputKind outputKind)
{
  switch (outputKind)
  {
    case DisplayOutputKind::Windowed: return "Windowed";
    case DisplayOutputKind::Fullscreen: return "Fullscreen";
    default: return "Unknown DisplayOutputKind";
  }
}

const char *HostRenderConfiguration::ToString(DisplayColorDepth colorDepth)
{
  switch (colorDepth)
  {
    case DisplayColorDepth::Color16Bits: return "Color16Bits";
    case DisplayColorDepth::Color24Bits: return "Color24Bits";
    case DisplayColorDepth::Color32Bits: return "Color32Bits";
    default: return "Unknown DisplayColorDepth";
  }
}

const char *HostRenderConfiguration::ToString(DisplayScale scale)
{
  switch (scale)
  {
    case DisplayScale::Automatic: return "Automatic";
    case DisplayScale::FixedScale1X: return "FixedScale1X";
    case DisplayScale::FixedScale2X: return "FixedScale2X";
    case DisplayScale::FixedScale3X: return "FixedScale3X";
    case DisplayScale::FixedScale4X: return "FixedScale4X";
    default: return "Unknown DisplayScale";
  }
}

const char *HostRenderConfiguration::ToString(DisplayScaleStrategy scaleStrategy)
{
  switch (scaleStrategy)
  {
    case DisplayScaleStrategy::Solid: return "Solid";
    case DisplayScaleStrategy::Scanlines: return "Scanlines";
    default: return "Unknown DisplayScaleStrategy";
  }
}

const char *HostRenderConfiguration::ToString(DisplayDriver driver)
{
  switch (driver)
  {
    case DisplayDriver::DirectDraw: return "DirectDraw";
    case DisplayDriver::Direct3D11: return "Direct3D11";
    default: return "Unknown DisplayDriver";
  }
}