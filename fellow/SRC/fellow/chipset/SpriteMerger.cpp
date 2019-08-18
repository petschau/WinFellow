#include "fellow/chipset/SpriteMerger.h"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/BitplaneUtility.h"

UBY SpriteMerger::_spriteTranslate[2][256][256];

void SpriteMerger::MergeLores(unsigned int spriteNumber, UBY *playfield, UBY *sprite, ULO spritePixelCount)
{
  const unsigned int inFrontIndex = IsInFrontOfPlayfield2(spriteNumber) ? 1 : 0;
  MergeLoresWithPlayfield(inFrontIndex, playfield, sprite, spritePixelCount);
}

void SpriteMerger::MergeHires(unsigned int spriteNumber, UBY *playfield, UBY *sprite, ULO spritePixelCount)
{
  const unsigned int inFrontIndex = IsInFrontOfPlayfield2(spriteNumber) ? 1 : 0;
  MergeHiresWithPlayfield(inFrontIndex, playfield, sprite, spritePixelCount);
}

void SpriteMerger::MergeHam(unsigned int spriteNumber, UBY *hamSpritesPlayfield, UBY *sprite, ULO spritePixelCount)
{
  for (unsigned int i = 0; i < spritePixelCount; ++i)
  {
    *hamSpritesPlayfield++ = _spriteTranslate[1][*hamSpritesPlayfield][*sprite++];
  }
}

void SpriteMerger::MergeDualLores(unsigned int spriteNumber, UBY *playfield1, UBY *playfield2, UBY *sprite, ULO spritePixelCount)
{
  UBY *playfield;
  const unsigned int inFrontIndex = DecidePositionVsPlayfields(spriteNumber, playfield1, playfield2, &playfield);
  MergeLoresWithPlayfield(inFrontIndex, playfield, sprite, spritePixelCount);
}

void SpriteMerger::MergeDualHires(unsigned int spriteNumber, UBY *playfield1, UBY *playfield2, UBY *sprite, ULO spritePixelCount)
{
  UBY *playfield;
  const unsigned int inFrontIndex = DecidePositionVsPlayfields(spriteNumber, playfield1, playfield2, &playfield);
  MergeHiresWithPlayfield(inFrontIndex, playfield, sprite, spritePixelCount);
}

void SpriteMerger::MergeLoresWithPlayfield(unsigned int inFrontIndex, UBY *playfield, UBY *sprite, ULO spritePixelCount)
{
  for (unsigned int i = 0; i < spritePixelCount; ++i)
  {
    *playfield++ = _spriteTranslate[inFrontIndex][*playfield][*sprite++];
  }
}

void SpriteMerger::MergeHiresWithPlayfield(unsigned int inFrontIndex, UBY *playfield, UBY *sprite, ULO spritePixelCount)
{
  for (unsigned int i = 0; i < spritePixelCount; ++i)
  {
    const UBY spritePixel = *sprite++;
    *playfield++ = _spriteTranslate[inFrontIndex][*playfield][spritePixel];
    *playfield++ = _spriteTranslate[inFrontIndex][*playfield][spritePixel];
  }
}

bool SpriteMerger::IsInFrontOfPlayfield1(unsigned int spriteNumber)
{
  return ((unsigned int)(bitplane_registers.bplcon2 & 0x7) << 1) > spriteNumber;
}

bool SpriteMerger::IsInFrontOfPlayfield2(unsigned int spriteNumber)
{
  return ((unsigned int)(bitplane_registers.bplcon2 & 0x38)) > (4 * spriteNumber);
}

unsigned int SpriteMerger::DecidePositionVsPlayfields(unsigned int spriteNumber, UBY *playfield1, UBY *playfield2, UBY **playfield)
{
  // determine whetever this sprite is in front of playfield 1 and/or in front or behind playfield 2
  if (BitplaneUtility::IsPlayfield1Pri())
  {
    // playfield 1 is in front of playfield 2
    if (IsInFrontOfPlayfield1(spriteNumber))
    {
      // current sprite is in front of playfield 1, and thus also in front of playfield 2
      *playfield = playfield1;
      return 1;
    }
    else
    {
      // current sprite is behind playfield 1, decide position related to playfield 2
      *playfield = playfield2;
      return IsInFrontOfPlayfield2(spriteNumber) ? 1 : 0;
    }
  }

  // playfield 2 is in front of playfield 1
  if (IsInFrontOfPlayfield2(spriteNumber))
  {
    // current sprite is in front of playfield 2, and thus also in front of playfield 1
    *playfield = playfield2;
    return 1;
  }

  // current sprite is behind of playfield 2, decide position related to playfield 1
  *playfield = playfield1;
  return IsInFrontOfPlayfield1(spriteNumber) ? 1 : 0;
}

void SpriteMerger::Initialize()
{
  for (unsigned int k = 0; k < 2; ++k)
  {
    for (unsigned int i = 0; i < 256; ++i)
    {
      for (unsigned int j = 0; j < 256; ++j)
      {
        _spriteTranslate[k][i][j] = (UBY)((k == 0) ? ((i == 0) ? j : i) : ((j == 0) ? i : j));
      }
    }
  }
}
