#pragma once

struct BitplaneFetchStep
{
  unsigned int CycleOffset;
  unsigned int BitplaneIndex;
  bool CanAddModulo;

  void Initialize(unsigned int cycleOffset, unsigned int bitplaneIndex, bool CanAddModulo);

  BitplaneFetchStep();
};

struct BitplaneFetchProgram
{
  BitplaneFetchStep Steps[8];
  unsigned int StepCount;

  void AddStep(unsigned int cycleOffset, unsigned int bitplaneIndex, bool canAddModulo);

  BitplaneFetchProgram();
};

class BitplaneFetchPrograms
{
private:
  BitplaneFetchProgram FetchPrograms[2][8];

  void InitializeFetchPrograms();

public:
  const BitplaneFetchProgram *GetFetchProgram(bool isLores, unsigned int bitplaneCount) const;

  BitplaneFetchPrograms();
};

class BitplaneFetchProgramRunner
{
private:
  const BitplaneFetchProgram *_program;
  unsigned int _currentStep;

public:
  void StartProgram(const BitplaneFetchProgram *program);

  unsigned int GetCycleOffset() const;
  unsigned int GetBitplaneIndex() const;
  bool GetCanAddModulo() const;
  void NextStep();
  bool IsEnded() const;

  BitplaneFetchProgramRunner();
};
