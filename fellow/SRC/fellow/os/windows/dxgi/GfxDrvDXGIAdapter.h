#pragma once

#include <DXGI.h>
#include <list>

#include "GfxDrvDXGIOutput.h"

class GfxDrvDXGIAdapter
{
private:
  GfxDrvDXGIOutputList _outputs;

  void EnumerateOutputs(IDXGIAdapter *adapter);

public:
  static void LogCapabilities(IDXGIAdapter *adapter);

  const GfxDrvDXGIOutputList &GetOutputs();

  GfxDrvDXGIAdapter(IDXGIAdapter *adapter);
  virtual ~GfxDrvDXGIAdapter();
};

typedef std::list<GfxDrvDXGIAdapter *> GfxDrvDXGIAdapterList;
