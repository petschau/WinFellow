#ifndef SPRITEMERGER_H
#define SPRITEMERGER_H

#include "DEFS.H"

class SpriteMerger
{
private:
  /*===========================================================================*/
  /* Used when sprites are merged with the line1 arrays                        */
  /* syntax: spritetranslate[0 - behind, 1 - front][bitplanedata][spritedata]  */
  /*===========================================================================*/

  static UBY sprite_translate[2][256][256];

public:
  static void MergeLores(ULO sprite_number, UBY *playfield, UBY *sprite, ULO pixel_count);
  static void MergeHires(ULO sprite_number, UBY *playfield, UBY *sprite, ULO pixel_count);
  static void MergeHam(ULO sprite_number, UBY *playfield, UBY *ham_sprites_playfield, UBY *sprite, ULO pixel_count);

  static void Initialize();
};

#endif
