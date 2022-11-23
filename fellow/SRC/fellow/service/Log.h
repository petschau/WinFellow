#pragma once

#include <string>
#include "fellow/api/service/ILog.h"

namespace fellow::service
{
  class Log : public fellow::api::ILog
  {
  private:
    static constexpr unsigned int LogLevelError = 0;
    static constexpr unsigned int LogLevelInformation = 1;
    static constexpr unsigned int LogLevelDebug = 2;

    bool _new_line;
    bool _first_time;
    bool _enabled;
    unsigned int _level;
    std::string _logfilename;

    STR *LogTime(STR *buffer);
    FILE *OpenLogFile();
    void CloseLogFile(FILE *F);
    void AddLogInternal(FILE *F, STR *msg);

  public:
    void AddLogDebug(const char *format, ...) override;
    void AddLog(const char *, ...) override;
    void AddLog(const std::string &message) override;
    void AddLogList(const std::list<std::string> &messages) override;
    void AddLog2(STR *msg) override;
    void AddTimelessLog(const char *format, ...) override;

    Log();
  };
}
