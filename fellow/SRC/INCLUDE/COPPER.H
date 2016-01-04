#ifndef COPPER_H
#define COPPER_H

extern void copperEventHandler();
extern void copperSaveState(FILE *F);
extern void copperLoadState(FILE *F);
extern void copperEndOfFrame();
extern void copperHardReset();
extern void copperEmulationStart();
extern void copperEmulationStop();
extern void copperStartup();
extern void copperShutdown();

class Copper
{
public:
  virtual void NotifyDMAEnableChanged(bool new_dma_enable_state) = 0;
  virtual void NotifyCop1lcChanged() = 0;
  virtual void Load(ULO new_copper_pc) = 0;
  virtual void EventHandler() = 0;

  virtual void EndOfFrame() = 0;
  virtual void HardReset() = 0;
  virtual void EmulationStart() = 0;
  virtual void EmulationStop() = 0;
};

extern Copper* copper;

#endif
