#pragma once

#include "GfxDrvDXGIOutput.h"

class GfxDrvDXGIOutputEnumerator
{
public:
  static void EnumerateOutputs(IDXGIAdapter *adapter, GfxDrvDXGIOutputList &outputs);
  static void DeleteOutputs(GfxDrvDXGIOutputList &outputs);
};
