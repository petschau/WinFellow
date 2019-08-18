#include "fellow/debug/commands/ChipsetRegisterCommand.h"
#include "fellow/api/VM.h"

using namespace fellow::api;

ChipsetRegisterResult ChipsetRegisterCommand::Execute(const ChipsetRegisterParameters &parameters)
{
  return ChipsetRegisterResult{.DisplayState = VM->Chipset.Display.GetState(), .CopperState = VM->Chipset.Copper.GetState()};
}
