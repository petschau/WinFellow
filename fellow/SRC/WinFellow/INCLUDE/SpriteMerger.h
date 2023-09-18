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

  static uint8_t sprite_translate[2][256][256];

public:
  static void MergeLores(uint32_t sprite_number, uint8_t *playfield, uint8_t *sprite, uint32_t pixel_count);
  static void MergeHires(uint32_t sprite_number, uint8_t *playfield, uint8_t *sprite, uint32_t pixel_count);
  static void MergeHam(uint32_t sprite_number, uint8_t *playfield, uint8_t *ham_sprites_playfield, uint8_t *sprite, uint32_t pixel_count);

  static void Initialize();
};

#endif
