#pragma once

#include "fellow/api/defs.h"
#include <string>

struct ScriptLine
{
  ULL FrameNumber;
  ULO LineNumber;
  std::string Command;
  std::string Parameters;

  ScriptLine(ULL frameNumber, ULO lineNumber, const std::string &command, const std::string &parameters);
};
