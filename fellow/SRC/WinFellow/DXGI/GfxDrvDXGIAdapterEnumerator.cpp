#include "GfxDrvDXGIAdapterEnumerator.h"
#include "GfxDrvDXGIErrorLogger.h"
#include "Defs.h"
#include "FellowMain.h"
#include "VirtualHost/Core.h"

GfxDrvDXGIAdapterList *GfxDrvDXGIAdapterEnumerator::EnumerateAdapters(IDXGIFactory *factory)
{
  _core.Log->AddLog("GfxDrvDXGI: Enumerating adapters starting\n");

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
    _core.Log->AddLog("No adapters found!\n");
  }

  _core.Log->AddLog("GfxDrvDXGI: Enumerating adapters finished\n");

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
