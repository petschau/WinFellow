#include <stdio.h>

#include "GfxDrvDCScreenshot.h"
#include "fellow/api/Services.h"

using namespace fellow::api;

bool GfxDrvDCScreenshot::SaveScreenshotFromDCArea(HDC hDC, DWORD x, DWORD y, DWORD width, DWORD height, unsigned int lDisplayScale, DWORD bits, const char *filename)
{
  BITMAPFILEHEADER bfh;
  BITMAPINFOHEADER bih;
  HDC memDC = nullptr;
  HGDIOBJ oldbit = nullptr;
  FILE *file = nullptr;
  void *data = nullptr;
  bool bSuccess = FALSE;

  if (!hDC) return 0;
  if (bits <= 0) return 0;
  if (width <= 0) return 0;
  if (height <= 0) return 0;

  int bpp = bits / 8;
  if (bpp < 2) bpp = 2;
  if (bpp > 3) bpp = 3;

  int datasize = width * bpp * height;
  if (width * bpp % 4)
  {
    datasize += height * (4 - width * bpp % 4);
  }

  memset((void *)&bfh, 0, sizeof(bfh));

  bfh.bfType = 'B' + ('M' << 8);
  bfh.bfSize = sizeof(bfh) + sizeof(bih) + datasize;
  bfh.bfOffBits = sizeof(bfh) + sizeof(bih);

  memset((void *)&bih, 0, sizeof(bih));
  bih.biSize = sizeof(bih);
  bih.biWidth = width;
  bih.biHeight = height;
  bih.biPlanes = 1;
  bih.biBitCount = bpp * 8;
  bih.biCompression = BI_RGB;

  HGDIOBJ bitmap = CreateDIBSection(nullptr, (BITMAPINFO *)&bih, DIB_RGB_COLORS, &data, nullptr, 0);

  if (!bitmap) goto cleanup;
  if (!data) goto cleanup;

  memDC = CreateCompatibleDC(hDC);
  if (!memDC) goto cleanup;

  oldbit = SelectObject(memDC, bitmap);
  if (oldbit == nullptr || oldbit == HGDI_ERROR) goto cleanup;

  bSuccess = StretchBlt(memDC, 0, 0, width, height, hDC, x, y, width / lDisplayScale, height / lDisplayScale, SRCCOPY) ? true : false;
  if (!bSuccess) goto cleanup;

  file = fopen(filename, "wb");
  if (!file) goto cleanup;

  fwrite((void *)&bfh, sizeof(bfh), 1, file);
  fwrite((void *)&bih, sizeof(bih), 1, file);
  fwrite((void *)data, 1, datasize, file);

  bSuccess = TRUE;

cleanup:
  if (oldbit != nullptr && oldbit != HGDI_ERROR) SelectObject(memDC, oldbit);
  if (memDC) DeleteDC(memDC);
  if (file) fclose(file);
  if (bitmap) DeleteObject(bitmap);

  Service->Log.AddLogDebug("gfxDrvDDrawSaveScreenshotFromDCArea(hDC=0x%x, width=%d, height=%d, bits=%d, filename='%s' %s.\n", hDC, width, height, bits, filename, bSuccess ? "successful" : "failed");

  return bSuccess;
}
