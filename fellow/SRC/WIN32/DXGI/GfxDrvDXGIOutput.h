#ifndef GfxDrvDXGIOutput_H
#define GfxDrvDXGIOutput_H

#include <DXGI.h>
#include <list>

#include "GfxDrvDXGIMode.h"

class GfxDrvDXGIOutput
{
private:
  char _name[255];
  GfxDrvDXGIModeList _modes;

  const char* GetRotationDescription(DXGI_MODE_ROTATION rotation);
  void EnumerateModes(IDXGIOutput* output);
  void LogCapabilities(IDXGIOutput* output);

public:
  const GfxDrvDXGIModeList& GetModes();

  GfxDrvDXGIOutput(IDXGIOutput* output);
  virtual ~GfxDrvDXGIOutput();
};

typedef std::list<GfxDrvDXGIOutput*> GfxDrvDXGIOutputList;

#endif
