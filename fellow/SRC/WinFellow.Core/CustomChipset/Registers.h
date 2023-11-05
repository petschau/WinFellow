#pragma once

#include <cstdint>

namespace CustomChipset
{
  class Registers
  {
  public:
    uint16_t BplCon0;
    uint16_t BplCon2;
    uint16_t DmaConR;

    // Inferred values BplCon0
    bool IsLores;
    bool IsHires;
    bool IsDualPlayfield;
    bool IsHam;
    bool IsInterlaced;
    uint32_t EnabledBitplaneCount;

    void SetBplCon0(uint16_t data);

    Registers();
  };
}