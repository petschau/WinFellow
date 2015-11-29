#include "GfxDrvDXGIModeEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "DEFS.H"
#include "FELLOW.H"

GfxDrvDXGIModeList* GfxDrvDXGIModeEnumerator::EnumerateModes(IDXGIOutput *output)
{
  GfxDrvDXGIModeList *modes = new GfxDrvDXGIModeList();

  DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
  UINT numModes = 0;
  
  HRESULT hr = output->GetDisplayModeList(format, 0, &numModes, NULL);

  fellowAddLog("Output had %d modes.\n", numModes);
  return modes;
}

void GfxDrvDXGIModeEnumerator::DeleteModeList(GfxDrvDXGIModeList* modes)
{
  if (modes != 0)
  {
    for (GfxDrvDXGIModeList::iterator i = modes->begin(); i != modes->end(); ++i)
    {
      delete *i;
    }
    delete modes;
  }
}
