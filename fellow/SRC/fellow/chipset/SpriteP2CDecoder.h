#pragma once

union sprite_deco {
  UBY i8[8];
  ULO i32[2];
};

class SpriteP2CDecoder
{
private:
  static sprite_deco _spriteDecode4[4][2][256];
  static sprite_deco _spriteDecode16[4][256];

  static void InitializeP2CTables();

public:
  static void Decode4(unsigned int spriteNumber, ULO *chunkyDestination, UWO data1, UWO data2);
  static void Decode16(ULO *chunkyDestination, UWO data1, UWO data2, UWO data3, UWO data4);

  static void Initialize();
};
