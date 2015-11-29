#ifndef GfxDrvDXGIModeEnumerator_H
#define GfxDrvDXGIModeEnumerator_H

#include "GfxDrvDXGIMode.h"

class GfxDrvDXGIModeEnumerator
{
public:
  static GfxDrvDXGIModeList* EnumerateModes(IDXGIOutput *output);
  static void DeleteModeList(GfxDrvDXGIModeList* modes);
};

#endif
