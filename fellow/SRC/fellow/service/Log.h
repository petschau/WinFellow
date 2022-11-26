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

    char *LogTime(char *buffer);
    FILE *OpenLogFile();
    void CloseLogFile(FILE *F);
    void AddLogInternal(FILE *F, const char *msg);

  public:
    void AddLogDebug(const char *format, ...) override;
    void AddLog(const char *format, ...) override;
    void AddLog(const std::string &message) override;
    void AddLogList(const std::list<std::string> &messages) override;
    void AddLog2(const char *msg) override;
    void AddTimelessLog(const char *format, ...) override;
    void AddTimelessLog(const std::string &message) override;

    Log();
  };
}
