#pragma once

#include "fellow/api/drivers/IniValues.h"

class IIniDriver
{
public:
  virtual void SetCurrentInitdata(IniValues *currentinitdata) = 0;
  virtual IniValues *GetCurrentInitdata() = 0;

  virtual void SetDefaultInitdata(IniValues *initdata) = 0;
  virtual IniValues *GetDefaultInitdata() = 0;

  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;
  virtual void Startup() = 0;
  virtual void Shutdown() = 0;
};
