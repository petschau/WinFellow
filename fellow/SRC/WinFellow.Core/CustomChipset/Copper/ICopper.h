#pragma once

#include <cstdint>

class ICopper
{
public:
  virtual void NotifyDMAEnableChanged(bool new_dma_enable_state) = 0;
  virtual void NotifyCop1lcChanged() = 0;
  virtual void Load(uint32_t new_copper_pc) = 0;
  virtual void EventHandler() = 0;

  virtual void InitializeCopperEvent() = 0;

  virtual void EndOfFrame() = 0;
  virtual void HardReset() = 0;
  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;

  virtual ~ICopper() = default;
};
