#include "GfxDrvDXGIOutputEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "DEFS.H"
#include "FELLOW.H"

GfxDrvDXGIOutputList* GfxDrvDXGIOutputEnumerator::EnumerateOutputs(IDXGIAdapter *adapter)
{
  GfxDrvDXGIOutputList *outputs = new GfxDrvDXGIOutputList();
  IDXGIOutput *output;
  UINT i;
  for (i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i) 
  { 
    outputs->push_back(new GfxDrvDXGIOutput(output));
  }
  if (i == 0)
  {
    fellowAddLog("Device has no outputs.\n");
  }

  return outputs;
}

void GfxDrvDXGIOutputEnumerator::DeleteOutputList(GfxDrvDXGIOutputList* outputs)
{
  if (outputs != 0)
  {
    for (GfxDrvDXGIOutputList::iterator i = outputs->begin(); i != outputs->end(); ++i)
    {
      delete *i;
    }
    delete outputs;
  }
}
