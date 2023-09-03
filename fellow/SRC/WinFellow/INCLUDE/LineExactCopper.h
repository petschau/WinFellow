#ifndef LINEEXACTCOPPER_H
#define LINEEXACTCOPPER_H

#include "COPPER.H"

class LineExactCopper : public Copper
{
private:
  static ULO cycletable[16];

  /*============================================================================*/
  /* Translation table for raster ypos to cycle translation                     */
  /*============================================================================*/

  ULO ytable[512];

  void YTableInit();
  ULO GetCheckedWaitCycle(ULO waitCycle);
  void RemoveEvent();
  void InsertEvent(ULO cycle);

public:
  virtual void NotifyDMAEnableChanged(bool new_dma_enable_state);
  virtual void NotifyCop1lcChanged();
  virtual void Load(ULO new_copper_pc);
  virtual void EventHandler();

  virtual void EndOfFrame();
  virtual void HardReset();
  virtual void EmulationStart();
  virtual void EmulationStop();

  LineExactCopper();
  virtual ~LineExactCopper();
};

#endif
