#include "defs.h"

#ifdef GRAPH2

#include "bus.h"
#include "fmem.h"
#include "copper.h"
#include "GRAPH.H"

extern ULO copper_ptr;
extern ULO copcon;
extern ULO cop1lc;
extern ULO copper_suspended_wait;
extern BOOLE copper_dma;

typedef enum CopperStates_
{
  COPPER_STATE_NONE = 0,
  COPPER_STATE_READ_FIRST_WORD = 1,
  COPPER_STATE_READ_SECOND_WORD = 2
} CopperStates;

typedef struct CopperStateMachine_
{
  CopperStates state;

  UWO first;
  UWO second;

  BOOLE skip_next;

} CopperStateMachine;

CopperStateMachine CopperState;

static UWO Copper_ReadWord(void)
{
  return ((UWO) (memory_chip[copper_ptr] << 8) | (UWO) memory_chip[copper_ptr + 1]);
}

static void Copper_IncreasePtr(void)
{
  copper_ptr = (copper_ptr + 2) & 0x1ffffe;
}

void Copper_SetState(CopperStates newState, ULO cycle)
{
  if ((cycle % BUS_CYCLE_PER_LINE) & 1)
  {
    cycle++;
  }
  busRemoveEvent(&copperEvent);
  CopperState.state = newState;
  copperEvent.cycle = cycle;
  if (copper_dma)
  {
    busInsertEvent(&copperEvent);
  }
}

void Copper_Load(void)
{
  CopperState.skip_next = FALSE;
  ULO startCycle = busGetCycle() + 6;
  if ((startCycle % BUS_CYCLE_PER_LINE) & 1)
  {
    startCycle++;
  }
  Copper_SetState(COPPER_STATE_READ_FIRST_WORD, startCycle);
}

static void Copper_SetStateNone(void)
{
  busRemoveEvent(&copperEvent);
  CopperState.state = COPPER_STATE_NONE;
  CopperState.skip_next = FALSE;
  copperEvent.cycle = BUS_CYCLE_DISABLE;
}

static BOOLE Copper_RegisterIsAllowed(ULO regno)
{
  return (regno >= 0x80) || ((regno >= 0x40) && ((copcon & 0xffff) != 0x0));
}

static void Copper_Move(void)
{
  ULO regno = (ULO) (CopperState.first & 0x1fe);
  UWO value = CopperState.second;

  if (Copper_RegisterIsAllowed(regno))
  {
    Copper_SetState(COPPER_STATE_READ_FIRST_WORD, busGetCycle() + 2);
    if (!CopperState.skip_next)
    {
      memory_iobank_write[regno>>1](value, regno);
    }
    CopperState.skip_next = FALSE;
  }
  else
  {
    Copper_SetStateNone();
  }
}

static void Copper_Wait(void)
{
  bool blitterFinishDisable = (CopperState.second & 0x8000) == 0x8000;
  ULO ve = (((ULO) CopperState.second >> 8) & 0x7f) | 0x80;
  ULO he = (ULO) CopperState.second & 0xfe;

  ULO vp = (ULO) (CopperState.first >> 8);
  ULO hp = (ULO) (CopperState.first & 0xfe);

  ULO test_cycle = busGetCycle() + 2;
  ULO rasterY = test_cycle / BUS_CYCLE_PER_LINE;
  ULO rasterX = test_cycle % BUS_CYCLE_PER_LINE;

  if (rasterX & 1)
  {
    rasterX++;
  }

  CopperState.skip_next = FALSE;

  // Is the vertical position already larger? 
  if ((rasterY & ve) > (vp & ve))
  {
    CopperState.skip_next = FALSE;
    Copper_SetState(COPPER_STATE_READ_FIRST_WORD, busGetCycle() + 2);
    return;
  }

  // Is the vertical position an exact match? Examine the rest of the line for matching horisontal positions.
  if ((rasterY & ve) == (vp & ve))
  {
    while ((rasterX <= 0xe2) && ((rasterX & he) < (hp & he))) rasterX += 2;
    if (rasterX < 0xe4)
    {
      Copper_SetState(COPPER_STATE_READ_FIRST_WORD, rasterY*BUS_CYCLE_PER_LINE + rasterX + 2);
      return;
    }
  }

  // Try later lines
  rasterY++;

  // Find the first horisontal position on a line that match when comparing from the start of a line
  for (rasterX = 0; (rasterX <= 0xe2) && ((rasterX & he) < (hp & he)); rasterX += 2);

  // Find the first vertical position that match
  if (rasterX == 0xe4)
  {
    // There is no match on the horisontal position. The vertical position must be larger than vp to match
    while ((rasterY < BUS_LINES_PER_FRAME) && ((rasterY & ve) <= (vp & ve))) rasterY++;
  }
  else
  {
    // A match can either be exact on vp and hp, or the vertical position must be larger than vp.
    while ((rasterY < BUS_LINES_PER_FRAME) && ((rasterY & ve) < (vp & ve))) rasterY++;
  }

  if (rasterY >= BUS_LINES_PER_FRAME)
  {
    // No match was found
    Copper_SetStateNone();
  }
  else if ((rasterY & ve) == (vp & ve))
  {
    // An exact match on both vp and hp was found
    Copper_SetState(COPPER_STATE_READ_FIRST_WORD, rasterY*BUS_CYCLE_PER_LINE + rasterX + 2);
  }
  else
  {
    // A match on vp being larger (not equal) was found
    Copper_SetState(COPPER_STATE_READ_FIRST_WORD, rasterY*BUS_CYCLE_PER_LINE + rasterX + 2);
  }
}

