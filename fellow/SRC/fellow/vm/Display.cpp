#include "fellow/vm/Display.h"
#include "fellow/chipset/BitplaneRegisters.h"

using namespace fellow::api::vm;

namespace fellow::vm
{
  DisplayState Display::GetState()
  {
    DisplayState state;

    state.VPOS = bitplane_registers.GetVPosR();
    state.VHPOS = bitplane_registers.GetVHPosR();
    state.DENISEID = bitplane_registers.GetDeniseId();
    state.DIWSTRT = bitplane_registers.diwstrt;
    state.DIWSTOP = bitplane_registers.diwstop;
    state.DIWHIGH = bitplane_registers.diwhigh;
    state.DDFSTRT = bitplane_registers.ddfstrt;
    state.DDFSTOP = bitplane_registers.ddfstop;

    for (unsigned int i = 0; i < 6; i++)
    {
      state.BPLPT[i] = bitplane_registers.bplpt[i];
      state.BPLDAT[i] = bitplane_registers.bpldat[i];
    }

    state.BPLCON0 = bitplane_registers.bplcon0;
    state.BPLCON1 = bitplane_registers.bplcon1;
    state.BPLCON2 = bitplane_registers.bplcon2;
    state.BPL1MOD = bitplane_registers.bpl1mod;
    state.BPL2MOD = bitplane_registers.bpl2mod;

    for (unsigned int i = 0; i < 32; i++)
    {
      state.COLOR[i] = bitplane_registers.color[i];
    }

    // Decomposed values
    state.AgnusId = bitplane_registers.GetAgnusId();
    state.VerticalPosition = bitplane_registers.GetVerticalPosition();
    state.HorisontalPosition = bitplane_registers.GetHorisontalPosition();
    state.Lof = bitplane_registers.lof != 0;
    state.DiwXStart = bitplane_registers.DiwXStart;
    state.DiwXStop = bitplane_registers.DiwXStop;
    state.DiwYStart = bitplane_registers.DiwYStart;
    state.DiwYStop = bitplane_registers.DiwYStop;
    state.IsLores = bitplane_registers.IsLores;
    state.IsDualPlayfield = bitplane_registers.IsDualPlayfield;
    state.IsHam = bitplane_registers.IsHam;
    state.EnabledBitplaneCount = bitplane_registers.EnabledBitplaneCount;

    return state;
  }
}