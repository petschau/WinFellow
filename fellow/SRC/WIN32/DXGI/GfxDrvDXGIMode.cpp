#include "GfxDrvDXGIMode.h"
#include "DEFS.H"
#include "FELLOW.H"

unsigned int GfxDrvDXGIMode::_next_id = 1;

unsigned int GfxDrvDXGIMode::GetNewId()
{
  return _next_id++;
}

char *GfxDrvDXGIMode::GetScalingDescription()
{
  switch (GetScaling())
  {
    case DXGI_MODE_SCALING_UNSPECIFIED: return "UNSPECIFIED";
    case DXGI_MODE_SCALING_CENTERED: return "CENTERED";
    case DXGI_MODE_SCALING_STRETCHED: return "STRETCHED";
  }
  return "UNKNOWN SCALING";
}

char *GfxDrvDXGIMode::GetScanlineOrderDescription()
{
  switch (GetScanlineOrder())
  {
    case DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED: return "UNSPECIFIED";
    case DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE: return "PROGRESSIVE";
    case DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST: return "UPPER_FIELD_FIRST";
    case DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST: return "LOWER_FIELD_FIRST";
  }
  return "UNKNOWN SCANLINE ORDER";
}

void GfxDrvDXGIMode::LogCapabilities()
{
  fellowAddLog("DXGI mode (%d): %dx%dx%dhz - Scaling: %s Scanline order: %s\n", 
    GetId(), GetWidth(), GetHeight(), GetRefreshRate(), GetScalingDescription(), GetScanlineOrderDescription());
}

bool GfxDrvDXGIMode::CanUseMode()
{
  bool scanlineOrderOk = (GetScanlineOrder() == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED) || (GetScanlineOrder() == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE);
  bool refreshRateOk = (GetRefreshRate() == 0) || (GetRefreshRate() >= 50);
  bool sizeOk = (GetWidth() >= 640);

  return scanlineOrderOk && refreshRateOk && sizeOk;
}

GfxDrvDXGIMode::GfxDrvDXGIMode(DXGI_MODE_DESC *desc)
  : _dxgi_mode_description(*desc),
    _id(GetNewId())
{
  LogCapabilities();
}

GfxDrvDXGIMode::~GfxDrvDXGIMode()
{

}
