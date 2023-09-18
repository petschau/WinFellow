#ifndef LINEEXACTCOPPER_H
#define LINEEXACTCOPPER_H

#include "COPPER.H"

class LineExactCopper : public Copper
{
private:
  static uint32_t cycletable[16];

  /*============================================================================*/
  /* Translation table for raster ypos to cycle translation                     */
  /*============================================================================*/

  uint32_t ytable[512];

  void YTableInit();
  uint32_t GetCheckedWaitCycle(uint32_t waitCycle);
  void RemoveEvent();
  void InsertEvent(uint32_t cycle);

public:
  virtual void NotifyDMAEnableChanged(bool new_dma_enable_state);
  virtual void NotifyCop1lcChanged();
  virtual void Load(uint32_t new_copper_pc);
  virtual void EventHandler();

  virtual void EndOfFrame();
  virtual void HardReset();
  virtual void EmulationStart();
  virtual void EmulationStop();

  LineExactCopper();
  virtual ~LineExactCopper();
};

#endif
