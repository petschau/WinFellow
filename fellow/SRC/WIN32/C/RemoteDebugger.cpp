#include <httplib.h>
#include <sstream>

#include "RemoteDebugger.h"
#include "fellow/api/VM.h"
#include "JSON.h"
#include "WINDRV.H"
#include "FELLOW.H"
#include "fellow/api/Services.h"
#include "fellow/chipset/ChipsetInfo.h"

using namespace std;
using namespace httplib;
using namespace fellow::api::vm;
using namespace fellow::api;

class HttpServerProvider
{
public:
  Server Server;
};

class RemoteDebuggerAPI
{
private:
  void ExecuteDebugOperation(dbg_operations operation, Response& res);

public:
  // Commands
  void Run(const Request& req, Response& res);
  void RunInstruction(const Request& req, Response& res);
  void RunOverInstruction(const Request& req, Response& res);
  void RunOverLine(const Request& req, Response& res);
  void RunOverFrame(const Request& req, Response& res);
  void Break(const Request& req, Response& res);

  // Queries
  void GetCPUState(const Request& req, Response& res);
  void GetCPUDisassembly(const Request& req, Response& res);
  void GetClocks(const Request& req, Response& res);
  void GetChipsetDisplayState(const Request& req, Response& res);
  void GetChipsetCopperState(const Request& req, Response& res);
  void GetChipsetCopperDisassembly(const Request& req, Response& res);

  // Event log
  void GetEvents(const Request& req, Response& res);
};

// Commands - Every debug command will be run in a separate thread

void RemoteDebuggerAPI::ExecuteDebugOperation(dbg_operations operation, Response& res)
{
  if (winDrvDebugStart(operation, 0))
  {
    res.status = 200;
  }
  else
  {
    res.status = 409; // 409 - Conflict
  }
}

void RemoteDebuggerAPI::Run(const Request& req, Response& res)
{
  ExecuteDebugOperation(dbg_operations::DBG_RUN, res);
}

void RemoteDebuggerAPI::RunInstruction(const Request& req, Response& res)
{
  ExecuteDebugOperation(dbg_operations::DBG_STEP, res);
}

void RemoteDebuggerAPI::RunOverInstruction(const Request& req, Response& res)
{
  ExecuteDebugOperation(dbg_operations::DBG_STEP_OVER, res);
}

void RemoteDebuggerAPI::RunOverLine(const Request& req, Response& res)
{
  ExecuteDebugOperation(dbg_operations::DBG_STEP_LINE, res);
}

void RemoteDebuggerAPI::RunOverFrame(const Request& req, Response& res)
{
  ExecuteDebugOperation(dbg_operations::DBG_STEP_FRAME, res);
}

void RemoteDebuggerAPI::Break(const Request& req, Response& res)
{
  fellowRequestEmulationStop();
  res.status = 200;
}

// Queries
void RemoteDebuggerAPI::GetCPUState(const Request& req, Response& res)
{
  JSON json;

  json.StartObject(true);
  json.UnsignedNumberArrayOut<ULO>("D", 8, [](const unsigned int i) {return VM->CPU.GetDReg(i); }, true);
  json.UnsignedNumberArrayOut<ULO>("A", 8, [](const unsigned int i) {return VM->CPU.GetAReg(i); });
  json.NumberVariableOut("PC", VM->CPU.GetPC());
  json.NumberVariableOut("USP", VM->CPU.GetUSP());
  json.NumberVariableOut("SSP", VM->CPU.GetSSP());
  json.NumberVariableOut("MSP", VM->CPU.GetMSP());
  json.NumberVariableOut("ISP", VM->CPU.GetISP());
  json.NumberVariableOut("SR", VM->CPU.GetSR());
  json.EndObject();

  res.set_content(json.Get(), "application/json");
}

void RemoteDebuggerAPI::GetCPUDisassembly(const Request& req, Response& res)
{
  ULO disasm_pc = (ULO) stoi(req.matches[1].str());
  ULO count = (ULO) stoi(req.matches[2].str());
  JSON json;

  json.StartObject(true);
  json.StartArray("Lines");

  for (unsigned int i = 0; i < count; i++)
  {
    auto disassembly = VM->CPU.GetDisassembly(disasm_pc);

    json.StartObject(i == 0);
    json.NumberVariableOut("Address", disassembly.Address, true);
    json.StringVariableOut("Data", disassembly.Data);
    json.StringVariableOut("Instruction", disassembly.Instruction);
    json.StringVariableOut("Operands", disassembly.Operands);
    json.EndObject();

    disasm_pc = disassembly.Address + disassembly.Length;
  }
  json.EndArray();
  json.EndObject();

  res.set_content(json.Get(), "application/json");
}

