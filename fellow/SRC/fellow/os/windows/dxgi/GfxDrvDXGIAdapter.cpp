#include "GfxDrvDXGIAdapter.h"
#include "GfxDrvDXGIOutputEnumerator.h"
#include "fellow/api/defs.h"
#include <list>
#include "fellow/api/Services.h"

using namespace std;
using namespace fellow::api;

void GfxDrvDXGIAdapter::LogCapabilities(IDXGIAdapter *adapter)
{
  DXGI_ADAPTER_DESC desc{};
  HRESULT hr = adapter->GetDesc(&desc);

  if (!SUCCEEDED(hr))
  {
    Service->Log.AddLog("GfxDrvDXGIAdapter::LogCapabilities() ERROR Failed to GetDesc()");
    return;
  }

  list<string> messages;
  char s[512];

  snprintf(s, 255, "DXGI adapter name: %ls", desc.Description);
  messages.emplace_back(s);
  sprintf(s, "Vendor ID: %.4X", desc.VendorId);
  messages.emplace_back(s);
  sprintf(s, "Device ID: %.4X", desc.DeviceId);
  messages.emplace_back(s);
  sprintf(s, "Subsys ID: %.4X", desc.SubSysId);
  messages.emplace_back(s);
  sprintf(s, "Revision:  %.4X", desc.Revision);
  messages.emplace_back(s);
  sprintf(s, "Dedicated system memory: %I64d", (__int64)desc.DedicatedSystemMemory);
  messages.emplace_back(s);
  sprintf(s, "Dedicated video memory:  %I64d", (__int64)desc.DedicatedVideoMemory);
  messages.emplace_back(s);
  sprintf(s, "Shared system memory:    %I64d", (__int64)desc.SharedSystemMemory);
  messages.emplace_back(s);

  Service->Log.AddLogList(messages);
}

void GfxDrvDXGIAdapter::EnumerateOutputs(IDXGIAdapter *adapter)
{
  GfxDrvDXGIOutputEnumerator::EnumerateOutputs(adapter, _outputs);
}

const GfxDrvDXGIOutputList &GfxDrvDXGIAdapter::GetOutputs()
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
