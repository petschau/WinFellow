#include "fellow/debug/console/CPUWriterUtils.h"

using namespace std;
using namespace fellow::api::vm;

void CPUWriterUtils::WriteCPURegisters(DebugWriter &writer, const M68KRegisters &registers)
{
  writer.String("D0:").Hex32Vector(registers.DataRegisters).String(":D7").Endl();
  writer.String("A0:").Hex32Vector(registers.AddressRegisters).String(":A7").Endl();

  writer.String("PC:").Hex32(registers.PC);
  writer.String(" USP:").Hex32(registers.USP);
  writer.String(" SSP:").Hex32(registers.SSP);
  writer.String(" MSP:").Hex32(registers.MSP);
  writer.String(" ISP:").Hex32(registers.ISP);
  writer.String(" VBR:").Hex32(registers.VBR);
  writer.Endl();

  writer.String("SR:").Hex16(registers.SR);
  writer.String(" T:").NumberBits(registers.SR, 14, 2);
  writer.String(" S:").NumberBit(registers.SR, 13);
  writer.String(" M:").NumberBit(registers.SR, 12);
  writer.String(" I:").NumberBits(registers.SR, 8, 3);
  writer.String(" X:").NumberBit(registers.SR, 4);
  writer.String(" N:").NumberBit(registers.SR, 3);
  writer.String(" Z:").NumberBit(registers.SR, 2);
  writer.String(" V:").NumberBit(registers.SR, 1);
  writer.String(" C:").NumberBit(registers.SR, 0);
  writer.Endl();
}

void CPUWriterUtils::WriteCPUDisassemblyLine(DebugWriter &writer, const M68KDisassemblyLine &line)
{
  size_t maxDataLength = line.Data.length();
  size_t maxInstructionLength = line.Instruction.length();

  //  size_t maxDataLength = writer.GetMaxLength(line, &M68KDisassemblyLine::GetData);
  //size_t maxInstructionLength = writer.GetMaxLength(line, &M68KDisassemblyLine::GetInstruction);
  WriteCPUDisassemblyLine(writer, line, maxDataLength, maxInstructionLength);
}

void CPUWriterUtils::WriteCPUDisassemblyLine(DebugWriter &writer, const M68KDisassemblyLine &line, size_t maxDataLength, size_t maxInstructionLength)
{
  writer.Hex32(line.Address);
  writer.Char(' ');
  writer.StringLeft(line.Data, maxDataLength);
  writer.Char(' ');
  writer.StringLeft(line.Instruction, maxInstructionLength);
  writer.Char(' ');
  writer.String(line.Operands);
  writer.Endl();
}