#include "GfxDrvDXGIOutput.h"
#include "GfxDrvDXGIModeEnumerator.h"
#include "DEFS.H"
#include "FELLOW.H"

char* GfxDrvDXGIOutput::GetRotationDescription(DXGI_MODE_ROTATION rotation)
{
  switch (rotation)
  {
    case DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_IDENTITY:
      return "DXGI_MODE_ROTATION_IDENTITY";
    case DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_ROTATE90:
      return "DXGI_MODE_ROTATION_ROTATE90";
    case DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_ROTATE180:
      return "DXGI_MODE_ROTATION_ROTATE180";
    case DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_ROTATE270:
      return "DXGI_MODE_ROTATION_ROTATE270";
    case DXGI_MODE_ROTATION::DXGI_MODE_ROTATION_UNSPECIFIED:
      return "DXGI_MODE_ROTATION_UNSPECIFIED";
  }
  return "UNKNOWN ROTATION";
}

void GfxDrvDXGIOutput::LogCapabilities(IDXGIOutput *output)
{
  DXGI_OUTPUT_DESC desc;
  HRESULT hr = output->GetDesc(&desc);

  sprintf(_name, "%254ls", desc.DeviceName);
  
  fellowAddLog("DXGI Output: %ls\n", desc.DeviceName);
  fellowAddLog("Attached to desktop: %s\n", (desc.AttachedToDesktop) ? "YES" : "NO");
  fellowAddLog("Desktop coordinates: (%d, %d) (%d, %d)\n", desc.DesktopCoordinates.left, desc.DesktopCoordinates.top, desc.DesktopCoordinates.right, desc.DesktopCoordinates.bottom);
  fellowAddLog("Rotation: %s\n\n", GetRotationDescription(desc.Rotation));
}

void GfxDrvDXGIOutput::EnumerateModes(IDXGIOutput *output)
{
  GfxDrvDXGIModeEnumerator::EnumerateModes(output, _modes);
}

GfxDrvDXGIOutput::GfxDrvDXGIOutput(IDXGIOutput *output)
{
  LogCapabilities(output);
  EnumerateModes(output);
}

GfxDrvDXGIOutput::~GfxDrvDXGIOutput()
{
  GfxDrvDXGIModeEnumerator::DeleteModeList(_modes);
}
