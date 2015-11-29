#ifndef GfxDrvDXGIErrorLogger_H
#define GfxDrvDXGIErrorLogger_H

#include <DXGI.h>
#include <string>

class GfxDrvDXGIErrorLogger
{
private:
  static const char* GetErrorString(const HRESULT hResult);

public:
  static void LogError(const char* headline, const HRESULT hResult);
};

#endif
