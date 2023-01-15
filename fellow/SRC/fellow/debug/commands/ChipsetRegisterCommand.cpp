#include "fellow/debug/commands/ChipsetRegisterCommand.h"
#include "fellow/api/VM.h"

using namespace fellow::api;

ChipsetRegisterResult ChipsetRegisterCommand::Execute(const ChipsetRegisterParameters &parameters)
{
  return ChipsetRegisterResult{.DisplayState = _chipset->Display->GetState(), .CopperState = _chipset->Copper->GetState()};
}

ChipsetRegisterCommand::ChipsetRegisterCommand(vm::IChipset *chipset) : _chipset(chipset)
{
}
