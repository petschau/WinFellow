#pragma once

#include "fellow/debug/console/DebugWriter.h"
#include "fellow/api/VM.h"

class CPUWriterUtils
{
public:
  static void WriteCPURegisters(DebugWriter &writer, const fellow::api::vm::M68KRegisters &registers);
  static void WriteCPUDisassemblyLine(DebugWriter &writer, const fellow::api::vm::M68KDisassemblyLine &line, size_t maxDataLength, size_t maxInstructionLength);
  static void WriteCPUDisassemblyLine(DebugWriter &writer, const fellow::api::vm::M68KDisassemblyLine &line);
};
