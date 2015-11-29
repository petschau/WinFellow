#include "GfxDrvDXGIAdapter.h"
#include "GfxDrvDXGIOutputEnumerator.h"
#include "DEFS.H"
#include "FELLOW.H"

void GfxDrvDXGIAdapter::LogCapabilities()
{
  DXGI_ADAPTER_DESC desc;
  HRESULT hr = _adapter->GetDesc(&desc);

  fellowAddLog("DXGI Adapter: %ls\n", desc.Description);
  fellowAddLog("Vendor ID: %.4X\n", desc.VendorId);
  fellowAddLog("Device ID: %.4X\n", desc.DeviceId);
  fellowAddLog("Subsys ID: %.4X\n", desc.SubSysId);
  fellowAddLog("Revision:  %.4X\n", desc.Revision);
  fellowAddLog("Dedicated system memory: %I64d\n", (__int64) desc.DedicatedSystemMemory);
  fellowAddLog("Dedicated video memory:  %I64d\n", (__int64) desc.DedicatedVideoMemory);
  fellowAddLog("Shared system memory:    %I64d\n", (__int64) desc.SharedSystemMemory);
}

bool GfxDrvDXGIAdapter::EnumerateOutputs()
{
  _outputs = GfxDrvDXGIOutputEnumerator::EnumerateOutputs(_adapter);
  return _outputs != 0;
}

GfxDrvDXGIAdapter::GfxDrvDXGIAdapter(IDXGIAdapter *adapter)
  : _adapter(adapter)
{
  LogCapabilities();
  EnumerateOutputs();
}

GfxDrvDXGIAdapter::~GfxDrvDXGIAdapter()
{
  if (_outputs != 0)
  {
    GfxDrvDXGIOutputEnumerator::DeleteOutputList(_outputs);
    _outputs = 0;
  }
  
  if (_adapter != 0)
  {
    _adapter->Release();
    _adapter = 0;
  }
}
