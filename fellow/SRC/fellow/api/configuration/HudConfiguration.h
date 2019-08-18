#pragma once

class HudConfiguration
{
public:
  bool FPSEnabled;
  bool LEDEnabled;

  HudConfiguration(bool FPSEnabled, bool LEDEnabled) noexcept : FPSEnabled(FPSEnabled), LEDEnabled(LEDEnabled)
  {
  }

  HudConfiguration() noexcept : FPSEnabled(false), LEDEnabled(false)
  {
  }
};
