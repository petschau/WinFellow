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

void GfxDrvDXGIOutput::LogCapabilities(void)
{
  DXGI_OUTPUT_DESC desc;
  HRESULT hr = _output->GetDesc(&desc);

  fellowAddLog("DXGI Output: %ls\n", desc.DeviceName);
  fellowAddLog("Attached to desktop: %s\n", (desc.AttachedToDesktop) ? "YES" : "NO");
  fellowAddLog("Desktop coordinates: (%d, %d) (%d, %d)\n", desc.DesktopCoordinates.left, desc.DesktopCoordinates.top, desc.DesktopCoordinates.right, desc.DesktopCoordinates.bottom);
  fellowAddLog("Rotation: %s\n\n", GetRotationDescription(desc.Rotation));
}

bool GfxDrvDXGIOutput::EnumerateModes(void)
{
  _modes = GfxDrvDXGIModeEnumerator::EnumerateModes(_output);
  return (_modes != 0);
}

GfxDrvDXGIOutput::GfxDrvDXGIOutput(IDXGIOutput *output)
  : _output(output)
{
  LogCapabilities();
  EnumerateModes();
}

GfxDrvDXGIOutput::~GfxDrvDXGIOutput()
{
  if (_modes != 0)
  {
    GfxDrvDXGIModeEnumerator::DeleteModeList(_modes);
    _modes = 0;
  }
  
  if (_output != 0)
  {
    _output->Release();
    _output = 0;
  }
}
