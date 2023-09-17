#include "DEFS.H"
#include "Script.h"
#include "BUS.H"
#include "KBD.H"
#include "GAMEPORT.H"

ScriptLine::ScriptLine(uint64_t frameNumber, uint32_t lineNumber, const string& command, const string& parameters)
  : FrameNumber(frameNumber), LineNumber(lineNumber), Command(command), Parameters(parameters)
{
}

void Script::ExecuteMouseCommand(const string& parameters)
{
  uint32_t port;
  LON x, y;
  BOOLE button1, button2, button3;

  sscanf(parameters.c_str(), "%d %d %d %d %d %d", &port, &x, &y, &button1, &button2, &button3);

  gameportMouseHandler((port == 0) ? GP_MOUSE0 : GP_MOUSE1, x, y, button1, button2, button3);
}

void Script::ExecuteKeyCommand(const string& parameters)
{
  kbdKeyAdd(atoi(parameters.c_str()));
}

void Script::ExecuteJoystickCommand(const string& parameters)
{
  uint32_t port;
  BOOLE left, up, right, down, button1, button2;
  sscanf(parameters.c_str(), "%d %d %d %d %d %d %d", &port, &left, &up, &right, &down, &button1, &button2);
  gameportJoystickHandler((port == 0) ? GP_JOYKEY0 : GP_JOYKEY1, left, up, right, down, button1, button2);
}

void Script::ExecuteEmulatorActionCommand(const string& parameters)
{
  uint8_t eventId = GetIdForAction(parameters);
  if (eventId != 255)
  {
    kbdEventEOFAdd(eventId);
  }
}

void Script::Execute(const ScriptLine& line)
{
  if (line.Command == MouseCommand)
  {
    ExecuteMouseCommand(line.Parameters);
  }
  else if (line.Command == KeyCommand)
  {
    ExecuteKeyCommand(line.Parameters);
  }
  else if (line.Command == JoystickCommand)
  {
    ExecuteJoystickCommand(line.Parameters);
  }
  else if (line.Command == EmulatorActionCommand)
  {
    ExecuteEmulatorActionCommand(line.Parameters);
  }
}

void Script::ExecuteUntil(uint64_t frameNumber, uint32_t lineNumber)
{
  if (_lines.size() == 0)
  {
    return;
  }
  while (_lines.size() > _nextLine && (_lines[_nextLine].FrameNumber < frameNumber || (_lines[_nextLine].FrameNumber == frameNumber && _lines[_nextLine].LineNumber <= lineNumber)))
  {
    Execute(_lines[_nextLine]);
    _nextLine++;
  }
}

void Script::RecordKey(uint8_t keyCode)
{
  char parameters[32];
  sprintf(parameters, "%u", (uint32_t)keyCode);
  _lines.push_back(ScriptLine(busGetRasterFrameCount(), busGetRasterY(), KeyCommand, parameters));
}

void Script::RecordMouse(gameport_inputs mousedev, LON x, LON y, BOOLE button1, BOOLE button2, BOOLE button3)
{
  uint32_t port = (mousedev == GP_MOUSE0) ? 0 : 1;
  char parameters[128];
  sprintf(parameters, "%u %d %d %u %u %u", port, x, y, button1, button2, button3);
  _lines.push_back(ScriptLine(busGetRasterFrameCount(), busGetRasterY(), MouseCommand, parameters));
}

void Script::RecordJoystick(gameport_inputs joydev, BOOLE left, BOOLE up, BOOLE right, BOOLE down, BOOLE button1, BOOLE button2)
{
  uint32_t port = (joydev == GP_JOYKEY0 || joydev == GP_ANALOG0) ? 0 : 1;
  char parameters[128];
  sprintf(parameters, "%u %u %u %u %u %u %u", port, left, up, right, down, button1, button2);
  _lines.push_back(ScriptLine(busGetRasterFrameCount(), busGetRasterY(), JoystickCommand, parameters));
}

string Script::GetStringForAction(kbd_event action)
{
  switch (action)
  {
  case EVENT_EXIT:
    return "EVENT_EXIT";
  case EVENT_DF1_INTO_DF0:
    return "EVENT_DF1_INTO_DF0";
  case EVENT_DF2_INTO_DF0:
    return "EVENT_DF2_INTO_DF0";
  case EVENT_DF3_INTO_DF0:
    return "EVENT_DF3_INTO_DF0";
  }
  return "";
}

uint8_t Script::GetIdForAction(const string& action)
{
  uint8_t eventId = 255;
  if (action == "EVENT_EXIT")
  {
    eventId = EVENT_EXIT;
  }
  else if (action == "EVENT_DF1_INTO_DF0")
  {
    eventId = EVENT_DF1_INTO_DF0;
  }
  else if (action == "EVENT_DF2_INTO_DF0")
  {
    eventId = EVENT_DF2_INTO_DF0;
  }
  else if (action == "EVENT_DF3_INTO_DF0")
  {
    eventId = EVENT_DF3_INTO_DF0;
  }
  return eventId;
}

void Script::RecordEmulatorAction(kbd_event action)
{
  string actionString = GetStringForAction(action);
  if (actionString == "")
  {
    return;
  }

  _lines.push_back(ScriptLine(busGetRasterFrameCount(), busGetRasterY(), EmulatorActionCommand, actionString));
}

void Script::Load(const string& filename)
{
  _nextLine = 0;
  _lines.clear();

  FILE *F = fopen(filename.c_str(), "r");
  if (F == nullptr)
  {
    return;
  }

  char line[256];

  while (!feof(F))
  {
    if (fgets(line, 256, F) != nullptr)
    {
      string s(line);
      size_t firstComma = s.find_first_of(',');
      size_t secondComma = s.find(',', firstComma + 1);
      size_t thirdComma = s.find(',', secondComma + 1);

      string frameNumber = s.substr(0, firstComma);
      string lineNumber = s.substr(firstComma + 1, secondComma - firstComma - 1);
      string command = s.substr(secondComma + 1, thirdComma - secondComma - 1);
      string parameters = s.substr(thirdComma + 1, s.length() - thirdComma - 2);

      _lines.push_back(ScriptLine(_atoi64(frameNumber.c_str()), atoi(lineNumber.c_str()), command, parameters));
    }
  }
  fclose(F);
}

void Script::Save(const string& filename)
{
  FILE *F = fopen(filename.c_str(), "w");

  for (const ScriptLine& line : _lines)
  {
    fprintf(F, "%I64d,%d,%s,%s\n", line.FrameNumber, line.LineNumber, line.Command.c_str(), line.Parameters.c_str());
  }
  fclose(F);
}

Script::Script()
  : _nextLine(0)
{
}

Script::~Script()
{
}
