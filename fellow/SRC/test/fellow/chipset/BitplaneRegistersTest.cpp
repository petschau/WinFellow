#include "test/catch/catch.hpp"
#include "fellow/chipset/BitplaneRegisters.h"
#include "fellow/chipset/ChipsetInfo.h"
#include "fellow/scheduler/Scheduler.h"
#include "FMEM.H"
#undef new

namespace test::fellow::chipset::BitplaneRegistersTests
{
  class BitplaneRegistersTests
  {
  public:
    BitplaneRegisters _bitplaneregisters;

    void Startup()
    {
      scheduler.Startup();
      chipsetStartup();

      bitplane_registers.SetAddDataCallback(nullptr);
      bitplane_registers.SetFlushCallback(nullptr);
    }

    BitplaneRegistersTests()
    {
      Startup();
    }
  };

  void BitplaneRegistersTestStartup()
  {
    scheduler.Startup();
    chipsetStartup();
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "BitplaneRegisters should be set to default values after class construction", "[bitplaneregisters]")
  {
    REQUIRE(_bitplaneregisters.lof == 0x8000);
    REQUIRE(_bitplaneregisters.diwstrt == 0);
    REQUIRE(_bitplaneregisters.diwstop == 0);
    REQUIRE(_bitplaneregisters.ddfstrt == 0);
    REQUIRE(_bitplaneregisters.ddfstop == 0);

    for (int i = 0; i < 6; i++)
    {
      REQUIRE(_bitplaneregisters.bpldat[i] == 0);
      REQUIRE(_bitplaneregisters.bplpt[i] == 0);
    }

    REQUIRE(_bitplaneregisters.bplcon0 == 0);
    REQUIRE(_bitplaneregisters.bplcon1 == 0);
    REQUIRE(_bitplaneregisters.bplcon2 == 0);
    REQUIRE(_bitplaneregisters.bpl1mod == 0);
    REQUIRE(_bitplaneregisters.bpl2mod == 0);
    REQUIRE(_bitplaneregisters.diwhigh == 0x100);

    for (int i = 0; i < 64; i++)
    {
      REQUIRE(_bitplaneregisters.color[i] == 0);
    }

    REQUIRE(_bitplaneregisters.IsLores == true);
    REQUIRE(_bitplaneregisters.IsHires == false);
    REQUIRE(_bitplaneregisters.IsDualPlayfield == false);
    REQUIRE(_bitplaneregisters.IsHam == false);
    REQUIRE(_bitplaneregisters.EnabledBitplaneCount == 0);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get short frame from vposr", "[bitplaneregisters]")
  {
    bitplane_registers.lof = 0;
    UWO vposr = rvposr(0x004);
    REQUIRE((vposr & 0x8000) == 0);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get long frame from vposr", "[bitplaneregisters]")
  {
    bitplane_registers.lof = 0x8000;
    UWO vposr = rvposr(0x004);
    REQUIRE((vposr & 0x8000) == 0x8000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get PAL OCS Agnus id from vposr", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    UWO vposr = rvposr(0x004);
    REQUIRE((vposr & 0x7f00) == 0);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get PAL ECS Agnus id from vposr", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    UWO vposr = rvposr(0x004);
    REQUIRE((vposr & 0x7f00) == 0x2000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get top bit of OCS raster line number from vposr", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    scheduler.SetFrameCycle(scheduler.GetCyclesInLine() * 313);
    UWO vposr = rvposr(0x004);
    REQUIRE((vposr & 0x0007) == 1);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get top bits of ECS raster line number from vposr", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    scheduler.SetFrameCycle(scheduler.GetCyclesInLine() * 2000);
    UWO vposr = rvposr(0x004);
    REQUIRE((vposr & 0x0007) == 7);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should not get additional top bits of OCS raster line number from vposr", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    scheduler.SetFrameCycle(scheduler.GetCyclesInLine() * 2000);
    UWO vposr = rvposr(0x004);
    REQUIRE((vposr & 0x0007) == 1);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get low byte of raster line number from vhposr", "[bitplaneregisters]")
  {
    scheduler.SetFrameCycle(scheduler.GetCyclesInLine() * 231);
    UWO vhposr = rvhposr(0x006);
    REQUIRE(((vhposr & 0xff00) >> 8) == 231);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get raster line horisontal position from vhposr", "[bitplaneregisters]")
  {
    scheduler.SetFrameCycle(scheduler.GetCyclesInLine() * 231 + scheduler.GetCycleFromCycle280ns(155));
    UWO vhposr = rvhposr(0x006);
    REQUIRE((vhposr & 0x00ff) == 155);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set long frame with vpos", "[bitplaneregisters]")
  {
    wvpos(0x8000, 0x02a);
    REQUIRE(bitplane_registers.lof == 0x8000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set short frame with vpos", "[bitplaneregisters]")
  {
    wvpos(0, 0x02a);
    REQUIRE(bitplane_registers.lof == 0);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should get ECS Denise id from rdeniseid", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    UWO id = rdeniseid(0x07c);
    REQUIRE(id == 0xfc);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set diwstrt", "[bitplaneregisters]")
  {
    wdiwstrt(0xcdef, 0x8e);
    REQUIRE(bitplane_registers.diwstrt == 0xcdef);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set diwstop", "[bitplaneregisters]")
  {
    wdiwstop(0xfedc, 0x90);
    REQUIRE(bitplane_registers.diwstop == 0xfedc);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstrt", "[bitplaneregisters]")
  {
    wddfstrt(0x18, 0x92);
    REQUIRE(bitplane_registers.ddfstrt == 0x18);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstrt with lower bit masked away on ECS", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    wddfstrt(0x23, 0x92);
    REQUIRE(bitplane_registers.ddfstrt == 0x22);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstrt with 2 lower bits masked away on OCS", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    wddfstrt(0x23, 0x92);
    REQUIRE(bitplane_registers.ddfstrt == 0x20);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstrt to a very low value", "[bitplaneregisters]")
  {
    wddfstrt(4, 0x92);
    REQUIRE(bitplane_registers.ddfstrt == 4);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstrt to a very high value", "[bitplaneregisters]")
  {
    wddfstrt(0xfc, 0x92);
    REQUIRE(bitplane_registers.ddfstrt == 0xfc);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstop", "[bitplaneregisters]")
  {
    wddfstop(0xc8, 0x94);
    REQUIRE(bitplane_registers.ddfstop == 0xc8);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstop with lower bit masked away on ECS", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    wddfstop(0xd3, 0x94);
    REQUIRE(bitplane_registers.ddfstop == 0xd2);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstop with 2 lower bitt masked away on OCS", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    wddfstop(0xd3, 0x94);
    REQUIRE(bitplane_registers.ddfstop == 0xd0);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstop to a very high value", "[bitplaneregisters]")
  {
    wddfstop(0xfc, 0x94);
    REQUIRE(bitplane_registers.ddfstop == 0xfc);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set ddfstop to a very low value", "[bitplaneregisters]")
  {
    wddfstop(4, 0x94);
    REQUIRE(bitplane_registers.ddfstop == 4);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl1pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[0] = 0;
    wbpl1pth(0xffff, 0xe0);
    REQUIRE(bitplane_registers.bplpt[0] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl1ptl and mask it to an even number", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[0] = 0;
    wbpl1ptl(0xffff, 0xe2);
    REQUIRE(bitplane_registers.bplpt[0] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl2pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[1] = 0;
    wbpl2pth(0xffff, 0xe4);
    REQUIRE(bitplane_registers.bplpt[1] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl2ptl and mask it to an even number", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[1] = 0;
    wbpl2ptl(0xffff, 0xe6);
    REQUIRE(bitplane_registers.bplpt[1] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl3pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[2] = 0;
    wbpl3pth(0xffff, 0xe8);
    REQUIRE(bitplane_registers.bplpt[2] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl3ptl and mask it to an even number", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[2] = 0;
    wbpl3ptl(0xffff, 0xea);
    REQUIRE(bitplane_registers.bplpt[2] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl4pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[3] = 0;
    wbpl4pth(0xffff, 0xec);
    REQUIRE(bitplane_registers.bplpt[3] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl4ptl and mask it to an even number", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[3] = 0;
    wbpl4ptl(0xffff, 0xee);
    REQUIRE(bitplane_registers.bplpt[3] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl5pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[4] = 0;
    wbpl5pth(0xffff, 0xf0);
    REQUIRE(bitplane_registers.bplpt[4] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl5ptl and mask it to an even number", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[4] = 0;
    wbpl5ptl(0xffff, 0xf2);
    REQUIRE(bitplane_registers.bplpt[4] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl6pth and mask it to chip memory size", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[5] = 0;
    wbpl6pth(0xffff, 0xf4);
    REQUIRE(bitplane_registers.bplpt[5] == 0x00070000);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl6ptl and mask it to an even number", "[bitplaneregisters]")
  {
    bitplane_registers.bplpt[5] = 0;
    wbpl6ptl(0xffff, 0xf6);
    REQUIRE(bitplane_registers.bplpt[5] == 0x0000fffe);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl1mod and mask it to even number", "[bitplaneregisters]")
  {
    wbpl1mod(0xcdef, 0x108);
    REQUIRE(bitplane_registers.bpl1mod == 0xcdee);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl2mod and mask it to even number", "[bitplaneregisters]")
  {
    wbpl2mod(0xfedd, 0x10a);
    REQUIRE(bitplane_registers.bpl2mod == 0xfedc);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl1dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    wbpl1dat(0xf1de, 0x110);
    REQUIRE(bitplane_registers.bpldat[0] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl2dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    wbpl2dat(0xf1de, 0x112);
    REQUIRE(bitplane_registers.bpldat[1] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl3dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    wbpl3dat(0xf1de, 0x114);
    REQUIRE(bitplane_registers.bpldat[2] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl4dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    wbpl4dat(0xf1de, 0x116);
    REQUIRE(bitplane_registers.bpldat[3] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl5dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    wbpl5dat(0xf1de, 0x118);
    REQUIRE(bitplane_registers.bpldat[4] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set bpl6dat", "[bitplaneregisters]")
  {
    chipsetSetIsCycleExact(true);
    wbpl6dat(0xf1de, 0x11a);
    REQUIRE(bitplane_registers.bpldat[5] == 0xf1de);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Should set colors and halfbrite colors", "[bitplaneregisters]")
  {
    for (unsigned int index = 0; index < 32; index++)
    {
      wcolor(0xfe7e, 0x180 + index * 2);
      REQUIRE(bitplane_registers.color[index] == 0xe7e);
      REQUIRE(bitplane_registers.color[index + 32] == 0x737);
    }
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for vposr", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_read[0x004 >> 1] == rvposr);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for vhposr", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_read[0x006 >> 1] == rvhposr);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for vpos", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x02a >> 1] == wvpos);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for deniseid on ECS", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_read[0x07c >> 1] == rdeniseid);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Does not install memory hook for deniseid on OCS", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_read[0x07c >> 1] == rdeniseid);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for diwhigh on ECS", "[bitplaneregisters]")
  {
    chipsetSetECS(true);
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x1e4 >> 1] == wdiwhigh);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Does not install memory hook for diwhigh on OCS", "[bitplaneregisters]")
  {
    chipsetSetECS(false);
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x1e4 >> 1] == wdiwhigh);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for diwstrt", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x8e >> 1] == wdiwstrt);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for diwstop", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x90 >> 1] == wdiwstop);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for ddfstrt", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x92 >> 1] == wddfstrt);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for ddfstop", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x94 >> 1] == wddfstop);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl1pth", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xe0 >> 1] == wbpl1pth);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl1ptl", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xe2 >> 1] == wbpl1ptl);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl2pth", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xe4 >> 1] == wbpl2pth);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl2ptl", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xe6 >> 1] == wbpl2ptl);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl3pth", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xe8 >> 1] == wbpl3pth);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl3ptl", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xea >> 1] == wbpl3ptl);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl4pth", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xec >> 1] == wbpl4pth);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl4ptl", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xee >> 1] == wbpl4ptl);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl5pth", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xf0 >> 1] == wbpl5pth);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl5ptl", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xf2 >> 1] == wbpl5ptl);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl6pth", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xf4 >> 1] == wbpl6pth);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl6ptl", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0xf6 >> 1] == wbpl6ptl);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bplcon0", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x100 >> 1] == wbplcon0);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bplcon1", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x102 >> 1] == wbplcon1);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bplcon2", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x104 >> 1] == wbplcon2);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl1mod", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x108 >> 1] == wbpl1mod);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl2mod", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x10a >> 1] == wbpl2mod);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl1dat", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x110 >> 1] == wbpl1dat);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl2dat", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x112 >> 1] == wbpl2dat);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl3dat", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x114 >> 1] == wbpl3dat);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl4dat", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x116 >> 1] == wbpl4dat);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl5dat", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x118 >> 1] == wbpl5dat);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl6dat", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();
    REQUIRE(memory_iobank_write[0x11a >> 1] == wbpl6dat);
  }

  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hooks for wcolor", "[bitplaneregisters]")
  {
    bitplane_registers.InstallIOHandlers();

    for (unsigned int colorRegister = 0; colorRegister < 32; colorRegister++)
    {
      REQUIRE(memory_iobank_write[(0x180 + colorRegister) >> 1] == wcolor);
    }
  }
}
