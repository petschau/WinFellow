#include "Defs.h"
#include "SpriteP2CDecoder.h"

sprite_deco SpriteP2CDecoder::sprite_deco4[4][2][256];
sprite_deco SpriteP2CDecoder::sprite_deco16[4][256];

void SpriteP2CDecoder::Decode4(unsigned int sprite_number, uint32_t *chunky_destination, uint16_t data1, uint16_t data2)
{
  uint32_t sprite_class = sprite_number >> 1;
  uint8_t *word[2] = {(uint8_t *)&data1, (uint8_t *)&data2};
  uint32_t planardata = *word[0]++;
  chunky_destination[2] = sprite_deco4[sprite_class][0][planardata].i32[0];
  chunky_destination[3] = sprite_deco4[sprite_class][0][planardata].i32[1];

  planardata = *word[0];
  chunky_destination[0] = sprite_deco4[sprite_class][0][planardata].i32[0];
  chunky_destination[1] = sprite_deco4[sprite_class][0][planardata].i32[1];

  planardata = *word[1]++;
  chunky_destination[2] |= sprite_deco4[sprite_class][1][planardata].i32[0];
  chunky_destination[3] |= sprite_deco4[sprite_class][1][planardata].i32[1];

  planardata = *word[1];
  chunky_destination[0] |= sprite_deco4[sprite_class][1][planardata].i32[0];
  chunky_destination[1] |= sprite_deco4[sprite_class][1][planardata].i32[1];
}

void SpriteP2CDecoder::Decode16(uint32_t *chunky_destination, uint16_t data1, uint16_t data2, uint16_t data3, uint16_t data4)
{
  uint8_t *word[4] = {(uint8_t *)&data1, (uint8_t *)&data2, (uint8_t *)&data3, (uint8_t *)&data4};
  uint32_t planardata = *word[0]++;
  chunky_destination[2] = sprite_deco16[0][planardata].i32[0];
  chunky_destination[3] = sprite_deco16[0][planardata].i32[1];

  planardata = *word[0];
  chunky_destination[0] = sprite_deco16[0][planardata].i32[0];
  chunky_destination[1] = sprite_deco16[0][planardata].i32[1];

  for (uint8_t bpl = 1; bpl < 4; bpl++)
  {
    planardata = *word[bpl]++;
    chunky_destination[2] |= sprite_deco16[bpl][planardata].i32[0];
    chunky_destination[3] |= sprite_deco16[bpl][planardata].i32[1];

    planardata = *word[bpl];
    chunky_destination[0] |= sprite_deco16[bpl][planardata].i32[0];
    chunky_destination[1] |= sprite_deco16[bpl][planardata].i32[1];
  }
}

void SpriteP2CDecoder::P2CTablesInitialize()
{
  for (uint32_t m = 0; m < 4; m++)
  {
    for (uint32_t n = 0; n < 2; n++)
    {
      for (uint32_t q = 0; q < 256; q++)
      {
        for (uint32_t p = 0; p < 8; p++)
        {
          sprite_deco4[m][n][q].i8[p] = (uint8_t)(((q & (0x80 >> p)) == 0) ? 0 : (((m + 4) << 4) | (1 << (n + 2))));
        }
      }
    }
  }

  for (uint32_t n = 0; n < 4; n++)
  {
    for (uint32_t q = 0; q < 256; q++)
    {
      for (uint32_t p = 0; p < 8; p++)
      {
        sprite_deco16[n][q].i8[p] = ((q & (0x80 >> p)) == 0) ? 0 : (64 | (1 << (n + 2)));
      }
    }
  }
}

void SpriteP2CDecoder::Initialize()
{
  P2CTablesInitialize();
}
