#pragma once

#include "fellow/api/defs.h"

class SpriteMerger
{
private:
  /*===========================================================================*/
  /* Used when sprites are merged with the line1 arrays                        */
  /* syntax: spritetranslate[0 - behind, 1 - front][bitplanedata][spritedata]  */
  /*===========================================================================*/

  static UBY _spriteTranslate[2][256][256];

  static bool IsInFrontOfPlayfield1(unsigned int spriteNumber);
  static bool IsInFrontOfPlayfield2(unsigned int spriteNumber);
  static unsigned int DecidePositionVsPlayfields(unsigned int spriteNumber, UBY *playfield1, UBY *playfield2, UBY **playfield);

  static void MergeLoresWithPlayfield(unsigned int inFront, UBY *playfield, UBY *sprite, ULO spritePixelCount);
  static void MergeHiresWithPlayfield(unsigned int inFront, UBY *playfield, UBY *sprite, ULO spritePixelCount);

public:
  static void MergeLores(unsigned int spriteNumber, UBY *playfield, UBY *sprite, ULO spritePixelCount);
  static void MergeHires(unsigned int spriteNumber, UBY *playfield, UBY *sprite, ULO spritePixelCount);
  static void MergeDualLores(unsigned int spriteNumber, UBY *playfield1, UBY *playfield2, UBY *sprite, ULO spritePixelCount);
  static void MergeDualHires(unsigned int spriteNumber, UBY *playfield1, UBY *playfield2, UBY *sprite, ULO spritePixelCount);
  static void MergeHam(unsigned int spriteNumber, UBY *hamSpritesPlayfield, UBY *sprite, ULO spritePixelCount);

  static void Initialize();
};
