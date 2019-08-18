#include "fellow/vm/Copper.h"
#include "fellow/chipset/CopperRegisters.h"
#include "fellow/scheduler/Scheduler.h"
#include "FMEM.H"
#include "fellow/chipset/CopperUtility.h"

using namespace fellow::api::vm;

namespace fellow::vm
{
  CopperState Copper::GetState()
  {
    return CopperState{GetChipType(),
                       copper_registers.cop1lc,
                       copper_registers.cop2lc,
                       copper_registers.PC,
                       copper_registers.copcon,
                       copper_registers.InstructionStart,
                       (copper_registers.SuspendedWait != SchedulerEvent::EventDisableCycle) ? CopperRunState::Suspended : CopperRunState::Active};
  }

  CopperChipType Copper::GetChipType()
  {
    if (chipsetGetECS())
    {
      return CopperChipType::ECS;
    }

    return CopperChipType::OCS;
  }

  CopperDisassemblyLine Copper::GetDisassembly(uint32_t address)
  {
    uint16_t ir1 = chipmemReadWord(address);
    uint16_t ir2 = chipmemReadWord(address + 2);

    return CopperDisassemblyLine{address, ir1, ir2,
                                 CopperUtility::IsMove(ir1)   ? CopperInstructionType::Move
                                 : CopperUtility::IsWait(ir2) ? CopperInstructionType::Wait
                                                              : CopperInstructionType::Skip,
                                 (!CopperUtility::IsMove(ir1) && !CopperUtility::HasBlitterFinishedDisable(ir2)) ? "Blitter finish enabled" : ""};
  }
}