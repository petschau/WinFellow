#pragma once

#include <Windows.h>
#include <string>
#include "fellow/debug/console/ConsoleDebugger.h"

class ConsoleDebuggerHost
{
private:
  ConsoleDebugger &_debugger;
  HANDLE _conOut;
  HANDLE _conIn;
  char *_exitCommand = "exit";
  bool _previousPauseEmulationOnLostFocus;

  bool InitializeConsole();
  void ReleaseConsole();

  void WriteLine(const std::string &line);
  std::string ReadLine();

  void ReadCommands();

  bool StartVM();
  void StopVM();

public:
  bool RunInDebugger();

  ConsoleDebuggerHost(ConsoleDebugger &debugger);
};
