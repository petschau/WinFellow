#include "fellow/application/HostRenderRuntimeSettings.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

void HostRenderRuntimeSettings::Log()
{
  Service->Log.AddLog("HostRenderRuntimeSettings - Selected driver display mode is %s\n", DisplayMode.ToString().c_str());
  Service->Log.AddLog("HostRenderRuntimeSettings - Output scale factor is %u\n", OutputScaleFactor);
}
