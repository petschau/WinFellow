#pragma once

class LineExactCopper
{
private:
  static ULO _cycletable[16];

  // Translation table for raster ypos to cycle
  ULO _ytable[512]{};
  ULO _firstCopperCycle{};

  void YTableInit();
  ULO GetCheckedWaitCycle(const ULO waitCycle);
  void RemoveEvent();
  void InsertEvent(const ULO cycle);
  UWO ReadWord();

public:
  void NotifyDMAEnableChanged(const bool new_dma_enable_state);
  void NotifyCop1lcChanged();
  void Load(const ULO new_copper_pc);
  void EventHandler();

  void EndOfFrame();
  void HardReset();
  void EmulationStart();
  void EmulationStop();
  void Startup();
  void Shutdown();

  LineExactCopper();
  ~LineExactCopper() = default;
};

extern LineExactCopper line_exact_copper;
