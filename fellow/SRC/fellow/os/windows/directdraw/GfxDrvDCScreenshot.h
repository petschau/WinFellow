#pragma once

#include <Windows.h>

class GfxDrvDCScreenshot
{
public:
  static bool SaveScreenshotFromDCArea(HDC hDC, DWORD x, DWORD y, DWORD width, DWORD height, unsigned int lDisplayScale, DWORD bits, const char *filename);
};
