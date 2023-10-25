#pragma once

#include "ILog.h"

namespace Service
{
  class Log : public ILog
  {
  private:
    static const unsigned int LogLevelError = 0;
    static const unsigned int LogLevelInformation = 1;
    static const unsigned int LogLevelDebug = 2;

    bool _new_line;
    bool _first_time;
    bool _enabled;
    unsigned int _level;
    std::string _logfilename;

    char *LogTime(char *buffer);
    FILE *OpenLogFile();
    void CloseLogFile(FILE *F);
    void AddLogInternal(FILE *F, const char *msg);

  public:
    void AddLogDebug(const char *format, ...) override;
    void AddLog(const char *, ...) override;
    void AddLogList(const std::list<std::string> &messages) override;
    void AddLog2(const char *msg) override;
    void AddTimelessLog(const char *format, ...) override;

    Log();
    virtual ~Log();
  };
}
