#pragma once

typedef union sprite_deco_ {
  uint8_t i8[8];
  uint32_t i32[2];
} sprite_deco;

class SpriteP2CDecoder
{
private:
  static sprite_deco sprite_deco4[4][2][256];
  static sprite_deco sprite_deco16[4][256];

  static void P2CTablesInitialize();

public:
  static void Decode4(unsigned int sprite_number, uint32_t *chunky_destination, uint16_t data1, uint16_t data2);
  static void Decode16(uint32_t *chunky_destination, uint16_t data1, uint16_t data2, uint16_t data3, uint16_t data4);

  static void Initialize();
};
