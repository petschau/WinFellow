#pragma once

#include <string>

#include "fellow/api/defs.h"
#include "fellow/api/IRuntimeEnvironment.h"
#include "fellow/api/Services.h"
#include "fellow/api/VM.h"
#include "fellow/application/Modules.h"

enum class fellow_runtime_error_codes
{
  FELLOW_RUNTIME_ERROR_NO_ERROR = 0,
  FELLOW_RUNTIME_ERROR_CPU_PC_BAD_BANK = 1
};

class Fellow : public fellow::api::IRuntimeEnvironment
{
private:
  bool _performResetBeforeStartingEmulation = false;
  fellow::api::IPerformanceCounter *_emulationRunPerformanceCounter = nullptr;
  fellow::api::VirtualMachine *_vm{};
  Modules _modules{};
  Scheduler _scheduler;

  jmp_buf _runtimeErrorEnvironment{};
  fellow_runtime_error_codes _runtimeErrorCode{};

  void CreateServices();
  void DeleteServices();

  void CreateVM();
  void DeleteVM();

  void CreateDrivers();
  void InitializeDrivers();
  void DeleteDrivers();

  void CreateModules();
  void InitializeModules(int argc, char *argv[]);
  void DeleteModules();

  void DrawFailed();
  void NastyExit();

  void StepFrame();
  void StepLine();

  void Run();

  void SoftReset();
  void HardReset();

  void RuntimeErrorCheck();
  void HandleCpuPcBadBank();

  void SetRuntimeErrorCode(fellow_runtime_error_codes error_code);
  fellow_runtime_error_codes GetRuntimeErrorCode();

  void LogPerformanceReport();
  void LogPerformanceCounterPercentage(const fellow::api::IPerformanceCounter *counter, LLO ticksPerFrame, ULL frameCount);

public:
  void RequestResetBeforeStartingEmulation() override;
  void RequestEmulationStop() override;
  std::string GetVersionString() override;

  bool StartEmulation() override;
  void StopEmulation() override;

  void StepOne() override;
  void StepOver() override;
  void RunDebug(ULO breakpoint) override;

  fellow::api::VirtualMachine *GetVM() override;

  void RunApplication();

  Fellow(int argc, char *argv[]);
  virtual ~Fellow();
};
