#include "OSServicesFactory.h"
#include "fellow/os/windows/timer/PerformanceCounterWin32.h"
#include "fellow/os/windows/io/FileopsWin32.h"

using namespace fellow::api;

namespace fellow::os
{
  static FileopsWin32 _fileops;
  static PerformanceCounterFactoryWin32 _performanceCounterFactory;

  IFileops &GetIFileopsImplementation()
  {
    return _fileops;
  }

  IPerformanceCounterFactory &GetIPerformanceCounterFactoryImplementation()
  {
    return _performanceCounterFactory;
  }
}