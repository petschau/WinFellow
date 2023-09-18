#include "DEFS.H"

#include "Logger.h"
#include "fileops.h"
#include "BUS.H"

void Logger::Log(uint32_t line, uint32_t cylinder, char *message)
{
  if (_enableLog)
  {
    if (_logfile == nullptr)
    {
      char filename[MAX_PATH];
      fileopsGetGenericFileName(filename, "WinFellow", "Graphics.log");
      _logfile = fopen(filename, "w");
    }
    fprintf(_logfile, 
            "Frame %.16I64X Line %.3X Cylinder %.3X (%.3X,%.3X): %s",
            busGetRasterFrameCount(),
            line,
            cylinder,
            busGetRasterY(),
            busGetRasterX(),
            message);
  }
}

void Logger::Shutdown(void)
{
  if (_logfile != nullptr)
  {
    fclose(_logfile);
    _logfile = nullptr;
  }
}
