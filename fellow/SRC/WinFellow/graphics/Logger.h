#ifndef LOGGER_H
#define LOGGER_H

#include "DEFS.H"

class Logger
{
private:
  bool _enableLog;
  FILE *_logfile;

public:
  void Log(uint32_t line, uint32_t cylinder, char *message);
  bool IsLogEnabled(void) {return _enableLog;}

  void Shutdown(void);

  Logger(void) : _enableLog(false), _logfile(0) {};

};

#endif