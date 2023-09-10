#include "SpriteMerger.h"
#include "GRAPH.H"
#include "CoreHost.h"

UBY SpriteMerger::sprite_translate[2][256][256];

void SpriteMerger::MergeLores(ULO sprite_number, UBY *playfield, UBY *sprite, ULO pixel_count)
{
  ULO in_front = ((_core.Registers.BplCon2 & 0x38) > (4 * sprite_number)) ? 1 : 0;

  for (ULO i = 0; i < pixel_count; ++i)
  {
    *playfield++ = sprite_translate[in_front][*playfield][*sprite++];
  }
}


void SpriteMerger::MergeHires(ULO sprite_number, UBY *playfield, UBY *sprite, ULO pixel_count)
{
  ULO in_front = ((_core.Registers.BplCon2 & 0x38) >(4 * sprite_number)) ? 1 : 0;

  for (ULO i = 0; i < pixel_count; ++i)
  {
    *playfield++ = sprite_translate[in_front][*playfield][*sprite];
    *playfield++ = sprite_translate[in_front][*playfield][*sprite++];
  }
}

void SpriteMerger::MergeHam(ULO sprite_number, UBY *playfield, UBY *ham_sprites_playfield, UBY *sprite, ULO pixel_count)
{
  ULO in_front = ((_core.Registers.BplCon2 & 0x38) >(4 * sprite_number)) ? 1 : 0;

  for (ULO i = 0; i < pixel_count; ++i)
  {
    *ham_sprites_playfield++ = sprite_translate[in_front][*playfield][*sprite++];
  }
}

void SpriteMerger::Initialize()
{
  for (ULO k = 0; k < 2; k++)
  {
    for (ULO i = 0; i < 256; i++)
    {
      for (ULO j = 0; j < 256; j++)
      {
        ULO l;
        if (k == 0) l = (i == 0) ? j : i;
        else l = (j == 0) ? i : j;
        sprite_translate[k][i][j] = (UBY)l;
      }
    }
  }
}
