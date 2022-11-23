#include "fellow/os/null/GraphicsDriver.h"

using namespace std;

void GraphicsDriver::ClearCurrentBuffer()
{
}

void GraphicsDriver::BufferFlip()
{
}

void GraphicsDriver::SizeChanged(unsigned int width, unsigned int height)
{
}

void GraphicsDriver::PositionChanged()
{
}

GfxDrvMappedBufferPointer GraphicsDriver::MapChipsetFramebuffer()
{
  return GfxDrvMappedBufferPointer
  {
    .IsValid = false
  };
}

void GraphicsDriver::UnmapChipsetFramebuffer()
{
}

GfxDrvColorBitsInformation GraphicsDriver::GetColorBitsInformation()
{
  return GfxDrvColorBitsInformation();
}

list<DisplayMode> GraphicsDriver::GetDisplayModeList()
{
  return list<DisplayMode>();
}

void GraphicsDriver::DrawHUD(const MappedChipsetFramebuffer &mappedFramebuffer)
{
}

void GraphicsDriver::NotifyActiveStatus(bool active)
{
}

bool GraphicsDriver::SaveScreenshot(const bool, const char *filename)
{
  return true;
}

bool GraphicsDriver::ValidateRequirements()
{
  return true;
}

bool GraphicsDriver::EmulationStart(
const HostRenderConfiguration &hostRenderConfiguration,
const HostRenderRuntimeSettings &hostRenderRuntimeSettings,
const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings,
HudPropertyProvider *hudPropertyProvider)
{
  return true;
}

ULO GraphicsDriver::EmulationStartPost(const ChipsetBufferRuntimeSettings &chipsetBufferRuntimeSettings)
{
  return 1;
}

void GraphicsDriver::EmulationStop()
{
}

bool GraphicsDriver::Startup(DISPLAYDRIVER displaydriver)
{
  return true;
}

void GraphicsDriver::Shutdown()
{
}
