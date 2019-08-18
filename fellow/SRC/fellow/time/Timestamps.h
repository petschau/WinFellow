#pragma once

#include "fellow/api/defs.h"

union ChipBusTimestamp {
  ULO LineAndCycle[2]{};
  ULL Timestamp;

  ULO GetLine() const;
  ULO GetCycle() const;
  void Clear();
  bool HasValue() const;
  void Add(UWO cycles);
  void Set(ULO line, ULO cycle);
  void operator=(const ChipBusTimestamp &other);
  bool operator==(const ChipBusTimestamp &other) const;
  bool operator>(const ChipBusTimestamp &other) const;
  ULO ToBaseClockTimestamp() const;
  static ULO ToBaseClockTimestamp(ULO line, ULO cycle);
  static ULO GetCyclesInLine();

  ChipBusTimestamp(UWO line, UWO cycle);
  ChipBusTimestamp(ULO timestamp);
  ChipBusTimestamp(const ChipBusTimestamp &timestamp);
  ChipBusTimestamp();
};

struct SHResTimestamp
{
  unsigned int Line;
  unsigned int Pixel;
  ULO Cylinder() const;
  ULO ChipCycle() const;

  void Set(ULO line, ULO pixel);
  ULO GetUnwrappedLine() const;
  ULO GetUnwrappedPixel() const;
  ULO GetUnwrappedFirstPixelInNextCylinder() const;
  void AddWithLineWrap(ULO count);

  bool operator==(const SHResTimestamp &timestamp) const;
  bool operator<=(const SHResTimestamp &timestamp) const;
  ULO ToBaseClockTimestamp() const;

  SHResTimestamp() = default;
  SHResTimestamp(ULO line, ULO pixel);
  SHResTimestamp(const SHResTimestamp &timestamp);
  SHResTimestamp(const SHResTimestamp &timestamp, ULO addCount);
};