static void Copper_Skip(void)
{
  ULO ve = (((ULO) CopperState.second >> 8) & 0x7f) | 0x80;
  ULO he = (ULO) CopperState.second & 0xfe;

  ULO vp = (ULO) (CopperState.first >> 8);
  ULO hp = (ULO) (CopperState.first & 0xfe);

  ULO test_cycle = busGetCycle();
  ULO rasterY = test_cycle / BUS_CYCLE_PER_LINE;
  ULO rasterX = test_cycle % BUS_CYCLE_PER_LINE;

  if (rasterX & 1)
  {
    rasterX++;
  }

  CopperState.skip_next = (((rasterY & ve) > (vp & ve)) || (((rasterY & ve) == (vp & ve)) && ((rasterX & he) >= (hp & he))));
  Copper_SetState(COPPER_STATE_READ_FIRST_WORD, busGetCycle() + 4);
}

static void Copper_Execute(void)
{
  if ((CopperState.first & 1) == 0)
  {
    Copper_Move();
  }
  else if ((CopperState.second & 1) == 0)
  {    
    Copper_Wait();
  }
  else
  {
    Copper_Skip();
  }
}

static void Copper_ReadFirstWord(void)
{
  CopperState.first = Copper_ReadWord();
  Copper_IncreasePtr();
  Copper_SetState(COPPER_STATE_READ_SECOND_WORD, busGetCycle() + 2);
}

static void Copper_ReadSecondWord(void)
{
  CopperState.second = Copper_ReadWord();
  Copper_IncreasePtr();
  Copper_Execute();
}

void Copper_EventHandler(void)
{
  if (busGetRasterX() == 0xe2)
  {
    Copper_SetState(CopperState.state, busGetCycle() + 1);
    return;
  }
  if (cpuEvent.cycle != BUS_CYCLE_DISABLE)
  {
    cpuEvent.cycle += 2;
  }
  switch (CopperState.state)
  {
    case COPPER_STATE_READ_FIRST_WORD:
      Copper_ReadFirstWord();
      break;
    case COPPER_STATE_READ_SECOND_WORD:
      Copper_ReadSecondWord();
      break;
  }
}

/*-------------------------------------------------------------------------------
; void copperUpdateDMA(void);
; Called by wdmacon every time that register is written
; This routine takes action when the copper DMA state is changed
;-------------------------------------------------------------------------------*/

void Copper_UpdateDMA(void)
{
  if (copper_dma == TRUE)
  {
    // here copper was on, test if it is still on
    if ((dmacon & 0x80) == 0x0)
    {
      // here copper DMA is being turned off
      // remove copper from the event list
      copper_dma = FALSE;
      busRemoveEvent(&copperEvent);
    }
  }
  else
  {
    // here copper was off, test if it is still off
    if ((dmacon & 0x80) == 0x80)
    {
      // here copper is being turned on
      // reactivate the cycle the copper was waiting for the last time it was on.
      // if we have passed it in the mean-time, execute immediately.
      busRemoveEvent(&copperEvent);
      if (copperEvent.cycle != -1)
      {
	// dma not hanging
	if (copperEvent.cycle <= bus.cycle)
	{
          copperEvent.cycle = busGetCycle() + 2;
          busInsertEvent(&copperEvent);
	}
	else
	{
          busInsertEvent(&copperEvent);
	}
      }
      copper_dma = TRUE;
    }
  }
}

void Copper_EndOfFrame(void)
{
  copper_ptr = cop1lc;
  CopperState.skip_next = FALSE;
  Copper_SetState(COPPER_STATE_READ_FIRST_WORD, 40);
}

void Copper_Startup(void)
{
  CopperState.skip_next = FALSE;
}

void Copper_Shutdown(void)
{
}

#endif
