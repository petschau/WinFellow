#include "fellow/chipset/BitplaneDMAFetchPrograms.h"

void BitplaneFetchStep::Initialize(unsigned int cycleOffset, unsigned int bitplaneIndex, bool canAddModulo)
{
  CycleOffset = cycleOffset;
  BitplaneIndex = bitplaneIndex;
  CanAddModulo = canAddModulo;
}

BitplaneFetchStep::BitplaneFetchStep() : CycleOffset(0), BitplaneIndex(0), CanAddModulo(false)
{
}

void BitplaneFetchProgram::AddStep(unsigned int cycleOffset, unsigned int bitplaneIndex, bool canAddModulo)
{
  Steps[StepCount].Initialize(cycleOffset, bitplaneIndex, canAddModulo);
  ++StepCount;
}

BitplaneFetchProgram::BitplaneFetchProgram() : StepCount(0)
{
}

void BitplaneFetchPrograms::InitializeFetchPrograms()
{
  // Lores, 1 bitplane
  FetchPrograms[0][1].AddStep(7, 0, true);

  // Lores, 2 bitplanes
  FetchPrograms[0][2].AddStep(3, 1, true);
  FetchPrograms[0][2].AddStep(4, 0, true);

  // Lores, 3 bitplanes
  FetchPrograms[0][3].AddStep(3, 1, true);
  FetchPrograms[0][3].AddStep(2, 2, true);
  FetchPrograms[0][3].AddStep(2, 0, true);

  // Lores, 4 bitplanes
  FetchPrograms[0][4].AddStep(1, 3, true);
  FetchPrograms[0][4].AddStep(2, 1, true);
  FetchPrograms[0][4].AddStep(2, 2, true);
  FetchPrograms[0][4].AddStep(2, 0, true);

  // Lores, 5 bitplanes
  FetchPrograms[0][5].AddStep(1, 3, true);
  FetchPrograms[0][5].AddStep(2, 1, true);
  FetchPrograms[0][5].AddStep(2, 2, true);
  FetchPrograms[0][5].AddStep(1, 4, true);
  FetchPrograms[0][5].AddStep(1, 0, true);

  // Lores, 6 bitplanes
  FetchPrograms[0][6].AddStep(1, 3, true);
  FetchPrograms[0][6].AddStep(1, 5, true);
  FetchPrograms[0][6].AddStep(1, 1, true);
  FetchPrograms[0][6].AddStep(2, 2, true);
  FetchPrograms[0][6].AddStep(1, 4, true);
  FetchPrograms[0][6].AddStep(1, 0, true);

  // Hires, 1 bitplane
  FetchPrograms[1][1].AddStep(3, 0, false);
  FetchPrograms[1][1].AddStep(4, 0, true);

  // Hires, 2 bitplane
  FetchPrograms[1][2].AddStep(2, 1, false);
  FetchPrograms[1][2].AddStep(1, 0, false);
  FetchPrograms[1][2].AddStep(3, 1, true);
  FetchPrograms[1][2].AddStep(1, 0, true);

  // Hires, 3 bitplane
  FetchPrograms[1][3].AddStep(1, 2, false);
  FetchPrograms[1][3].AddStep(1, 1, false);
  FetchPrograms[1][3].AddStep(1, 0, false);
  FetchPrograms[1][3].AddStep(2, 2, true);
  FetchPrograms[1][3].AddStep(1, 1, true);
  FetchPrograms[1][3].AddStep(1, 0, true);

  // Hires, 4 bitplane
  FetchPrograms[1][4].AddStep(0, 3, false);
  FetchPrograms[1][4].AddStep(1, 2, false);
  FetchPrograms[1][4].AddStep(1, 1, false);
  FetchPrograms[1][4].AddStep(1, 0, false);
  FetchPrograms[1][4].AddStep(1, 3, true);
  FetchPrograms[1][4].AddStep(1, 2, true);
  FetchPrograms[1][4].AddStep(1, 1, true);
  FetchPrograms[1][4].AddStep(1, 0, true);
}

const BitplaneFetchProgram *BitplaneFetchPrograms::GetFetchProgram(bool isLores, unsigned int bitplaneCount) const
{
  return &FetchPrograms[isLores ? 0 : 1][bitplaneCount];
}

BitplaneFetchPrograms::BitplaneFetchPrograms()
{
  InitializeFetchPrograms();
}

void BitplaneFetchProgramRunner::StartProgram(const BitplaneFetchProgram *program)
{
  _program = program;
  _currentStep = 0;
}

unsigned int BitplaneFetchProgramRunner::GetCycleOffset() const
{
  return _program->Steps[_currentStep].CycleOffset;
}

unsigned int BitplaneFetchProgramRunner::GetBitplaneIndex() const
{
  return _program->Steps[_currentStep].BitplaneIndex;
}

bool BitplaneFetchProgramRunner::GetCanAddModulo() const
{
  return _program->Steps[_currentStep].CanAddModulo;
}

void BitplaneFetchProgramRunner::NextStep()
{
  ++_currentStep;
}

bool BitplaneFetchProgramRunner::IsEnded() const
{
  return _currentStep == _program->StepCount;
}

BitplaneFetchProgramRunner::BitplaneFetchProgramRunner() : _program(nullptr), _currentStep(0)
{
}
