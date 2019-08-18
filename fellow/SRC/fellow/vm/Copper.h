#pragma once

#include "fellow/api/vm/ICopper.h"

namespace fellow::vm
{
  class Copper : public fellow::api::vm::ICopper
  {
  private:
    api::vm::CopperChipType GetChipType();

  public:
    fellow::api::vm::CopperState GetState() override;
    fellow::api::vm::CopperDisassemblyLine GetDisassembly(uint32_t address) override;

    Copper() = default;
    virtual ~Copper() = default;
  };
}
