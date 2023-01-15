#include "fellow/application/Modules.h"
#include "test/catch/catch.hpp"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/scheduler/Scheduler.h"
#include "fellow/memory/Memory.h"
#include <fellow/api/ChipsetConstants.h>

#undef new

namespace test::fellow::chipset::BitplaneRegistersTests
{
  class BitplaneRegistersTests
  {
  public:
    BitplaneRegisters _bitplaneRegisters;
    Scheduler _scheduler;
    Modules _modules;
    ChipsetBankHandler _chipsetBankHandler;

    void Startup()
    {
      _scheduler.Startup(&_modules);
      chipsetStartup();
      _bitplaneRegisters.Initialize(&_scheduler);
      _bitplaneRegisters.InstallIOHandlers(_chipsetBankHandler, false);
    }

    BitplaneRegistersTests()
    {
      Startup();
    }
  };

  //void BitplaneRegistersTestStartup()
  //{
  //  scheduler.Startup();
  //  chipsetStartup();
  //}

  TEST_CASE_METHOD(BitplaneRegistersTests, "BitplaneRegisters should be set to default values after class instance initialization", "[bitplaneregisters]")
  {
    REQUIRE(_bitplaneRegisters.lof == 0x8000);
    REQUIRE(_bitplaneRegisters.diwstrt == 0);
    REQUIRE(_bitplaneRegisters.diwstop == 0);
    REQUIRE(_bitplaneRegisters.ddfstrt == 0);
    REQUIRE(_bitplaneRegisters.ddfstop == 0);

    for (int i = 0; i < 6; i++)
    {
      REQUIRE(_bitplaneRegisters.bpldat[i] == 0);
      REQUIRE(_bitplaneRegisters.bplpt[i] == 0);
    }

    REQUIRE(_bitplaneRegisters.bplcon0 == 0);
    REQUIRE(_bitplaneRegisters.bplcon1 == 0);
    REQUIRE(_bitplaneRegisters.bplcon2 == 0);
    REQUIRE(_bitplaneRegisters.bpl1mod == 0);
    REQUIRE(_bitplaneRegisters.bpl2mod == 0);
    REQUIRE(_bitplaneRegisters.diwhigh == 0x100);

    for (unsigned int i = 0; i < 64; i++)
    {
      REQUIRE(_bitplaneRegisters.color[i] == 0);
    }

    REQUIRE(_bitplaneRegisters.IsLores == true);
    REQUIRE(_bitplaneRegisters.IsHires == false);
    REQUIRE(_bitplaneRegisters.IsDualPlayfield == false);
    REQUIRE(_bitplaneRegisters.IsHam == false);
    REQUIRE(_bitplaneRegisters.EnabledBitplaneCount == 0);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set long frame in VPOSW and get long frame from VPOSR", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0x8000, RegisterIndex::VPOSW);
    uint16_t vposr = _chipsetBankHandler.ReadWord(RegisterIndex::VPOSR);
    REQUIRE((vposr & 0x8000) == 0x8000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set short frame in VPOSW and get short frame from VPOSR", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0x0000, RegisterIndex::VPOSW);
    uint16_t vposr = _chipsetBankHandler.ReadWord(RegisterIndex::VPOSR);
    REQUIRE((vposr & 0x8000) == 0x0000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get Fat Agnus PAL id from VPOSR", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    uint16_t vposr = _chipsetBankHandler.ReadWord(RegisterIndex::VPOSR);
    REQUIRE(((vposr & 0x7f00) >> 8) == AgnusId::Agnus_8371_Fat_PAL);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get ECS Agnus PAL id from VPOSR", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    uint16_t vposr = _chipsetBankHandler.ReadWord(RegisterIndex::VPOSR);
    REQUIRE(((vposr & 0x7f00) >> 8) ==AgnusId::Agnus_8372_ECS_PAL_until_Rev4);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get top bit of OCS raster line number from VPOSR", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    _scheduler.SetFrameCycle(_scheduler.GetCyclesInLine() * 313);
    uint16_t vposr = _chipsetBankHandler.ReadWord(RegisterIndex::VPOSR);
    REQUIRE((vposr & 0x0007) == 1);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get top bits of ECS raster line number from VPOSR", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    _scheduler.SetFrameCycle(_scheduler.GetCyclesInLine() * 2000);
    uint16_t vposr = _chipsetBankHandler.ReadWord(RegisterIndex::VPOSR);
    REQUIRE((vposr & 0x0007) == 7);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should not get additional top bits of high raster line number in OCS from VPOSR", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    _scheduler.SetFrameCycle(_scheduler.GetCyclesInLine() * 2000);
    uint16_t vposr = _chipsetBankHandler.ReadWord(RegisterIndex::VPOSR);
    REQUIRE((vposr & 0x0007) == 1);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get low byte of raster line number from VHPOSR", "[bitplaneregisters]")
  {
    _scheduler.SetFrameCycle(_scheduler.GetCyclesInLine() * 231);
    uint16_t vhposr = _chipsetBankHandler.ReadWord(RegisterIndex::VHPOSR);
    REQUIRE(((vhposr & 0xff00) >> 8) == 231);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get raster line horisontal position from VHPOSR", "[bitplaneregisters]")
  {
    _scheduler.SetFrameCycle(_scheduler.GetCyclesInLine() * 231 + _scheduler.GetCycleFromCycle280ns(155));
    uint16_t vhposr = _chipsetBankHandler.ReadWord(RegisterIndex::VHPOSR);
    REQUIRE((vhposr & 0x00ff) == 155);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get random values for DENISEID on OCS", "[bitplaneregisters]")
  {
    chipsetSetECS(false);

    // Read many values from register and expect different values
    bool valueChanged = false;
    uint16_t initialValue = _chipsetBankHandler.ReadWord(RegisterIndex::DENISEID);
    for (auto i = 0; i < 64 && !valueChanged; i++)
    {
      valueChanged = initialValue != _chipsetBankHandler.ReadWord(RegisterIndex::DENISEID);
    }

    REQUIRE(valueChanged == true);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get Denise ECS id from DENISEID", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    uint16_t deniseid = _chipsetBankHandler.ReadWord(RegisterIndex::DENISEID);
    REQUIRE((deniseid & 0xff) == DeniseId::Denise_8373_ECS);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set diwstrt through DIWSTRT", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0xcdef, RegisterIndex::DIWSTRT);
    REQUIRE(_bitplaneRegisters.diwstrt == 0xcdef);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set diwstop through DIWSTOP", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0xfedc, RegisterIndex::DIWSTOP);
    REQUIRE(_bitplaneRegisters.diwstop == 0xfedc);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstrt through DDFSTRT", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0x18, RegisterIndex::DDFSTRT);
    REQUIRE(_bitplaneRegisters.ddfstrt == 0x18);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstrt with 2 lower bits masked away on OCS", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    _chipsetBankHandler.WriteWord(0x23, RegisterIndex::DDFSTRT);
    REQUIRE(_bitplaneRegisters.ddfstrt == 0x20);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstrt with lower bit masked away on ECS", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    _chipsetBankHandler.WriteWord(0x23, RegisterIndex::DDFSTRT);
    REQUIRE(_bitplaneRegisters.ddfstrt == 0x22);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstrt to a very low value", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0x04, RegisterIndex::DDFSTRT);
    REQUIRE(_bitplaneRegisters.ddfstrt == 0x04);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstrt to a very high value", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0xfc, RegisterIndex::DDFSTRT);
    REQUIRE(_bitplaneRegisters.ddfstrt == 0xfc);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstop through DDFSTOP", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0xc8, RegisterIndex::DDFSTOP);
    REQUIRE(_bitplaneRegisters.ddfstop == 0xc8);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstop with 2 lower bits masked away on OCS", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    _chipsetBankHandler.WriteWord(0xd3, RegisterIndex::DDFSTOP);
    REQUIRE(_bitplaneRegisters.ddfstop == 0xd0);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstop with lower bit masked away on ECS", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    _chipsetBankHandler.WriteWord(0xd3, RegisterIndex::DDFSTOP);
    REQUIRE(_bitplaneRegisters.ddfstop == 0xd2);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstop to a very high value", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0xfc, RegisterIndex::DDFSTOP);
    REQUIRE(_bitplaneRegisters.ddfstop == 0xfc);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstop to a very low value", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0x04, RegisterIndex::DDFSTOP);
    REQUIRE(_bitplaneRegisters.ddfstop == 4);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl1pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[0] = 0;
    _chipsetBankHandler.WriteWord(0xffff, RegisterIndex::BPL1PTH);
    REQUIRE(_bitplaneRegisters.bplpt[0] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl1ptl and mask it to an even number", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[0] = 0;
    _chipsetBankHandler.WriteWord(0xffff, RegisterIndex::BPL1PTL);
    REQUIRE(_bitplaneRegisters.bplpt[0] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl2pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[1] = 0;
    _chipsetBankHandler.WriteWord(0xffff, RegisterIndex::BPL2PTH);
    REQUIRE(_bitplaneRegisters.bplpt[1] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl2ptl and mask it to an even number", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[1] = 0;
    _chipsetBankHandler.WriteWord(0xffff, RegisterIndex::BPL2PTL);
    REQUIRE(_bitplaneRegisters.bplpt[1] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl3pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[2] = 0;
    _chipsetBankHandler.WriteWord(0xe4, RegisterIndex::BPL3PTH);
    REQUIRE(_bitplaneRegisters.bplpt[2] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl3ptl and mask it to an even number", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[2] = 0;
    _chipsetBankHandler.WriteWord(0xe4, RegisterIndex::BPL3PTL);
    REQUIRE(_bitplaneRegisters.bplpt[2] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl4pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[3] = 0;
    _chipsetBankHandler.WriteWord(0xe4, RegisterIndex::BPL4PTH);
    REQUIRE(_bitplaneRegisters.bplpt[3] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl4ptl and mask it to an even number", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[3] = 0;
    _chipsetBankHandler.WriteWord(0xe4, RegisterIndex::BPL4PTL);
    REQUIRE(_bitplaneRegisters.bplpt[3] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl5pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[4] = 0;
    _chipsetBankHandler.WriteWord(0xe4, RegisterIndex::BPL5PTH);
    REQUIRE(_bitplaneRegisters.bplpt[4] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl5ptl and mask it to an even number", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[4] = 0;
    _chipsetBankHandler.WriteWord(0xe4, RegisterIndex::BPL5PTL);
    REQUIRE(_bitplaneRegisters.bplpt[4] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl6pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[5] = 0;
    _chipsetBankHandler.WriteWord(0xe4, RegisterIndex::BPL6PTH);
    REQUIRE(_bitplaneRegisters.bplpt[5] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl6ptl and mask it to an even number", "[bitplaneregisters]")
  {
    _bitplaneRegisters.bplpt[5] = 0;
    _chipsetBankHandler.WriteWord(0xe4, RegisterIndex::BPL6PTL);
    REQUIRE(_bitplaneRegisters.bplpt[5] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl1mod and mask it to even number", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0xcdef, RegisterIndex::BPL1MOD);
    REQUIRE(_bitplaneRegisters.bpl1mod == 0xcdee);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl2mod and mask it to even number", "[bitplaneregisters]")
  {
    _chipsetBankHandler.WriteWord(0xfedd, RegisterIndex::BPL2MOD);
    REQUIRE(_bitplaneRegisters.bpl2mod == 0xfedc);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl1dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    _chipsetBankHandler.WriteWord(0xf1de, RegisterIndex::BPL1DAT);
    REQUIRE(_bitplaneRegisters.bpldat[0] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl2dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    _chipsetBankHandler.WriteWord(0xf1de, RegisterIndex::BPL2DAT);
    REQUIRE(_bitplaneRegisters.bpldat[1] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl3dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    _chipsetBankHandler.WriteWord(0xf1de, RegisterIndex::BPL3DAT);
    REQUIRE(_bitplaneRegisters.bpldat[2] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl4dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    _chipsetBankHandler.WriteWord(0xf1de, RegisterIndex::BPL4DAT);
    REQUIRE(_bitplaneRegisters.bpldat[3] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl5dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    _chipsetBankHandler.WriteWord(0xf1de, RegisterIndex::BPL5DAT);
    REQUIRE(_bitplaneRegisters.bpldat[4] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl6dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    _chipsetBankHandler.WriteWord(0xf1de, RegisterIndex::BPL6DAT);
    REQUIRE(_bitplaneRegisters.bpldat[5] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set colors and halfbrite colors", "[bitplaneregisters]")
  {
    for (unsigned int index = 0; index < 32; index++)
    {
      _chipsetBankHandler.WriteWord(0xfe7e, RegisterIndex::COLOR00 + index * 2);
      REQUIRE(_bitplaneRegisters.color[index] == 0xe7e);
      REQUIRE(_bitplaneRegisters.color[index + 32] == 0x737);
    }
  }
}
