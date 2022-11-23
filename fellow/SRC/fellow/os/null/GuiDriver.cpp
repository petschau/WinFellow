#include "fellow/os/null/GuiDriver.h"

void GuiDriver::Requester(fellow::api::FELLOW_REQUESTER_TYPE type, const char *szMessage)
{
}

void GuiDriver::SetProcessDPIAwareness(const char *pszAwareness)
{
}

BOOLE GuiDriver::Enter()
{
  // Return true to exit emulator
  return FALSE;
}

void GuiDriver::Startup()
{
}

void GuiDriver::Shutdown()
{
}
