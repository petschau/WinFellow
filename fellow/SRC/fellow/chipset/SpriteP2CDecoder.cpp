#include "fellow/api/defs.h"
#include "fellow/chipset/SpriteP2CDecoder.h"

sprite_deco SpriteP2CDecoder::_spriteDecode4[4][2][256];
sprite_deco SpriteP2CDecoder::_spriteDecode16[4][256];

void SpriteP2CDecoder::Decode4(unsigned int spriteNumber, ULO *chunkyDestination, UWO data1, UWO data2)
{
  const unsigned int spriteGroup = spriteNumber >> 1;

  unsigned int planardata = data1 >> 8;
  chunkyDestination[0] = _spriteDecode4[spriteGroup][0][planardata].i32[0];
  chunkyDestination[1] = _spriteDecode4[spriteGroup][0][planardata].i32[1];

  planardata = data1 & 0xff;
  chunkyDestination[2] = _spriteDecode4[spriteGroup][0][planardata].i32[0];
  chunkyDestination[3] = _spriteDecode4[spriteGroup][0][planardata].i32[1];

  planardata = data2 >> 8;
  chunkyDestination[0] |= _spriteDecode4[spriteGroup][1][planardata].i32[0];
  chunkyDestination[1] |= _spriteDecode4[spriteGroup][1][planardata].i32[1];

  planardata = data2 & 0xff;
  chunkyDestination[2] |= _spriteDecode4[spriteGroup][1][planardata].i32[0];
  chunkyDestination[3] |= _spriteDecode4[spriteGroup][1][planardata].i32[1];
}

void SpriteP2CDecoder::Decode16(ULO *chunkyDestination, UWO data1, UWO data2, UWO data3, UWO data4)
{
  unsigned int planardata = data1 >> 8;
  chunkyDestination[0] = _spriteDecode16[0][planardata].i32[0];
  chunkyDestination[1] = _spriteDecode16[0][planardata].i32[1];

  planardata = data1 & 0xff;
  chunkyDestination[2] = _spriteDecode16[0][planardata].i32[0];
  chunkyDestination[3] = _spriteDecode16[0][planardata].i32[1];

  planardata = data2 >> 8;
  chunkyDestination[0] |= _spriteDecode16[1][planardata].i32[0];
  chunkyDestination[1] |= _spriteDecode16[1][planardata].i32[1];

  planardata = data2 & 0xff;
  chunkyDestination[2] |= _spriteDecode16[1][planardata].i32[0];
  chunkyDestination[3] |= _spriteDecode16[1][planardata].i32[1];

  planardata = data3 >> 8;
  chunkyDestination[0] |= _spriteDecode16[2][planardata].i32[0];
  chunkyDestination[1] |= _spriteDecode16[2][planardata].i32[1];

  planardata = data3 & 0xff;
  chunkyDestination[2] |= _spriteDecode16[2][planardata].i32[0];
  chunkyDestination[3] |= _spriteDecode16[2][planardata].i32[1];

  planardata = data4 >> 8;
  chunkyDestination[0] |= _spriteDecode16[3][planardata].i32[0];
  chunkyDestination[1] |= _spriteDecode16[3][planardata].i32[1];

  planardata = data4 & 0xff;
  chunkyDestination[2] |= _spriteDecode16[3][planardata].i32[0];
  chunkyDestination[3] |= _spriteDecode16[3][planardata].i32[1];
}

void SpriteP2CDecoder::InitializeP2CTables()
{
  for (unsigned int spriteGroup = 0; spriteGroup < 4; ++spriteGroup)
  {
    for (unsigned int plane = 0; plane < 2; ++plane)
    {
      for (unsigned int data = 0; data < 256; ++data)
      {
        for (unsigned int pixel = 0; pixel < 8; ++pixel)
        {
          _spriteDecode4[spriteGroup][plane][data].i8[pixel] = (UBY)(((data & (0x80 >> pixel)) == 0) ? 0 : (((spriteGroup + 4) << 2) | (1 << plane)));
        }
      }
    }
  }

  for (unsigned int plane = 0; plane < 4; ++plane)
  {
    for (unsigned int data = 0; data < 256; ++data)
    {
      for (unsigned int pixel = 0; pixel < 8; ++pixel)
      {
        _spriteDecode16[plane][data].i8[pixel] = ((data & (0x80 >> pixel)) == 0) ? 0 : (16 | (1 << plane));
      }
    }
  }
}

void SpriteP2CDecoder::Initialize()
{
  InitializeP2CTables();
}
