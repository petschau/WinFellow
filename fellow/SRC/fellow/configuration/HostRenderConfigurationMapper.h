#pragma once

#include "fellow/configuration/Configuration.h"
#include "fellow/application/HostRenderConfiguration.h"

class HostRenderConfigurationMapper
{
public:
  static HostRenderConfiguration Map(cfg *cfg);

private:
  static DimensionsUInt MapDisplaySize(unsigned int displayWidth, unsigned int displayHeight);
  static DisplayOutputKind MapDisplayOutputKind(bool isScreenWindowed);
  static DisplayColorDepth MapDisplayColorDepth(bool isScreenWindowed, unsigned int screenColorBits);
  static unsigned int MapDisplayRefreshRate(bool isScreenWindowed, unsigned int screenRefresh);
  static DisplayScale MapDisplayScale(DISPLAYSCALE displayScale);
  static DisplayScaleStrategy MapDisplayScaleStrategy(DISPLAYSCALE_STRATEGY displayScaleStrategy);
  static DisplayDriver MapDisplayDriver(DISPLAYDRIVER displayDriver);
  static RectShresi MapChipsetOutputClip(unsigned int outputClipLeft, unsigned int outputClipTop, unsigned int outputClipRight, unsigned int outputClipBottom);

  static void ThrowInvalidOption();
};

class InvalidOptionException : public std::exception
{
public:
    InvalidOptionException() = default;
};
