#include "Defs.h"

#include "Logger.h"
#include "BusScheduler.h"

#include "VirtualHost/Core.h"

void Logger::Log(uint32_t line, uint32_t cylinder, const char *message)
{
  if (_enableLog)
  {
    if (_logfile == nullptr)
    {
      char filename[MAX_PATH];
      _core.Fileops->GetGenericFileName(filename, "WinFellow", "Graphics.log");
      _logfile = fopen(filename, "w");
    }
    fprintf(_logfile, "Frame %.16I64X Line %.3X Cylinder %.3X (%.3X,%.3X): %s", busGetRasterFrameCount(), line, cylinder, busGetRasterY(), busGetRasterX(), message);
  }
}

void Logger::Shutdown()
{
  if (_logfile != nullptr)
  {
    fclose(_logfile);
    _logfile = nullptr;
  }
}
