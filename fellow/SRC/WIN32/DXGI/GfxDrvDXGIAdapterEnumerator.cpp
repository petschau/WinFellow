#include "GfxDrvDXGIAdapterEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "DEFS.H"
#include "FELLOW.H"

GfxDrvDXGIAdapterList* GfxDrvDXGIAdapterEnumerator::EnumerateAdapters(IDXGIFactory *factory)
{
  fellowAddLog("GfxDrvDXGI: Enumerating adapters starting\n");

  GfxDrvDXGIAdapterList *adapters = new GfxDrvDXGIAdapterList();
  IDXGIAdapter *adapter;
  UINT i;
  for (i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) 
  { 
    adapters->push_back(new GfxDrvDXGIAdapter(adapter));
    adapter->Release();
  }

  if (i == 0)
  {
    fellowAddLog("No adapters found!\n");
  }

  fellowAddLog("GfxDrvDXGI: Enumerating adapters finished\n");	

  return adapters;
}

void GfxDrvDXGIAdapterEnumerator::DeleteAdapterList(GfxDrvDXGIAdapterList *adapters)
{
  if (adapters != 0)
  {
    for (GfxDrvDXGIAdapterList::iterator i = adapters->begin(); i != adapters->end(); ++i)
    {
      delete *i;
    }
    delete adapters;
  }
}
