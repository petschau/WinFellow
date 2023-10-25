#include "CustomChipset/RegisterUtility.h"

namespace CustomChipset
{
  bool RegisterUtility::IsLoresEnabled() const
  {
    return (_registers.BplCon0 & 0x8000) == 0;
  }
  bool RegisterUtility::IsHiresEnabled() const
  {
    return (_registers.BplCon0 & 0x8000) != 0;
  }
  bool RegisterUtility::IsDualPlayfieldEnabled() const
  {
    return (_registers.BplCon0 & 0x0400) == 0x0400;
  }
  bool RegisterUtility::IsHAMEnabled() const
  {
    return (_registers.BplCon0 & 0x0800) == 0x0800;
  }
  bool RegisterUtility::IsInterlaceEnabled() const
  {
    return (_registers.BplCon0 & 0x0004) == 0x0004;
  }
  unsigned int RegisterUtility::GetEnabledBitplaneCount() const
  {
    return (_registers.BplCon0 >> 12) & 7;
  }

  bool RegisterUtility::IsPlayfield1PriorityEnabled() const
  {
    return (_registers.BplCon2 & 0x0040) == 0;
  }
  bool RegisterUtility::IsPlayfield2PriorityEnabled() const
  {
    return (_registers.BplCon2 & 0x0040) == 0x0040;
  }

  bool RegisterUtility::IsMasterDMAEnabled() const
  {
    return (_registers.DmaConR & 0x0200) == 0x0200;
  }
  bool RegisterUtility::IsMasterDMAAndBitplaneDMAEnabled() const
  {
    return (_registers.DmaConR & 0x0300) == 0x0300;
  }
  bool RegisterUtility::IsDiskDMAEnabled() const
  {
    return (_registers.DmaConR & 0x0010) == 0x0010;
  }
  bool RegisterUtility::IsBlitterPriorityEnabled() const
  {
    return (_registers.DmaConR & 0x0400) == 0x0400;
  }

  RegisterUtility::RegisterUtility(const Registers &registers) : _registers(registers)
  {
  }
}