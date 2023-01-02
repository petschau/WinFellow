#pragma once

#include "fellow/api/defs.h"

enum class FELLOW_REQUESTER_TYPE
{
  FELLOW_REQUESTER_TYPE_NONE = 0,
  FELLOW_REQUESTER_TYPE_INFO = 1,
  FELLOW_REQUESTER_TYPE_WARN = 2,
  FELLOW_REQUESTER_TYPE_ERROR = 3
};

class IGuiDriver
{
public:
  virtual bool CheckEmulationNecessities() = 0;
  virtual void Requester(FELLOW_REQUESTER_TYPE type, const char *szMessage) = 0;
  virtual void SetProcessDPIAwareness(const char *pszAwareness) = 0;
  virtual void BeforeEnter() = 0;
  virtual BOOLE Enter() = 0;
  virtual void Initialize() = 0;
  virtual void Release() = 0;
};
