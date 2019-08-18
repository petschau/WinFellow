#pragma once
#include <ddraw.h>

class DirectDrawErrorLogger
{
private:
  static const char *GetErrorString(HRESULT hResult);

public:
  static void LogError(const char *header, HRESULT err);
};