#ifndef GfxDrvDXGIOutput_H
#define GfxDrvDXGIOutput_H

#include <DXGI.h>
#include <list>

#include "GfxDrvDXGIMode.h"

class GfxDrvDXGIOutput
{
private:
  IDXGIOutput* _output;
  GfxDrvDXGIModeList *_modes;

  char* GetRotationDescription(DXGI_MODE_ROTATION rotation);
  bool EnumerateModes(void);
  void LogCapabilities(void);

public:

  GfxDrvDXGIOutput(IDXGIOutput* output);
  virtual ~GfxDrvDXGIOutput();
};

typedef std::list<GfxDrvDXGIOutput*> GfxDrvDXGIOutputList;

#endif
