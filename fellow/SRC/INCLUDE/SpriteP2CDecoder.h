#ifndef SPRITEP2CDECODER_H
#define SPRITEP2CDECODER_H

typedef union sprite_deco_
{
  UBY i8[8];
  ULO i32[2];
} sprite_deco;

class SpriteP2CDecoder
{
private:
  static sprite_deco sprite_deco4[4][2][256];
  static sprite_deco sprite_deco16[4][256];

  static void P2CTablesInitialize();

public:
  static void Decode4(unsigned int sprite_number, ULO *chunky_destination, UWO data1, UWO data2);
  static void Decode16(ULO *chunky_destination, UWO data1, UWO data2, UWO data3, UWO data4);

  static void Initialize();
};

#endif
