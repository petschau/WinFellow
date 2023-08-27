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

    Registers();
  };
}