#include "GfxDrvDXGIModeEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "Defs.h"
#include "FELLOW.H"
#include "VirtualHost/Core.h"
#include <sstream>

using namespace std;

void GfxDrvDXGIModeEnumerator::EnumerateModes(IDXGIOutput *output, GfxDrvDXGIModeList &modes)
{
  DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
  UINT flags = 0;
  UINT numModes = 0;

  HRESULT hr = output->GetDisplayModeList(format, flags, &numModes, nullptr);

  if (FAILED(hr))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIModeEnumerator::EnumerateModes(): Failed to get display mode list.", hr);
    return;
  }

  _core.Log->AddLog("Output has %d modes.\n", numModes);

  DXGI_MODE_DESC *descs = new DXGI_MODE_DESC[numModes];
  hr = output->GetDisplayModeList(format, flags, &numModes, descs);

  if (FAILED(hr))
  {
    GfxDrvDXGIErrorLogger::LogError("GfxDrvDXGIModeEnumerator::EnumerateModes(): Failed to get display mode list.", hr);
    return;
  }

  list<string> loglines;
  for (UINT i = 0; i < numModes; i++)
  {
    auto mode = new GfxDrvDXGIMode(&descs[i]);
    modes.push_back(mode);

    loglines.emplace_back(mode->GetModeDescriptionString());
  }
  if (numModes > 0)
  {
    _core.Log->AddLogList(loglines);
  }
  delete[] descs;
}

void GfxDrvDXGIModeEnumerator::DeleteModeList(GfxDrvDXGIModeList &modes)
{
  for (GfxDrvDXGIModeList::iterator i = modes.begin(); i != modes.end(); ++i)
  {
    delete *i;
  }
}
