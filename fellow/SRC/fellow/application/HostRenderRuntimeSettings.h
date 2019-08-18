#pragma once

#include "fellow/application/DisplayMode.h"

struct HostRenderRuntimeSettings
{
  DisplayMode DisplayMode;        // Current displaymode selected for this session
  unsigned int OutputScaleFactor; // Scale factor of the host image

  void Log();
};
