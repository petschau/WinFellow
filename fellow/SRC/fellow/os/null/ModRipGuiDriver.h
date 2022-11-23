#pragma once

/* fellow includes */
#include "fellow/api/drivers/IModRipGuiDriver.h"

class ModRipGuiDriver : public IModRipGuiDriver
{
public:
  BOOLE Initialize() override;
  void SetBusy() override;
  void UnSetBusy() override;
  BOOLE SaveRequest(struct ModuleInfo *, MemoryAccessFunc) override;
  void ErrorSave(struct ModuleInfo *) override;
  BOOLE RipFloppy(int) override;
  BOOLE RipMemory() override;
  void UnInitialize() override;
  void Error(char *) override;
  BOOLE DumpChipMem() override;
  BOOLE RunProWiz() override;
};
