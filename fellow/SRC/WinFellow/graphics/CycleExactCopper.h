#pragma once

#include "Defs.h"
#include "LegacyCopper.h"

enum class CopperStates
{
  COPPER_STATE_NONE = 0,
  COPPER_STATE_READ_FIRST_WORD = 1,
  COPPER_STATE_READ_SECOND_WORD = 2,
  COPPER_STATE_TRANSFER_SECOND_WORD = 3
};

class CycleExactCopper : public Copper
{
private:
  CopperStates _state;
  uint16_t _first;
  uint16_t _second;
  bool _skip_next;

  uint16_t ReadWord();
  void IncreasePtr();

  void SetState(CopperStates newState, uint32_t cycle);
  void SetStateNone();

  bool IsRegisterAllowed(uint32_t regno);

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
  virtual void Load(uint32_t new_copper_pc);
  virtual void EventHandler();

  virtual void EndOfFrame();
  virtual void HardReset();
  virtual void EmulationStart();
  virtual void EmulationStop();

  CycleExactCopper();
  virtual ~CycleExactCopper();
};
