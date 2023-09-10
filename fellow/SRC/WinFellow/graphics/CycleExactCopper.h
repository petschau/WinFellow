#ifndef CYCLEEXACTCOPPER_H
#define CYCLEEXACTCOPPER_H

#include "DEFS.H"
#include "COPPER.H"

typedef enum CopperStates_
{
  COPPER_STATE_NONE = 0,
  COPPER_STATE_READ_FIRST_WORD = 1,
  COPPER_STATE_READ_SECOND_WORD = 2,
  COPPER_STATE_TRANSFER_SECOND_WORD = 3
} CopperStates;

class CycleExactCopper : public Copper
{
private:
  CopperStates _state;
  UWO _first;
  UWO _second;
  bool _skip_next;

  UWO ReadWord();
  void IncreasePtr();

  void SetState(CopperStates newState, ULO cycle);
  void SetStateNone();
  
  bool IsRegisterAllowed(ULO regno);
  
  void Move();
  void Wait();
  void Skip();
  bool IsMove();
  bool IsWait();

  void ReadFirstWord();
  void ReadSecondWord();
  void TransferSecondWord();

public:
  virtual void NotifyDMAEnableChanged(bool new_dma_enable_state);
  virtual void NotifyCop1lcChanged();
  virtual void Load(ULO new_copper_pc);
  virtual void EventHandler();

  virtual void EndOfFrame();
  virtual void HardReset();
  virtual void EmulationStart();
  virtual void EmulationStop();

  CycleExactCopper();
  virtual ~CycleExactCopper();
};

#endif
