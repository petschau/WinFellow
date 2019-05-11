#include <time.h>
#include <string>
#include "fellow/service/Log.h"
#include "fileops.h"

#define WRITE_LOG_BUF_SIZE 512

namespace fellow::service
{
  Log::Log() :
    _new_line(true), _first_time(true), _enabled(true), _level(LogLevelInformation)
  {
  }

  void Log::AddLogDebug(const char *format, ...)
  {
    if (_level >= LogLevelDebug)
    {
      va_list parms;
      char buffer[WRITE_LOG_BUF_SIZE];
      char *buffer2 = LogTime(buffer);

      va_start(parms, format);
      vsprintf_s(buffer2, WRITE_LOG_BUF_SIZE - 1 - strlen(buffer), format, parms);
      AddLog2(buffer);
      va_end(parms);
    }
  }

  void Log::AddLog(const char *format, ...)
  {
    va_list parms;
    char buffer[WRITE_LOG_BUF_SIZE];
    char *buffer2 = LogTime(buffer);

    va_start(parms, format);
    vsprintf_s(buffer2, WRITE_LOG_BUF_SIZE - 1 - strlen(buffer), format, parms);
    AddLog2(buffer);
    va_end(parms);
  }

  void Log::AddLog2(STR *msg)
  {
    if (!_enabled)
    {
      return;
    }

    FILE *F = nullptr;

    if (_first_time)
    {
      STR logfilename[MAX_PATH];
      fileopsGetFellowLogfileName(logfilename);
      _logfilename = logfilename;
      fopen_s(&F, _logfilename.c_str(), "w");
      _first_time = false;
    }
    else
    {
      fopen_s(&F, _logfilename.c_str(), "a");
    }

    if (F != nullptr)
    {
      fprintf(F, "%s", msg);
      fflush(F);
      fclose(F);
    }

    _new_line = (msg[strlen(msg) - 1] == '\n');
  }

  STR *Log::LogTime(STR* buffer)
  {
    if (_new_line)
    {
      // log date/time into buffer
      time_t thetime = time(NULL);
      struct tm timedata;
      localtime_s(&timedata, &thetime);
      strftime(buffer, 255, "%c: ", &timedata);
      // move buffer pointer ahead to log additional text after date/time
      return buffer + strlen(buffer);
    }

    // skip date/time, log to beginning of buffer
    buffer[0] = '\0';
    return buffer;
  }
}
