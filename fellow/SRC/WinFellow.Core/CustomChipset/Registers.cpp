#include "CustomChipset/Registers.h"
#include "CustomChipset/ChipsetInformation.h"

namespace CustomChipset
{
  void Registers::SetBplCon0(uint16_t data)
  {
    // if (BplCon0 == data) return;

    // if (Chipset_info.IsCycleExact)
    //{
    //   FlushShifter(); // Make sure shifting including current bus cycle is commited using old values
    // }

    // BplCon0 = data;
    // IsLores = (BplCon0 & 0x8000) == 0;
    // IsHires = !IsLores;
    // IsDualPlayfield = (BplCon0 & 0x0400) == 0x0400;
    // IsHam = (BplCon0 & 0x0800) == 0x0800;
    // IsInterlaced = (BplCon0 & 4) == 4;
    // EnabledBitplaneCount = (BplCon0 >> 12) & 7;

    // if (!chipset_info.IsCycleExact)
    //{
    //   graphNotifyBplCon0Changed();
    // }
    // else
    //{
    //   bitplane_shifter.UpdateDrawBatchFunc();
    // }
  }

  Registers::Registers() : BplCon0(0), BplCon2(0), DmaConR(0)
  {
  }
}