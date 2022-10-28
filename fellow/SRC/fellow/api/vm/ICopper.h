#pragma once

#include <cstdint>
#include <string>

namespace fellow::api::vm
{
  enum class CopperRunState
  {
    Active = 0,
    Suspended = 1,
    Halted = 2
  };

  enum class CopperInstructionType
  {
    Move = 0,
    Wait = 1,
    Skip = 2
  };

  enum class CopperChipType
  {
    OCS = 0,
    ECS = 1,
    AGA = 2
  };

  class CopperDisassemblyLine
  {
  public:
    uint32_t Address;
    uint16_t IR1;
    uint16_t IR2;
    CopperInstructionType Type;
    std::string Information;
  };

  class CopperState
  {
  public:
    CopperChipType ChipType;
    uint32_t COP1LC;
    uint32_t COP2LC;
    uint32_t COPLC;
    uint16_t COPCON;

    uint32_t InstructionStart;
    CopperRunState RunState;
  };

  class ICopper
  {
  public:
    virtual CopperState GetState() = 0;
    virtual CopperDisassemblyLine GetDisassembly(uint32_t address) = 0;

    virtual ~ICopper() = default;
  };
}
