#pragma once

#include <cstdint>

class DualPlayfieldMapper
{
private:
  static uint8_t _mappingTable[2][256][256]; // Usage: _mappingTable[0 - PF1 behind, 1 - PF2 behind][PF1data][PF2data]
  const uint8_t *_currentMapping;

public:
  static const uint8_t *const GetMappingPtr(const bool isPlayfield1Pri);
  static void InitializeMappingTable();

  uint8_t Map(const uint8_t oddPlayfieldColorIndex, const uint8_t evenPlayfieldColorIndex) const;

  DualPlayfieldMapper(const bool isPlayfield1Pri);
};