void RemoteDebuggerAPI::GetClocks(const Request& req, Response& res)
{
  JSON json;

  json.StartObject(true);
  json.NumberVariableOut("FrameNumber", VM->Clocks.GetFrameNumber(), true);
  json.NumberVariableOut("FrameCycle", VM->Clocks.GetFrameCycle());
  json.NumberVariableOut("FrameLayout", VM->Clocks.GetFrameLayout());
  json.BoolVariableOut("FrameIsInterlaced", VM->Clocks.GetFrameIsInterlaced());
  json.BoolVariableOut("FrameIsLong", VM->Clocks.GetFrameIsLong());
  json.NumberVariableOut("LineNumber", VM->Clocks.GetLineNumber());
  json.NumberVariableOut("LineCycle", VM->Clocks.GetLineCycle());
  json.BoolVariableOut("LineIsLong", VM->Clocks.GetLineIsLong());
  json.EndObject();

  res.set_content(json.Get(), "application/json");
}

void RemoteDebuggerAPI::GetChipsetDisplayState(const Request& req, Response& res)
{
  JSON json;
  auto state = VM->Chipset.Display.GetState();

  json.StartObject(true);
  json.NumberVariableOut("LOF", state.LOF, true);
  json.NumberVariableOut("DIWSTRT", state.DIWSTRT);
  json.NumberVariableOut("DIWSTOP", state.DIWSTOP);
  json.NumberVariableOut("DIWHIGH", state.DIWHIGH);
  json.NumberVariableOut("DDFSTRT", state.DDFSTRT);
  json.NumberVariableOut("DDFSTOP", state.DDFSTOP);
  json.UnsignedNumberArrayOut<ULO>("BPLPT", 6, [state](const unsigned int i) {return state.BPLPT[i]; });
  json.UnsignedNumberArrayOut<UWO>("BPLDAT", 32, [state](const unsigned int i) {return state.BPLDAT[i]; });
  json.NumberVariableOut("BPLCON0", state.BPLCON0);
  json.NumberVariableOut("BPLCON1", state.BPLCON1);
  json.NumberVariableOut("BPLCON2", state.BPLCON2);
  json.NumberVariableOut("BPL1MOD", state.BPL1MOD);
  json.NumberVariableOut("BPL2MOD", state.BPL2MOD);
  json.UnsignedNumberArrayOut<UWO>("COLOR", 32, [state](const unsigned int i) {return state.COLOR[i]; });
  json.EndObject();

  res.set_content(json.Get(), "application/json");
}

void RemoteDebuggerAPI::GetChipsetCopperState(const Request& req, Response& res)
{
  JSON json;
  auto state = VM->Chipset.Copper.GetState();

  json.StartObject(true);
  json.NumberVariableOut("COP1LC", state.COP1LC, true);
  json.NumberVariableOut("COP2LC", state.COP2LC);
  json.NumberVariableOut("COPLC", state.COPLC);
  json.NumberVariableOut("COPCON", state.COPCON);
  json.NumberVariableOut("InstructionStart", state.InstructionStart);
  json.NumberVariableOut("RunState", state.RunState);
  json.EndObject();

  res.set_content(json.Get(), "application/json");
}

void RemoteDebuggerAPI::GetChipsetCopperDisassembly(const Request& req, Response& res)
{
  ULO disasm_pc = chipsetMaskPtr((ULO)stoi(req.matches[1].str()));
  ULO count = (ULO)stoi(req.matches[2].str());
  JSON json;

  json.StartObject(true);
  json.StartArray("Lines");

  for (unsigned int i = 0; i < count; i++)
  {
    auto disassembly = VM->Chipset.Copper.GetDisassembly(disasm_pc);

    json.StartObject(i == 0);
    json.NumberVariableOut("Address", disassembly.Address, true);
    json.NumberVariableOut("IR1", disassembly.IR1);
    json.NumberVariableOut("IR2", disassembly.IR2);
    json.NumberVariableOut("Type", disassembly.Type);
    json.StringVariableOut("Information", disassembly.Information);
    json.EndObject();

    disasm_pc = chipsetMaskPtr(disassembly.Address + 4);
  }
  json.EndArray();
  json.EndObject();

  res.set_content(json.Get(), "application/json");
}

