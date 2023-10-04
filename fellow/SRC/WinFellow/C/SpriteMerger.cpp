#include "SpriteMerger.h"
#include "GRAPH.H"
#include "VirtualHost/Core.h"

uint8_t SpriteMerger::sprite_translate[2][256][256];

void SpriteMerger::MergeLores(uint32_t sprite_number, uint8_t* playfield, uint8_t* sprite, uint32_t pixel_count)
{
  uint32_t in_front = ((_core.Registers.BplCon2 & 0x38) > (4 * sprite_number)) ? 1 : 0;

  for (uint32_t i = 0; i < pixel_count; ++i)
  {
    *playfield++ = sprite_translate[in_front][*playfield][*sprite++];
  }
}


void SpriteMerger::MergeHires(uint32_t sprite_number, uint8_t* playfield, uint8_t* sprite, uint32_t pixel_count)
{
  uint32_t in_front = ((_core.Registers.BplCon2 & 0x38) > (4 * sprite_number)) ? 1 : 0;

  for (uint32_t i = 0; i < pixel_count; ++i)
  {
    *playfield++ = sprite_translate[in_front][*playfield][*sprite];
    *playfield++ = sprite_translate[in_front][*playfield][*sprite++];
  }
}

void SpriteMerger::MergeHam(uint32_t sprite_number, uint8_t* playfield, uint8_t* ham_sprites_playfield, uint8_t* sprite, uint32_t pixel_count)
{
  uint32_t in_front = ((_core.Registers.BplCon2 & 0x38) > (4 * sprite_number)) ? 1 : 0;

  for (uint32_t i = 0; i < pixel_count; ++i)
  {
    *ham_sprites_playfield++ = sprite_translate[in_front][*playfield][*sprite++];
  }
}

void SpriteMerger::Initialize()
{
  for (uint32_t k = 0; k < 2; k++)
  {
    for (uint32_t i = 0; i < 256; i++)
    {
      for (uint32_t j = 0; j < 256; j++)
      {
        uint32_t l;
        if (k == 0) l = (i == 0) ? j : i;
        else l = (j == 0) ? i : j;
        sprite_translate[k][i][j] = (uint8_t)l;
      }
    }
  }
}
