#include "GfxDrvDXGIMode.h"
#include "sstream"

using namespace std;
unsigned int GfxDrvDXGIMode::_next_id = 1;

unsigned int GfxDrvDXGIMode::GetNewId()
{
  return _next_id++;
}

const char *GfxDrvDXGIMode::GetScalingDescription()
{
  switch (GetScaling())
  {
    case DXGI_MODE_SCALING_UNSPECIFIED: return "UNSPECIFIED";
    case DXGI_MODE_SCALING_CENTERED: return "CENTERED";
    case DXGI_MODE_SCALING_STRETCHED: return "STRETCHED";
  }
  return "UNKNOWN SCALING";
}

const char *GfxDrvDXGIMode::GetScanlineOrderDescription()
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

string GfxDrvDXGIMode::GetModeDescriptionString()
{
  std::ostringstream ss;
  ss << "DXGI mode (" << GetId() << "): " << GetWidth() << "x" << GetHeight() << "x" << GetRefreshRate() << " - Scaling: " << GetScalingDescription()
     << " Scanline order: " << GetScanlineOrderDescription();
  return ss.str();
}

bool GfxDrvDXGIMode::CanUseMode()
{
  bool scanlineOrderOk = (GetScanlineOrder() == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED) || (GetScanlineOrder() == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE);
  bool refreshRateOk = (GetRefreshRate() == 0) || (GetRefreshRate() >= 50);
  bool sizeOk = (GetWidth() >= 640);

  return scanlineOrderOk && refreshRateOk && sizeOk;
}

GfxDrvDXGIMode::GfxDrvDXGIMode(DXGI_MODE_DESC *desc) : _id(GetNewId()), _dxgi_mode_description(*desc)
{
}
