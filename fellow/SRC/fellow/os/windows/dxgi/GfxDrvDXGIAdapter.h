#pragma once

#include <DXGI.h>
#include <list>

#include "GfxDrvDXGIOutput.h"

class GfxDrvDXGIAdapter
{
private:
  char _name[255];
  GfxDrvDXGIOutputList _outputs;

  void EnumerateOutputs(IDXGIAdapter *adapter);
  void LogCapabilities(IDXGIAdapter *adapter);

public:
  const GfxDrvDXGIOutputList &GetOutputs();

  GfxDrvDXGIAdapter(IDXGIAdapter *adapter);
  virtual ~GfxDrvDXGIAdapter();
};

typedef std::list<GfxDrvDXGIAdapter *> GfxDrvDXGIAdapterList;
