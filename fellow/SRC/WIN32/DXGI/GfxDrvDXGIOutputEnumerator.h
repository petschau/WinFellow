#ifndef GfxDrvDXGIOutputEnumerator_H
#define GfxDrvDXGIOutputEnumerator_H

#include "GfxDrvDXGIOutput.h"

class GfxDrvDXGIOutputEnumerator
{
public:
  static GfxDrvDXGIOutputList* EnumerateOutputs(IDXGIAdapter *adapter);
  static void DeleteOutputList(GfxDrvDXGIOutputList* outputs);
};

#endif
