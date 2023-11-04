#pragma once

#include "Defs.h"

class Logger
{
private:
  bool _enableLog;
  FILE *_logfile;

public:
  void Log(uint32_t line, uint32_t cylinder, const char *message);
  bool IsLogEnabled()
  {
    return _enableLog;
  }

  void Shutdown();

  Logger() : _enableLog(false), _logfile(nullptr){};
};
