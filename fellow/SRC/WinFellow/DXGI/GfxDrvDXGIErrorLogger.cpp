#include "GfxDrvDXGIErrorLogger.h"
#include "DEFS.H"
#include "FELLOW.H"

const char* GfxDrvDXGIErrorLogger::GetErrorString(const HRESULT hResult)
{
  switch (hResult)
  {
    case DXGI_ERROR_DEVICE_HUNG:		  return "DXGI_ERROR_DEVICE_HUNG";
    case DXGI_ERROR_DEVICE_REMOVED:		  return "DXGI_ERROR_DEVICE_REMOVED";
    case DXGI_ERROR_DEVICE_RESET:		  return "DXGI_ERROR_DEVICE_RESET";
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR:	  return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
    case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:	  return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
    case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE: return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE";
    case DXGI_ERROR_INVALID_CALL:		  return "DXGI_ERROR_INVALID_CALL";
    case DXGI_ERROR_MORE_DATA:			  return "DXGI_ERROR_MORE_DATA";
    case DXGI_ERROR_NONEXCLUSIVE:		  return "DXGI_ERROR_NONEXCLUSIVE";
    case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:	  return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";
    case DXGI_ERROR_NOT_FOUND:			  return "DXGI_ERROR_NOT_FOUND";
    case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED:	  return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";
    case DXGI_ERROR_REMOTE_OUTOFMEMORY:		  return "DXGI_ERROR_REMOTE_OUTOFMEMORY";
    case DXGI_ERROR_WAS_STILL_DRAWING:		  return "DXGI_ERROR_WAS_STILL_DRAWING";
    case DXGI_ERROR_UNSUPPORTED:		  return "DXGI_ERROR_UNSUPPORTED";
    case DXGI_STATUS_OCCLUDED:			  return "DXGI_STATUS_OCCLUDED";
    case DXGI_STATUS_MODE_CHANGE_IN_PROGRESS:     return "DXGI_STATUS_MODE_CHANGE_IN_PROGRESS";
    case E_FAIL:                                  return "E_FAIL";
    case E_INVALIDARG:				  return "E_INVALIDARG";
    case S_OK:					  return "S_OK";
    case S_FALSE:                                 return "S_FALSE";
  }
  return "Unknown error";
}

void GfxDrvDXGIErrorLogger::LogError(const char* headline, const HRESULT hResult)
{
  fellowAddLog("%s %s\n", headline, GetErrorString(hResult));
}
