#pragma once

#include <cstdint>

namespace fellow::api::vm
{
  class DisplayState
  {
  public:
    // Raw values
    uint16_t VPOS;
    uint16_t VHPOS;
    uint16_t DENISEID;
    uint16_t DIWSTRT;
    uint16_t DIWSTOP;
    uint16_t DIWHIGH;
    uint16_t DDFSTRT;
    uint16_t DDFSTOP;
    uint32_t BPLPT[6];
    uint16_t BPLDAT[6];
    uint16_t BPLCON0;
    uint16_t BPLCON1;
    uint16_t BPLCON2;
    uint16_t BPL1MOD;
    uint16_t BPL2MOD;
    uint16_t COLOR[32];

    // Decomposition
    uint16_t AgnusId;
    uint16_t VerticalPosition;
    uint16_t HorisontalPosition;
    bool Lof;
    uint16_t DiwXStart;
    uint16_t DiwXStop;
    uint16_t DiwYStart;
    uint16_t DiwYStop;
    bool IsLores;
    bool IsDualPlayfield;
    bool IsHam;
    unsigned int EnabledBitplaneCount;
  };

  class IDisplay
  {
  public:
    virtual DisplayState GetState() = 0;

    virtual ~IDisplay() = default;
  };
}
