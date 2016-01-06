#include "DEFS.H"
#include "SpriteP2CDecoder.h"

sprite_deco SpriteP2CDecoder::sprite_deco4[4][2][256];
sprite_deco SpriteP2CDecoder::sprite_deco16[4][256];

void SpriteP2CDecoder::Decode4(unsigned int sprite_number, ULO *chunky_destination, UWO data1, UWO data2)
{
  ULO sprite_class = sprite_number >> 1;
  UBY* word[2] =
  {
    (UBY *)&data1,
    (UBY *)&data2
  };
  ULO planardata = *word[0]++;
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

void SpriteP2CDecoder::Decode16(ULO *chunky_destination, UWO data1, UWO data2, UWO data3, UWO data4)
{
  UBY *word[4] =
  {
    (UBY*)&data1,
    (UBY*)&data2,
    (UBY*)&data3,
    (UBY*)&data4
  };
  ULO planardata = *word[0]++;
  chunky_destination[2] = sprite_deco16[0][planardata].i32[0];
  chunky_destination[3] = sprite_deco16[0][planardata].i32[1];

  planardata = *word[0];
  chunky_destination[0] = sprite_deco16[0][planardata].i32[0];
  chunky_destination[1] = sprite_deco16[0][planardata].i32[1];

  for (UBY bpl = 1; bpl < 4; bpl++)
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
  for (ULO m = 0; m < 4; m++)
  {
    for (ULO n = 0; n < 2; n++)
    {
      for (ULO q = 0; q < 256; q++)
      {
        for (ULO p = 0; p < 8; p++)
        {
          sprite_deco4[m][n][q].i8[p] = (UBY)(((q & (0x80 >> p)) == 0) ? 0 : (((m + 4) << 4) | (1 << (n + 2))));
        }
      }
    }
  }

  for (ULO n = 0; n < 4; n++)
  {
    for (ULO q = 0; q < 256; q++)
    {
      for (ULO p = 0; p < 8; p++)
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
