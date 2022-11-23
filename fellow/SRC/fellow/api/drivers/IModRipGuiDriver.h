#pragma once

/* fellow includes */
#include "fellow/api/defs.h"

/* own includes */
#include "fellow/application/modrip.h"

class IModRipGuiDriver
{
public:
  virtual BOOLE Initialize() = 0;
  virtual void SetBusy() = 0;
  virtual void UnSetBusy() = 0;
  virtual BOOLE SaveRequest(struct ModuleInfo *, MemoryAccessFunc) = 0;
  virtual void ErrorSave(struct ModuleInfo *) = 0;
  virtual BOOLE RipFloppy(int) = 0;
  virtual BOOLE RipMemory() = 0;
  virtual void UnInitialize() = 0;
  virtual void Error(char *) = 0;
  virtual BOOLE DumpChipMem() = 0;
  virtual BOOLE RunProWiz() = 0;
};
