#ifndef GfxDrvDXGIModeEnumerator_H
#define GfxDrvDXGIModeEnumerator_H

#include "GfxDrvDXGIMode.h"

class GfxDrvDXGIModeEnumerator
{
public:
  static void EnumerateModes(IDXGIOutput *output, GfxDrvDXGIModeList& modes);
  static void DeleteModeList(GfxDrvDXGIModeList& modes);
};

#endif
