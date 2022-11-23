#include "fellow/os/null/MouseDriver.h"

BOOLE MouseDriver::GetFocus()
{
  return true;
}

void MouseDriver::StateHasChanged(BOOLE active)
{
}

void MouseDriver::ToggleFocus()
{
}

void MouseDriver::MovementHandler()
{
}

void MouseDriver::SetFocus(const BOOLE bNewFocus, const BOOLE bRequestedByRPHost)
{
}

void MouseDriver::HardReset()
{
}

BOOLE MouseDriver::EmulationStart()
{
  return true;
}

void MouseDriver::EmulationStop()
{
}

void MouseDriver::Startup()
{
}

void MouseDriver::Shutdown()
{
}
