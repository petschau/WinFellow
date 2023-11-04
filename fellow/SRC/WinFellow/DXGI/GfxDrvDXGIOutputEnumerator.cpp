#include "GfxDrvDXGIOutputEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "Defs.h"
#include "FellowMain.h"
#include "VirtualHost/Core.h"

void GfxDrvDXGIOutputEnumerator::EnumerateOutputs(IDXGIAdapter *adapter, GfxDrvDXGIOutputList &outputs)
{
  IDXGIOutput *output;
  UINT i;
  for (i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i)
  {
    outputs.push_back(new GfxDrvDXGIOutput(output));
    output->Release();
  }
  if (i == 0)
  {
    _core.Log->AddLog("Device has no outputs.\n");
  }
}

void GfxDrvDXGIOutputEnumerator::DeleteOutputs(GfxDrvDXGIOutputList &outputs)
{
  for (GfxDrvDXGIOutputList::iterator i = outputs.begin(); i != outputs.end(); ++i)
  {
    delete *i;
  }
}