void RemoteDebuggerAPI::GetEvents(const Request& req, Response& res)
{
  auto eventLog = VM->EventLog.GetEvents();
  bool isFirst = true;
  JSON json;

  json.StartObject(true);
  json.StartArray("EventEntries");

  for (const auto& entry : eventLog)
  {
    json.StartObject(isFirst);
    json.NumberVariableOut("FrameNumber", entry.FrameNumber, true);
    json.NumberVariableOut("LineNumber", entry.LineNumber);
    json.NumberVariableOut("BaseCycle", entry.BaseCycle);
    json.StringVariableOut("Source", entry.Source);
    json.StringVariableOut("Description", entry.Description);
    json.EndObject();

    isFirst = false;
  }

  json.EndArray();
  json.EndObject();

  res.set_content(json.Get(), "application/json");
}

DWORD WINAPI webServerStart(LPVOID in)
{
  winDrvSetThreadName(-1, "WinFellow DebugServer");

  auto* webServer = (RemoteDebugger*)in;
  webServer->StartWebServer();

  return 0;
}

bool RemoteDebugger::StartWebServer()
{
  Service->Log.AddLog("Debugger: Registering routes\n");

  // Commands
  _serverProvider->Server.Get("/Fellow/Debugger/run", [this](const Request& req, Response& res) { _api->Run(req, res); });
  _serverProvider->Server.Get("/Fellow/Debugger/run/instruction", [this](const Request& req, Response& res) { _api->RunInstruction(req, res); });
  _serverProvider->Server.Get("/Fellow/Debugger/run/over/instruction", [this](const Request& req, Response& res) { _api->RunOverInstruction(req, res); });
  _serverProvider->Server.Get("/Fellow/Debugger/run/over/line", [this](const Request& req, Response& res) { _api->RunOverLine(req, res); });
  _serverProvider->Server.Get("/Fellow/Debugger/run/over/frame", [this](const Request& req, Response& res) { _api->RunOverFrame(req, res); });
  _serverProvider->Server.Get("/Fellow/Debugger/break", [this](const Request& req, Response& res) { _api->Break(req, res); });
  _serverProvider->Server.Get("/Fellow/Debugger/exit", [this](const Request& req, Response& res) { Stop(); });

  // Queries
  _serverProvider->Server.Get("/Fellow/Debugger/cpu/state", [this](const Request& req, Response& res) { _api->GetCPUState(req, res); });
  _serverProvider->Server.Get(R"(/Fellow/Debugger/cpu/disassembly/(\d+)/(\d+))", [this](const Request& req, Response& res) { _api->GetCPUDisassembly(req, res); });
  _serverProvider->Server.Get("/Fellow/Debugger/clocks", [this](const Request& req, Response& res) { _api->GetClocks(req, res); });
  _serverProvider->Server.Get("/Fellow/Debugger/chipset/display/state", [this](const Request& req, Response& res) { _api->GetChipsetDisplayState(req, res); });
  _serverProvider->Server.Get("/Fellow/Debugger/chipset/copper/state", [this](const Request& req, Response& res) { _api->GetChipsetCopperState(req, res); });
  _serverProvider->Server.Get(R"(/Fellow/Debugger/chipset/copper/disassembly/(\d+)/(\d+))", [this](const Request& req, Response& res) { _api->GetChipsetCopperDisassembly(req, res); });

  // Events
  _serverProvider->Server.Get("/Fellow/Debugger/events", [this](const Request& req, Response& res) { _api->GetEvents(req, res); });

  Service->Log.AddLog("Debugger: Listening\n");

  _serverProvider->Server.listen("localhost", 23875);

  Service->Log.AddLog("Debugger: Listen ended\n");

  return true;
}

bool RemoteDebugger::Start()
{
  Service->Log.AddLog("Debugger: Starting web-server\n");

  bool serverStarted = winDrvRunInNewThread(webServerStart, this);
  if (!serverStarted)
  {
    Service->Log.AddLog("Debugger: Failed to start web-server thread\n");
    return false;
  }

  Service->Log.AddLog("Debugger: Starting debugger client\n");

  bool debuggerClientStarted = winDrvDebugStartClient();

  Service->Log.AddLog("Debugger: Starting debug message loop\n");

  winDrvDebugMessageLoop();
  return true;
}

void RemoteDebugger::Stop()
{
  Service->Log.AddLog("Debugger: Stopping web-server\n");

  _serverProvider->Server.stop();

  Service->Log.AddLog("Debugger: Stopping debugger message loop\n");

  winDrvDebugStopMessageLoop();

  Service->Log.AddLog("Debugger: Web-server stopped\n");
}

RemoteDebugger::RemoteDebugger()
{
  _serverProvider = new HttpServerProvider();
  _api = new RemoteDebuggerAPI();
}

RemoteDebugger::~RemoteDebugger()
{
  delete _serverProvider;
  delete _api;
}
