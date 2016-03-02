#include "GfxDrvDXGIAdapter.h"
#include "GfxDrvDXGIOutputEnumerator.h"
#include "DEFS.H"
#include "FELLOW.H"

void GfxDrvDXGIAdapter::LogCapabilities(IDXGIAdapter *adapter)
{
  DXGI_ADAPTER_DESC desc;
  HRESULT hr = adapter->GetDesc(&desc);

  if (SUCCEEDED(hr))
  {
    sprintf(_name, "%254ls", desc.Description);

    fellowAddLog("DXGI Adapter: %ls\n", desc.Description);
    fellowAddLog("Vendor ID: %.4X\n", desc.VendorId);
    fellowAddLog("Device ID: %.4X\n", desc.DeviceId);
    fellowAddLog("Subsys ID: %.4X\n", desc.SubSysId);
    fellowAddLog("Revision:  %.4X\n", desc.Revision);
    fellowAddLog("Dedicated system memory: %I64d\n", (__int64)desc.DedicatedSystemMemory);
    fellowAddLog("Dedicated video memory:  %I64d\n", (__int64)desc.DedicatedVideoMemory);
    fellowAddLog("Shared system memory:    %I64d\n", (__int64)desc.SharedSystemMemory);
  }
}

void GfxDrvDXGIAdapter::EnumerateOutputs(IDXGIAdapter *adapter)
{
  GfxDrvDXGIOutputEnumerator::EnumerateOutputs(adapter, _outputs);
}

const GfxDrvDXGIOutputList& GfxDrvDXGIAdapter::GetOutputs()
{
  return _outputs;
}

GfxDrvDXGIAdapter::GfxDrvDXGIAdapter(IDXGIAdapter *adapter)
{
  LogCapabilities(adapter);
  EnumerateOutputs(adapter);
}

GfxDrvDXGIAdapter::~GfxDrvDXGIAdapter()
{
  GfxDrvDXGIOutputEnumerator::DeleteOutputs(_outputs);
}
