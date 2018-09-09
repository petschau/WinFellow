#ifndef FELLOW_SERVICE_LOG_H
#define FELLOW_SERVICE_LOG_H

#include <string>
#include "fellow/api/service/ILog.h"

namespace fellow::service
{
  class Log : public fellow::api::service::ILog
  {
  private:
    bool _new_line;
    bool _first_time;
    bool _enabled;
    std::string _logfilename;

  public:
    void AddLog(const char *, ...) override;
    void AddLog2(STR *msg) override ;

    Log();
  };
}

#endif
