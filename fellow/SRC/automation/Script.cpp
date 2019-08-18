#include "fellow/api/defs.h"
#include "Script.h"
#include "fellow/scheduler/Scheduler.h"
#include "KBD.H"
#include "GAMEPORT.H"

ScriptLine::ScriptLine(ULL frameNumber, ULO lineNumber, const string &command, const string &parameters) : FrameNumber(frameNumber), LineNumber(lineNumber), Command(command), Parameters(parameters)
{
}

void Script::ExecuteMouseCommand(const string &parameters)
{
  ULO port;
  LON x, y;
  BOOLE button1, button2, button3;

  sscanf(parameters.c_str(), "%u %d %d %d %d %d", &port, &x, &y, &button1, &button2, &button3);

  gameportMouseHandler((port == 0) ? gameport_inputs::GP_MOUSE0 : gameport_inputs::GP_MOUSE1, x, y, button1, button2, button3);
}

void Script::ExecuteKeyCommand(const string &parameters)
{
  kbdKeyAdd(atoi(parameters.c_str()));
}

void Script::ExecuteJoystickCommand(const string &parameters)
{
  ULO port;
  BOOLE left, up, right, down, button1, button2;
  sscanf(parameters.c_str(), "%u %d %d %d %d %d %d", &port, &left, &up, &right, &down, &button1, &button2);
  gameportJoystickHandler((port == 0) ? gameport_inputs::GP_JOYKEY0 : gameport_inputs::GP_JOYKEY1, left, up, right, down, button1, button2);
}

void Script::ExecuteEmulatorActionCommand(const string &parameters)
{
  kbd_event kbdEvent = GetKbdEventForAction(parameters);
  if (kbdEvent != kbd_event::EVENT_NONE)
  {
    kbdEventEOFAdd(kbdEvent);
  }
}

void Script::Execute(const ScriptLine &line)
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

void Script::ExecuteUntil(ULL frameNumber, ULO lineNumber)
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

void Script::RecordKey(UBY keyCode)
{
  char parameters[32];
  sprintf(parameters, "%u", (ULO)keyCode);
  _lines.push_back(ScriptLine(scheduler.GetRasterFrameCount(), scheduler.GetRasterY(), KeyCommand, parameters));
}

void Script::RecordMouse(gameport_inputs mousedev, LON x, LON y, BOOLE button1, BOOLE button2, BOOLE button3)
{
  ULO port = (mousedev == gameport_inputs::GP_MOUSE0) ? 0 : 1;
  char parameters[128];
  sprintf(parameters, "%u %d %d %d %d %d", port, x, y, button1, button2, button3);
  _lines.push_back(ScriptLine(scheduler.GetRasterFrameCount(), scheduler.GetRasterY(), MouseCommand, parameters));
}

void Script::RecordJoystick(gameport_inputs joydev, BOOLE left, BOOLE up, BOOLE right, BOOLE down, BOOLE button1, BOOLE button2)
{
  ULO port = (joydev == gameport_inputs::GP_JOYKEY0 || joydev == gameport_inputs::GP_ANALOG0) ? 0 : 1;
  char parameters[128];
  sprintf(parameters, "%u %d %d %d %d %d %d", port, left, up, right, down, button1, button2);
  _lines.push_back(ScriptLine(scheduler.GetRasterFrameCount(), scheduler.GetRasterY(), JoystickCommand, parameters));
}

string Script::GetStringForAction(kbd_event action)
{
  switch (action)
  {
    case kbd_event::EVENT_EXIT: return "EVENT_EXIT";
    case kbd_event::EVENT_DF1_INTO_DF0: return "EVENT_DF1_INTO_DF0";
    case kbd_event::EVENT_DF2_INTO_DF0: return "EVENT_DF2_INTO_DF0";
    case kbd_event::EVENT_DF3_INTO_DF0: return "EVENT_DF3_INTO_DF0";
  }
  return "";
}

kbd_event Script::GetKbdEventForAction(const string &action)
{
  kbd_event kbdEvent = kbd_event::EVENT_NONE;
  if (action == "EVENT_EXIT")
  {
    kbdEvent = kbd_event::EVENT_EXIT;
  }
  else if (action == "EVENT_DF1_INTO_DF0")
  {
    kbdEvent = kbd_event::EVENT_DF1_INTO_DF0;
  }
  else if (action == "EVENT_DF2_INTO_DF0")
  {
    kbdEvent = kbd_event::EVENT_DF2_INTO_DF0;
  }
  else if (action == "EVENT_DF3_INTO_DF0")
  {
    kbdEvent = kbd_event::EVENT_DF3_INTO_DF0;
  }
  return kbdEvent;
}

void Script::RecordEmulatorAction(kbd_event action)
{
  string actionString = GetStringForAction(action);
  if (actionString == "")
  {
    return;
  }

  _lines.push_back(ScriptLine(scheduler.GetRasterFrameCount(), scheduler.GetRasterY(), EmulatorActionCommand, actionString));
}

void Script::Load(const string &filename)
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

      _lines.push_back(ScriptLine(atoll(frameNumber.c_str()), atoi(lineNumber.c_str()), command, parameters));
    }
  }
  fclose(F);
}

void Script::Save(const string &filename)
{
  FILE *F = fopen(filename.c_str(), "w");

  for (const ScriptLine &line : _lines)
  {
    fprintf(F, "%llu,%u,%s,%s\n", line.FrameNumber, line.LineNumber, line.Command.c_str(), line.Parameters.c_str());
  }
  fclose(F);
}

Script::Script() : _nextLine(0)
{
}

Script::~Script()
{
}
