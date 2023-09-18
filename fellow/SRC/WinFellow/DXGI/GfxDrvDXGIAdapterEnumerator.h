#pragma once

#include <DXGI.h>
#include "GfxDrvDXGIAdapter.h"

class GfxDrvDXGIAdapterEnumerator
{
public:
  static GfxDrvDXGIAdapterList* EnumerateAdapters(IDXGIFactory *factory);
  static void DeleteAdapterList(GfxDrvDXGIAdapterList* adapters);
};
