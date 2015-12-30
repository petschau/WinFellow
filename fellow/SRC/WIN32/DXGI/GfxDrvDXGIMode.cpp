#include "GfxDrvDXGIMode.h"
#include "DEFS.H"
#include "FELLOW.H"

char *GfxDrvDXGIMode::GetScalingDescription(DXGI_MODE_SCALING scaling)
{
  switch (scaling)
  {
    case DXGI_MODE_SCALING_UNSPECIFIED: return "UNSPECIFIED";
    case DXGI_MODE_SCALING_CENTERED: return "CENTERED";
    case DXGI_MODE_SCALING_STRETCHED: return "STRETCHED";
  }
  return "UNKNOWN SCALING";
}

char *GfxDrvDXGIMode::GetScanlineOrderDescription(DXGI_MODE_SCANLINE_ORDER scanline_order)
{
  switch (scanline_order)
  {
    case DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED: return "UNSPECIFIED";
    case DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE: return "PROGRESSIVE";
    case DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST: return "UPPER_FIELD_FIRST";
    case DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST: return "LOWER_FIELD_FIRST";
  }
  return "UNKNOWN SCANLINE ORDER";
}

void GfxDrvDXGIMode::LogCapabilities(DXGI_MODE_DESC *desc)
{
  if (desc)
  {
    _width = desc->Width;
    _height = desc->Height;
    _refresh = desc->RefreshRate.Numerator / desc->RefreshRate.Denominator;

    fellowAddLog("DXGI mode: %dx%dx%dhz - Scaling: %s Scanline order: %s\n", _width, _height, _refresh, GetScalingDescription(desc->Scaling), GetScanlineOrderDescription(desc->ScanlineOrdering));
  }
}

GfxDrvDXGIMode::GfxDrvDXGIMode(DXGI_MODE_DESC *desc)
{
  LogCapabilities(desc);
}
