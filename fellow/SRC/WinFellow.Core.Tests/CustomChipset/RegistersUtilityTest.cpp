#include "catch/catch_amalgamated.hpp"
#include "VirtualHost/Core.h"

TEST_CASE("CustomChipset::RegisterUtility.GetEnabledBitplaneCount() should return the number of enabled bitplanes")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Zero enabled bitplanes returns 0")
  {
    _core.Registers.BplCon0 = 0x8fff;
    REQUIRE(_registerUtility.GetEnabledBitplaneCount() == 0);
  }

  SECTION("One enabled bitplane returns 1")
  {
    _core.Registers.BplCon0 = 0x9fff;
    REQUIRE(_registerUtility.GetEnabledBitplaneCount() == 1);
  }

  SECTION("Two enabled bitplanes returns 2")
  {
    _core.Registers.BplCon0 = 0xafff;
    REQUIRE(_registerUtility.GetEnabledBitplaneCount() == 2);
  }

  SECTION("Three enabled bitplanes returns 3")
  {
    _core.Registers.BplCon0 = 0xbfff;
    REQUIRE(_registerUtility.GetEnabledBitplaneCount() == 3);
  }

  SECTION("Four enabled bitplanes returns 4")
  {
    _core.Registers.BplCon0 = 0xcfff;
    REQUIRE(_registerUtility.GetEnabledBitplaneCount() == 4);
  }

  SECTION("Five enabled bitplanes returns 5")
  {
    _core.Registers.BplCon0 = 0xdfff;
    REQUIRE(_registerUtility.GetEnabledBitplaneCount() == 5);
  }

  SECTION("Six enabled bitplanes returns 6")
  {
    _core.Registers.BplCon0 = 0xefff;
    REQUIRE(_registerUtility.GetEnabledBitplaneCount() == 6);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsLoresEnabled() should return the lores enabled state")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Lores mode on returns true")
  {
    _core.Registers.BplCon0 = 0x7fff;
    REQUIRE(_registerUtility.IsLoresEnabled() == true);
  }
  SECTION("Lores mode off returns false")
  {
    _core.Registers.BplCon0 = 0x8000;
    REQUIRE(_registerUtility.IsLoresEnabled() == false);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsHiresEnabled() should return the hires enabled state")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Hires mode on returns true")
  {
    _core.Registers.BplCon0 = 0x8000;
    REQUIRE(_registerUtility.IsHiresEnabled() == true);
  }

  SECTION("Hires mode off returns false")
  {
    _core.Registers.BplCon0 = 0x7fff;
    REQUIRE(_registerUtility.IsHiresEnabled() == false);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsDualPlayfieldEnabled() should return the dual playfield enabled state")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Dual playfield mode on returns true")
  {
    _core.Registers.BplCon0 = 0x0400;
    REQUIRE(_registerUtility.IsDualPlayfieldEnabled() == true);
  }

  SECTION("Dual playfield mode off returns false")
  {
    _core.Registers.BplCon0 = 0xfbff;
    REQUIRE(_registerUtility.IsDualPlayfieldEnabled() == false);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsHAMEnabled() should return the HAM enabled state")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("HAM mode on returns true")
  {
    _core.Registers.BplCon0 = 0x0800;
    REQUIRE(_registerUtility.IsHAMEnabled() == true);
  }

  SECTION("HAM mode off returns false")
  {
    _core.Registers.BplCon0 = 0xf7ff;
    REQUIRE(_registerUtility.IsHAMEnabled() == false);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsInterlaceEnabled() should return the interlace enabled state")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Interlace mode on returns true")
  {
    _core.Registers.BplCon0 = 0x0004;
    REQUIRE(_registerUtility.IsInterlaceEnabled() == true);
  }

  SECTION("Interlace mode off returns false")
  {
    _core.Registers.BplCon0 = 0xfffb;
    REQUIRE(_registerUtility.IsInterlaceEnabled() == false);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsPlayfield1PriorityEnabled() should return the priority state of playfield 1")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Playfield 1 priority on returns true")
  {
    _core.Registers.BplCon2 = 0xffbf;
    REQUIRE(_registerUtility.IsPlayfield1PriorityEnabled() == true);
  }

  SECTION("Playfield 1 priority off returns false")
  {
    _core.Registers.BplCon2 = 0x0040;
    REQUIRE(_registerUtility.IsPlayfield1PriorityEnabled() == false);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsPlayfield2PriorityEnabled() should return the priority state of playfield 2")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Playfield 2 priority on returns true")
  {
    _core.Registers.BplCon2 = 0x0040;
    REQUIRE(_registerUtility.IsPlayfield2PriorityEnabled() == true);
  }

  SECTION("Playfield 2 priority off returns false")
  {
    _core.Registers.BplCon2 = 0xffbf;
    REQUIRE(_registerUtility.IsPlayfield2PriorityEnabled() == false);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsMasterDMAEnabled() should return the enabled state of master DMA")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Master DMA on returns true")
  {
    _core.Registers.DmaConR = 0x0200;
    REQUIRE(_registerUtility.IsMasterDMAEnabled() == true);
  }

  SECTION("Master DMA off returns false")
  {
    _core.Registers.DmaConR = 0xfdff;
    REQUIRE(_registerUtility.IsMasterDMAEnabled() == false);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsDiskDMAEnabled() should return the enabled state of disk DMA")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Disk DMA on returns true")
  {
    _core.Registers.DmaConR = 0x0010;
    REQUIRE(_registerUtility.IsDiskDMAEnabled() == true);
  }

  SECTION("Disk DMA off returns false")
  {
    _core.Registers.DmaConR = 0xffef;
    REQUIRE(_registerUtility.IsDiskDMAEnabled() == false);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsMasterDMAAndBitplaneDMAEnabled() should return the combined enabled state of master DMA and bitplane DMA")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Master DMA on and Bitplane DMA on returns true")
  {
    _core.Registers.DmaConR = 0x0300;
    REQUIRE(_registerUtility.IsMasterDMAAndBitplaneDMAEnabled() == true);
  }

  SECTION("Master DMA on and Bitplane DMA off returns false")
  {
    _core.Registers.DmaConR = 0x0200;
    REQUIRE(_registerUtility.IsMasterDMAAndBitplaneDMAEnabled() == false);
  }

  SECTION("Master DMA off and Bitplane DMA on returns false")
  {
    _core.Registers.DmaConR = 0xfeff;
    REQUIRE(_registerUtility.IsMasterDMAAndBitplaneDMAEnabled() == false);
  }

  SECTION("Master DMA off and Bitplane DMA off returns false")
  {
    _core.Registers.DmaConR = 0xfcff;
    REQUIRE(_registerUtility.IsMasterDMAAndBitplaneDMAEnabled() == false);
  }
}

TEST_CASE("CustomChipset::RegisterUtility.IsBlitterPriorityEnabled() should return the priority state of the blitter")
{
  Core _core = Core();
  const RegisterUtility &_registerUtility = _core.RegisterUtility;

  SECTION("Blitter priority on returns true")
  {
    _core.Registers.DmaConR = 0x0400;
    REQUIRE(_registerUtility.IsBlitterPriorityEnabled() == true);
  }

  SECTION("Blitter priority off returns false")
  {
    _core.Registers.DmaConR = 0xfbff;
    REQUIRE(_registerUtility.IsBlitterPriorityEnabled() == false);
  }
}
