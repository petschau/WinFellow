#pragma once

#include <cstdint>

namespace CustomChipset
{
  class ChipsetInformation
  {
  private:
    bool _isEcs;
    bool _isCycleExact;
    uint32_t _pointerMask;
    uint32_t _addressMask;

    void UpdateAddressMasksFromConfiguration();

  public:
    uint32_t MaskAddress(uint32_t address);
    uint32_t MaskPointer(uint32_t pointer);
    uint32_t ReplaceLowPointer(uint32_t originalPointer, uint16_t newLowPart);
    uint32_t ReplaceHighPointer(uint32_t originalPointer, uint16_t newHighPart);

    bool SetIsEcs(bool isEcs);
    bool GetIsEcs();

    void Startup();
  };
}