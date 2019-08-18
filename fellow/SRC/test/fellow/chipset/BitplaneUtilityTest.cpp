#include "test/catch/catch.hpp"
#include "BitplaneUtility.h"

#undef new

TEST_CASE("BitplaneUtility functions are working", "[bitplaneutility]")
{
  SECTION("GetEnabledBitplaneCount() returns the number of enabled bitplanes")
  {
    SECTION("Zero enabled bitplanes returns 0")
    {
      bplcon0 = 0x8fff;
      REQUIRE(BitplaneUtility::GetEnabledBitplaneCount() == 0);
    }
    SECTION("One enabled bitplane returns 1")
    {
      bplcon0 = 0x9fff;
      REQUIRE(BitplaneUtility::GetEnabledBitplaneCount() == 1);
    }
    SECTION("Two enabled bitplanes returns 2")
    {
      bplcon0 = 0xafff;
      REQUIRE(BitplaneUtility::GetEnabledBitplaneCount() == 2);
    }
    SECTION("Three enabled bitplanes returns 3")
    {
      bplcon0 = 0xbfff;
      REQUIRE(BitplaneUtility::GetEnabledBitplaneCount() == 3);
    }
    SECTION("Four enabled bitplanes returns 4")
    {
      bplcon0 = 0xcfff;
      REQUIRE(BitplaneUtility::GetEnabledBitplaneCount() == 4);
    }
    SECTION("Five enabled bitplanes returns 5")
    {
      bplcon0 = 0xdfff;
      REQUIRE(BitplaneUtility::GetEnabledBitplaneCount() == 5);
    }
    SECTION("Six enabled bitplanes returns 6")
    {
      bplcon0 = 0xefff;
      REQUIRE(BitplaneUtility::GetEnabledBitplaneCount() == 6);
    }
  }

  SECTION("IsLores() returns lores state")
  {
    SECTION("Lores mode on returns true")
    {
      bplcon0 = 0x7fff;
      REQUIRE(BitplaneUtility::IsLores() == true);
    }
    SECTION("Lores mode off returns false")
    {
      bplcon0 = 0x8000;
      REQUIRE(BitplaneUtility::IsLores() == false);
    }
  }

  SECTION("IsHires() returns hires state")
  {
    SECTION("Hires mode on returns true")
    {
      bplcon0 = 0x8000;
      REQUIRE(BitplaneUtility::IsHires() == true);
    }
    SECTION("Hires mode off returns false")
    {
      bplcon0 = 0x7fff;
      REQUIRE(BitplaneUtility::IsHires() == false);
    }
  }

  SECTION("IsDualPlayfield() returns dual playfield state")
  {
    SECTION("Dual playfield mode on returns true")
    {
      bplcon0 = 0x0400;
      REQUIRE(BitplaneUtility::IsDualPlayfield() == true);
    }
    SECTION("Dual playfield mode off returns false")
    {
      bplcon0 = 0xfbff;
      REQUIRE(BitplaneUtility::IsDualPlayfield() == false);
    }
  }

  SECTION("IsHam() returns ham state")
  {
    SECTION("Ham mode on returns true")
    {
      bplcon0 = 0x0800;
      REQUIRE(BitplaneUtility::IsHam() == true);
    }
    SECTION("Ham mode off returns false")
    {
      bplcon0 = 0xf7ff;
      REQUIRE(BitplaneUtility::IsHam() == false);
    }
  }

  SECTION("IsPlayfield1Pri() returns playfield 1 priority state")
  {
    SECTION("Playfield 1 priority on returns true")
    {
      bplcon2 = 0xffbf;
      REQUIRE(BitplaneUtility::IsPlayfield1Pri() == true);
    }
    SECTION("Playfield 1 priority off returns false")
    {
      bplcon2 = 0x0040;
      REQUIRE(BitplaneUtility::IsPlayfield1Pri() == false);
    }
  }

  SECTION("IsDMAEnabled() returns bitplane DMA enabled state")
  {
    SECTION("Bitplane DMA on returns true")
    {
      dmaconr = 0x0300;
      REQUIRE(BitplaneUtility::IsDMAEnabled() == true);
    }
    SECTION("Bitplane DMA off returns false")
    {
      dmaconr = 0xfcff;
      REQUIRE(BitplaneUtility::IsDMAEnabled() == false);
    }
  }
}
