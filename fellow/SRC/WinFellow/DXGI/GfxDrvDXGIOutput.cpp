#include "GfxDrvDXGIOutput.h"
#include "GfxDrvDXGIModeEnumerator.h"
#include "DEFS.H"
#include "FELLOW.H"
#include "fellow/api/Services.h"

using namespace std;
using namespace fellow::api;

const char* GfxDrvDXGIOutput::GetRotationDescription(DXGI_MODE_ROTATION rotation)
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
  if (output)
  {
    DXGI_OUTPUT_DESC desc;
    HRESULT hr = output->GetDesc(&desc);

    if (SUCCEEDED(hr))
    {
      list<string> logmessages;
      char s[255];
      snprintf(_name, 255, "%ls", desc.DeviceName);

      sprintf(s,"DXGI Output: %s", _name);
      logmessages.emplace_back(s);
      sprintf(s, "Attached to desktop: %s", (desc.AttachedToDesktop) ? "YES" : "NO");
      logmessages.emplace_back(s);
      sprintf(s, "Desktop coordinates: (%ld, %ld) (%ld, %ld)", desc.DesktopCoordinates.left, desc.DesktopCoordinates.top, desc.DesktopCoordinates.right, desc.DesktopCoordinates.bottom);
      logmessages.emplace_back(s);
      sprintf(s, "Rotation: %s", GetRotationDescription(desc.Rotation));
      logmessages.emplace_back(s);
      Service->Log.AddLogList(logmessages);
    }
  }
}

void GfxDrvDXGIOutput::EnumerateModes(IDXGIOutput *output)
{
  GfxDrvDXGIModeEnumerator::EnumerateModes(output, _modes);
}

const GfxDrvDXGIModeList& GfxDrvDXGIOutput::GetModes()
{
  return _modes;
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
