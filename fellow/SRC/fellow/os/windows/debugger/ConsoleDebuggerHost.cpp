#include "fellow/os/windows/debugger/ConsoleDebuggerHost.h"

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <iostream>
#include "fellow/api/defs.h"
#include "fellow/api/Services.h"
#include "fellow/application/Fellow.h"

#include "fellow/application/WGui.h"
#include "GfxDrvCommon.h"

using namespace std;
using namespace fellow::api;

void ConsoleDebuggerHost::WriteLine(const std::string &line)
{
  DWORD numberOfCharsWritten = 0;
  LPVOID reserved = nullptr;

  WriteConsole(_conOut, line.c_str(), (DWORD)line.length(), &numberOfCharsWritten, reserved);
}

std::string ConsoleDebuggerHost::ReadLine()
{
  DWORD numberOfCharsRead = 0;
  char inputBuffer[256]{};

  ReadConsole(_conIn, inputBuffer, 256, &numberOfCharsRead, nullptr);
  string line = inputBuffer;
  return line.erase(line.find_last_not_of(" \n\r\t") + 1);
}

bool ConsoleDebuggerHost::InitializeConsole()
{
  auto result = AllocConsole();
  if (!result)
  {
    Service->Log.AddLog("ConsoleDebuggerHost::InitializeConsole() - Failed to allocate console\n");
    return false;
  }

  _conOut = CreateFile("CONOUT$", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, /*FILE_ATTRIBUTE_NORMAL*/ 0, nullptr);

  if (_conOut == INVALID_HANDLE_VALUE)
  {
    Service->Log.AddLog("ConsoleDebuggerHost::InitializeConsole() - Failed to create CONOUT$\n");
    FreeConsole();
    return false;
  }

  _conIn = CreateFile("CONIN$", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, /*FILE_ATTRIBUTE_NORMAL*/ 0, nullptr);

  if (_conIn == INVALID_HANDLE_VALUE)
  {
    Service->Log.AddLog("ConsoleDebuggerHost::InitializeConsole() - Failed to create CONIN$\n");
    CloseHandle(_conOut);
    FreeConsole();
    return false;
  }

  // Disable console window close button because it closes the application, no questions asked....
  HWND hConsoleWindow = GetConsoleWindow();
  HMENU hMenu = GetSystemMenu(hConsoleWindow, false);
  DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);

  return true;
}

void ConsoleDebuggerHost::ReleaseConsole()
{
  CloseHandle(_conIn);
  CloseHandle(_conOut);
  FreeConsole();
}

void ConsoleDebuggerHost::ReadCommands()
{
  while (true)
  {
    string command = ReadLine();
    if (command == _exitCommand)
    {
      return;
    }

    string reply = _debugger.ExecuteCommand(command);
    if (!reply.empty())
    {
      WriteLine(reply);
    }
  }
}

bool ConsoleDebuggerHost::StartVM()
{
  // The configuration has been activated
  // Prepare the modules for emulation
  _previousPauseEmulationOnLostFocus = gfxDrvCommon->GetPauseEmulationWhenWindowLosesFocus();
  gfxDrvCommon->SetPauseEmulationWhenWindowLosesFocus(false);

  if (!fellowEmulationStart())
  {
    Service->Log.AddLog("ConsoleDebuggerHost::StartVM() - Failed to start emulation\n");
    StopVM();

    return false;
  }

  if (fellowGetPreStartReset())
  {
    fellowHardReset();
  }

  return true;
}

void ConsoleDebuggerHost::StopVM()
{
  fellowEmulationStop();
  gfxDrvCommon->SetPauseEmulationWhenWindowLosesFocus(_previousPauseEmulationOnLostFocus);
}

bool ConsoleDebuggerHost::RunInDebugger()
{
  bool started = StartVM();
  if (!started)
  {
    return false;
  }

  bool initialized = InitializeConsole();
  if (!initialized)
  {
    return false;
  }

  WriteLine(string(fellowGetVersionString()) + "\r\n");
  WriteLine(string("Console debugger - type h for command summary") + "\r\n");

  ReadCommands();

  ReleaseConsole();
  StopVM();

  return true;
}

ConsoleDebuggerHost::ConsoleDebuggerHost(ConsoleDebugger &debugger) : _debugger(debugger), _conIn(nullptr), _conOut(nullptr), _previousPauseEmulationOnLostFocus(false)

{
}
