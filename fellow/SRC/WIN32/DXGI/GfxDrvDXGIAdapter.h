#ifndef GfxDrvDXGIAdapter_H
#define GfxDrvDXGIAdapter_H

#include <DXGI.h>
#include <list>

#include "GfxDrvDXGIOutput.h"

class GfxDrvDXGIAdapter
{
private:
  IDXGIAdapter* _adapter;
  GfxDrvDXGIOutputList* _outputs;

public:
  bool EnumerateOutputs(void);
  void LogCapabilities(void);
  IDXGIAdapter *GetDXGIAdapter() {return _adapter;}

  GfxDrvDXGIAdapter(IDXGIAdapter* adapter);
  virtual ~GfxDrvDXGIAdapter();
};

typedef std::list<GfxDrvDXGIAdapter*> GfxDrvDXGIAdapterList;

#endif
