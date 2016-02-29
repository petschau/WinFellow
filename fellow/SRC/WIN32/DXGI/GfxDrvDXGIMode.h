#ifndef GfxDrvDXGIMode_H
#define GfxDrvDXGIMode_H

#include <DXGI.h>
#include <list>

class GfxDrvDXGIMode
{
private:
  static unsigned int _next_id;
  unsigned int _id;
  DXGI_MODE_DESC _dxgi_mode_description;

  void LogCapabilities();
  char *GetScalingDescription();
  char *GetScanlineOrderDescription();

public:
  unsigned int GetId() { return _id; }
  unsigned int GetWidth() { return _dxgi_mode_description.Width; }
  unsigned int GetHeight() { return _dxgi_mode_description.Height; }
  unsigned int GetRefreshRate() { return _dxgi_mode_description.RefreshRate.Numerator / _dxgi_mode_description.RefreshRate.Denominator; }
  DXGI_MODE_SCALING GetScaling() { return _dxgi_mode_description.Scaling; }
  DXGI_MODE_SCANLINE_ORDER GetScanlineOrder() { return _dxgi_mode_description.ScanlineOrdering; }
  DXGI_MODE_DESC *GetDXGIModeDescription() { return &_dxgi_mode_description; }

  static unsigned int GetNewId();
  bool CanUseMode();

  GfxDrvDXGIMode(DXGI_MODE_DESC *desc);
  ~GfxDrvDXGIMode();
};

typedef std::list<GfxDrvDXGIMode*> GfxDrvDXGIModeList;

#endif
