#include "DEFS.H"

#include "Logger.h"
#include "fileops.h"
#include "BUS.H"

void Logger::Log(ULO line, ULO cylinder, STR *message)
{
  if (_enableLog)
  {
    if (_logfile == 0)
    {
      STR filename[MAX_PATH];
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
  if (_logfile != 0)
  {
    fclose(_logfile);
    _logfile = 0;
  }
}
