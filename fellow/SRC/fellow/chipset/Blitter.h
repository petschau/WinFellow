#pragma once

#include "fellow/api/defs.h"

struct blitter_state
{
  // Actual registers
  ULO bltcon;
  ULO bltafwm;
  ULO bltalwm;
  ULO bltapt;
  ULO bltbpt;
  ULO bltcpt;
  ULO bltdpt;
  ULO bltamod;
  ULO bltbmod;
  ULO bltcmod;
  ULO bltdmod;
  ULO bltadat;
  ULO bltbdat;
  ULO bltbdat_original;
  ULO bltcdat;
  ULO bltzero;

  // Calculated by the set-functions based on the state above
  ULO height;
  ULO width;

  // Line mode alters these and the state remains after the line is done.
  // ie. the next line continues where the last left off if the shifts are not re-initialised
  // by Amiga code.
  ULO a_shift_asc;
  ULO a_shift_desc;
  ULO b_shift_asc;
  ULO b_shift_desc;

  // Information about an ongoing blit
  // dma_pending is a flag showing that a blit has been activated (a write to BLTSIZE)
  // but at the time of activation the blit DMA was turned off
  BOOLE started;
  BOOLE dma_pending;
  ULO cycle_length; // Estimate for how many cycles the started blit will take
  ULO cycle_free;   // How many of these cycles are free to use by the CPU
};

// #define BLIT_VERIFY_MINTERMS

class Scheduler;

class Blitter
{
private:
  Scheduler *_scheduler;
  blitter_state blitter{};
  static ULO blit_cyclelength[16];
  static ULO blit_cyclefree[16];

  UBY blit_fill[2][2][256][2]{}; // Blitter fill mode lookup table, [inc,exc][fc][data][0 = next fc, 1 = filled data]

#ifdef BLIT_VERIFY_MINTERMS
  BOOLE blit_minterm_seen[256];
#endif
  BOOLE blitter_fast;

  void SetFast(BOOLE fast);
  BOOLE GetFast();
  BOOLE GetDMAPending();
  BOOLE GetZeroFlag();
  ULO GetFreeCycles();
  BOOLE IsDescending();
  void RemoveEvent();
  void InsertEvent(ULO cycle);

  void CopyABCD();
  void LineMode();
  void InitiateBlit();
  void FinishBlit();
  void ForceFinish();

  void wbltcon0(UWO data, ULO address);
  void wbltcon1(UWO data, ULO address);
  void wbltafwm(UWO data, ULO address);
  void wbltalwm(UWO data, ULO address);
  void wbltcpth(UWO data, ULO address);
  void wbltcptl(UWO data, ULO address);
  void wbltbpth(UWO data, ULO address);
  void wbltbptl(UWO data, ULO address);
  void wbltapth(UWO data, ULO address);
  void wbltaptl(UWO data, ULO address);
  void wbltdpth(UWO data, ULO address);
  void wbltdptl(UWO data, ULO address);
  void wbltsize(UWO data, ULO address);
  void wbltcon0l(UWO data, ULO address);
  void wbltsizv(UWO data, ULO address);
  void wbltsizh(UWO data, ULO address);
  void wbltcmod(UWO data, ULO address);
  void wbltbmod(UWO data, ULO address);
  void wbltamod(UWO data, ULO address);
  void wbltdmod(UWO data, ULO address);
  void wbltcdat(UWO data, ULO address);
  void wbltbdat(UWO data, ULO address);
  void wbltadat(UWO data, ULO address);
  void FillTableInit();
  void IOHandlersInstall();
  void IORegistersClear();
  void HardReset();
  void EndOfFrame();
  void StartEmulation();
  void StopEmulation();

#ifdef BLIT_VERIFY_MINTERMS
  ULO OptimizedMinterms(UBY minterm, ULO a_dat, ULO b_dat, ULO c_dat);
  ULO CorrectMinterms(UBY minterm, ULO a_dat, ULO b_dat, ULO c_dat);
  void VerifyMinterms();
#endif


public:
  BOOLE IsStarted();

  void Startup();
  void Shutdown();

  Blitter(Scheduler *scheduler);
};
