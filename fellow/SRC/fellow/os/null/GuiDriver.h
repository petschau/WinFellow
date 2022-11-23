#pragma once

#include "fellow/api/drivers/IGuiDriver.h"

class GuiDriver : public IGuiDriver
{
public:
  void Requester(fellow::api::FELLOW_REQUESTER_TYPE type, const char *szMessage) override;
  void SetProcessDPIAwareness(const char *pszAwareness) override;
  BOOLE Enter() override;
  void Startup() override;
  void Shutdown() override;
};
