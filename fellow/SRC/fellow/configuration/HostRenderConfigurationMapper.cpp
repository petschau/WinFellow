#include "fellow/configuration/HostRenderConfigurationMapper.h"

using namespace std;

HostRenderConfiguration HostRenderConfigurationMapper::Map(cfg *config)
{
  return HostRenderConfiguration{
      .Size = HostRenderConfigurationMapper::MapDisplaySize(cfgGetScreenWidth(config), cfgGetScreenHeight(config)),
      .OutputKind = HostRenderConfigurationMapper::MapDisplayOutputKind(cfgGetScreenWindowed(config)),
      .ColorDepth = HostRenderConfigurationMapper::MapDisplayColorDepth(cfgGetScreenWindowed(config), cfgGetScreenColorBits(config)),
      .RefreshRate = HostRenderConfigurationMapper::MapDisplayRefreshRate(cfgGetScreenWindowed(config), cfgGetScreenRefresh(config)),
      .Scale = HostRenderConfigurationMapper::MapDisplayScale(cfgGetDisplayScale(config)),
      .ScaleStrategy = HostRenderConfigurationMapper::MapDisplayScaleStrategy(cfgGetDisplayScaleStrategy(config)),
      .Driver = HostRenderConfigurationMapper::MapDisplayDriver(cfgGetDisplayDriver(config)),
      .ChipsetOutputClip = HostRenderConfigurationMapper::MapChipsetOutputClip(cfgGetClipAmigaLeft(config), cfgGetClipAmigaTop(config), cfgGetClipAmigaRight(config), cfgGetClipAmigaBottom(config)),
      .FrameskipRatio = cfgGetFrameskipRatio(config),
      .Deinterlace = cfgGetDeinterlace(config)};
}

DimensionsUInt HostRenderConfigurationMapper::MapDisplaySize(unsigned int displayWidth, unsigned int displayHeight)
{
  return DimensionsUInt(displayWidth, displayHeight);
}

DisplayOutputKind HostRenderConfigurationMapper::MapDisplayOutputKind(bool isScreenWindowed)
{
  return isScreenWindowed ? DisplayOutputKind::Windowed : DisplayOutputKind::Fullscreen;
}

DisplayColorDepth HostRenderConfigurationMapper::MapDisplayColorDepth(bool isScreenWindowed, unsigned int screenColorBits)
{
  if (isScreenWindowed) return DisplayColorDepth::Unassigned;

  switch (screenColorBits)
  {
    case 16: return DisplayColorDepth::Color16Bits;
    case 24: return DisplayColorDepth::Color24Bits;
    case 32: return DisplayColorDepth::Color32Bits;
    default: ThrowInvalidOption();
  }
}

unsigned int HostRenderConfigurationMapper::MapDisplayRefreshRate(bool isScreenWindowed, unsigned int screenRefresh)
{
  return (isScreenWindowed) ? 0 : screenRefresh;
}

DisplayScale HostRenderConfigurationMapper::MapDisplayScale(DISPLAYSCALE displayScale)
{
  switch (displayScale)
  {
    case DISPLAYSCALE::DISPLAYSCALE_AUTO: return DisplayScale::Automatic;
    case DISPLAYSCALE::DISPLAYSCALE_1X: return DisplayScale::FixedScale1X;
    case DISPLAYSCALE::DISPLAYSCALE_2X: return DisplayScale::FixedScale2X;
    case DISPLAYSCALE::DISPLAYSCALE_3X: return DisplayScale::FixedScale3X;
    case DISPLAYSCALE::DISPLAYSCALE_4X: return DisplayScale::FixedScale4X;
    default: ThrowInvalidOption();
  }
}

DisplayScaleStrategy HostRenderConfigurationMapper::MapDisplayScaleStrategy(DISPLAYSCALE_STRATEGY displayScaleStrategy)
{
  switch (displayScaleStrategy)
  {
    case DISPLAYSCALE_STRATEGY::DISPLAYSCALE_STRATEGY_SOLID: return DisplayScaleStrategy::Solid;
    case DISPLAYSCALE_STRATEGY::DISPLAYSCALE_STRATEGY_SCANLINES: return DisplayScaleStrategy::Scanlines;
    default: ThrowInvalidOption();
  }
}

DisplayDriver HostRenderConfigurationMapper::MapDisplayDriver(DISPLAYDRIVER displayDriver)
{
  switch (displayDriver)
  {
    case DISPLAYDRIVER::DISPLAYDRIVER_DIRECTDRAW: return DisplayDriver::DirectDraw;
    case DISPLAYDRIVER::DISPLAYDRIVER_DIRECT3D11: return DisplayDriver::Direct3D11;
    default: ThrowInvalidOption();
  }
}

RectShresi HostRenderConfigurationMapper::MapChipsetOutputClip(unsigned int outputClipLeft, unsigned int outputClipTop, unsigned int outputClipRight, unsigned int outputClipBottom)
{
  return RectShresi(outputClipLeft, outputClipTop, outputClipRight, outputClipBottom);
}

void HostRenderConfigurationMapper::ThrowInvalidOption()
{
  throw exception("Invalid configuration option");
}
