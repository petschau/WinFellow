#ifndef GfxDrvDXGIAdapterEnumerator_H
#define GfxDrvDXGIAdapterEnumerator_H

#include <DXGI.h>
#include "GfxDrvDXGIAdapter.h"

class GfxDrvDXGIAdapterEnumerator
{
public:
  static GfxDrvDXGIAdapterList* EnumerateAdapters(IDXGIFactory *factory);
  static void DeleteAdapterList(GfxDrvDXGIAdapterList* adapters);
};

#endif
