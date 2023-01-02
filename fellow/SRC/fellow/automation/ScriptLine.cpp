#include "fellow/automation/ScriptLine.h"

using namespace std;

ScriptLine::ScriptLine(ULL frameNumber, ULO lineNumber, const string &command, const string &parameters)
  : FrameNumber(frameNumber), LineNumber(lineNumber), Command(command), Parameters(parameters)
{
}
