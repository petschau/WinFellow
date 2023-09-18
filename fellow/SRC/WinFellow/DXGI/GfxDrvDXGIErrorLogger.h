#pragma once

#include <DXGI.h>

class GfxDrvDXGIErrorLogger
{
private:
  static const char* GetErrorString(const HRESULT hResult);

public:
  static void LogError(const char* headline, const HRESULT hResult);
};
