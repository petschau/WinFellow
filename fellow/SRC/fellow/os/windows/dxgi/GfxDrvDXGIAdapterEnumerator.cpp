#include "GfxDrvDXGIAdapterEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "fellow/api/defs.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

GfxDrvDXGIAdapterList *GfxDrvDXGIAdapterEnumerator::EnumerateAdapters(IDXGIFactory *factory)
{
  Service->Log.AddLog("GfxDrvDXGI: Enumerating adapters starting\n");

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
    Service->Log.AddLog("No adapters found!\n");
  }

  Service->Log.AddLog("GfxDrvDXGI: Enumerating adapters finished\n");

  return adapters;
}

void GfxDrvDXGIAdapterEnumerator::DeleteAdapterList(GfxDrvDXGIAdapterList *adapters)
{
  if (adapters != nullptr)
  {
    for (GfxDrvDXGIAdapterList::iterator i = adapters->begin(); i != adapters->end(); ++i)
    {
      delete *i;
    }
    delete adapters;
  }
}
