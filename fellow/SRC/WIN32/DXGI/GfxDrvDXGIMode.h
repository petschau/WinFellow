#ifndef GfxDrvDXGIMode_H
#define GfxDrvDXGIMode_H

#include <DXGI.h>
#include <list>

class GfxDrvDXGIMode
{
private:
  int _width;
  int _height;
  int _refresh;

  void LogCapabilities(DXGI_MODE_DESC *desc);
  char *GetScalingDescription(DXGI_MODE_SCALING scaling);
  char *GetScanlineOrderDescription(DXGI_MODE_SCANLINE_ORDER scanline_order);

public:
  GfxDrvDXGIMode(DXGI_MODE_DESC *desc);
};

typedef std::list<GfxDrvDXGIMode*> GfxDrvDXGIModeList;

#endif
