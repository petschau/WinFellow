#pragma once

#include "d3d11.h"

template <typename T> void ReleaseCOM(T **comobject)
{
  if (*comobject != nullptr)
  {
    (*comobject)->Release();
    *comobject = nullptr;
  }
}

static void GfxDrvDXGISetDebugName(ID3D11DeviceChild *child, const char *name)
{
  if (child != nullptr && name != nullptr)
  {
    child->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name), name);
  }
}
