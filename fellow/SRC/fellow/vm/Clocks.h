#pragma once

#include "fellow/api/vm/IClocks.h"

namespace fellow::vm
{
  class Clocks : public fellow::api::vm::IClocks
  {
  public:
    uint64_t GetFrameNumber() override;
    uint32_t GetFrameCycle() override;
    fellow::api::vm::FrameLayoutType GetFrameLayout() override;
    bool GetFrameIsInterlaced() override;
    bool GetFrameIsLong() override;
    unsigned int GetLineNumber() override;
    unsigned int GetLineCycle() override;
    bool GetLineIsLong() override;

    Clocks() = default;
    virtual ~Clocks() = default;
  };
}
