//#include "test/catch/catch.hpp"
//#include "fellow/chipset/BitplaneRegisters.h"
//#include "fellow/chipset/ChipsetInfo.h"
//#include "fellow/scheduler/Scheduler.h"
//#include "FMEM.H"
//#undef new
//
//namespace test::fellow::chipset
//{
//  class HostColorMapTests
//  {
//  public:
//    BitplaneRegisters _bitplaneregisters;
//
//    void Startup()
//    {
//      scheduler.Startup();
//      chipsetStartup();
//
//      bitplane_registers.SetAddDataCallback(nullptr);
//      bitplane_registers.SetFlushCallback(nullptr);
//    }
//
//    HostColorMapTests()
//    {
//      Startup();
//    }
//  };
//
//  void HostColorMapTestStartup()
//  {
//    scheduler.Startup();
//    chipsetStartup();
//  }
//
//  TEST_CASE_METHOD(HostColorMapTests, "Should map colors and halfbrite colors to 32 bit host colors", "[bitplaneregisters]")
//  {
//    bitplane_registers.InitializeHostColorMap(0, 8, 8, 8, 16, 8, 32);
//
//    for (unsigned int index = 0; index < 32; index++)
//    {
//      wcolor(0xfcf1, 0x180 + index * 2);
//      REQUIRE(bitplane_registers.HostColor[index] == 0x0010f0c00010f0c0UL);
//      REQUIRE(bitplane_registers.HostColor[index + 32] == 0x0000706000007060UL);
//    }
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Should map colors and halfbrite colors to 16 bit host colors", "[bitplaneregisters]")
//  {
//    bitplane_registers.InitializeHostColorMap(0, 5, 5, 5, 10, 5, 16);
//
//    for (unsigned int index = 0; index < 32; index++)
//    {
//      wcolor(0xfcf1, 0x180 + index * 2);
//      REQUIRE(bitplane_registers.HostColor[index] == 0x0bd80bd80bd80bd8UL);
//      REQUIRE(bitplane_registers.HostColor[index + 32] == 0x01cc01cc01cc01ccUL);
//    }
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for vposr", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_read[0x004 >> 1] == rvposr);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for vhposr", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_read[0x006 >> 1] == rvhposr);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for vpos", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x02a >> 1] == wvpos);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for deniseid on ECS", "[bitplaneregisters]")
//  {
//    chipsetSetECS(true);
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_read[0x07c >> 1] == rdeniseid);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Does not install memory hook for deniseid on OCS", "[bitplaneregisters]")
//  {
//    chipsetSetECS(false);
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_read[0x07c >> 1] == rdeniseid);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for diwhigh on ECS", "[bitplaneregisters]")
//  {
//    chipsetSetECS(true);
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x1e4 >> 1] == wdiwhigh);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Does not install memory hook for diwhigh on OCS", "[bitplaneregisters]")
//  {
//    chipsetSetECS(false);
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x1e4 >> 1] == wdiwhigh);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for diwstrt", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x8e >> 1] == wdiwstrt);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for diwstop", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x90 >> 1] == wdiwstop);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for ddfstrt", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x92 >> 1] == wddfstrt);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for ddfstop", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x94 >> 1] == wddfstop);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl1pth", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xe0 >> 1] == wbpl1pth);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl1ptl", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xe2 >> 1] == wbpl1ptl);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl2pth", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xe4 >> 1] == wbpl2pth);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl2ptl", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xe6 >> 1] == wbpl2ptl);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl3pth", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xe8 >> 1] == wbpl3pth);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl3ptl", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xea >> 1] == wbpl3ptl);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl4pth", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xec >> 1] == wbpl4pth);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl4ptl", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xee >> 1] == wbpl4ptl);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl5pth", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xf0 >> 1] == wbpl5pth);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl5ptl", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xf2 >> 1] == wbpl5ptl);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl6pth", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xf4 >> 1] == wbpl6pth);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl6ptl", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0xf6 >> 1] == wbpl6ptl);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bplcon0", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x100 >> 1] == wbplcon0);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bplcon1", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x102 >> 1] == wbplcon1);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bplcon2", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x104 >> 1] == wbplcon2);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl1mod", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x108 >> 1] == wbpl1mod);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl2mod", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x10a >> 1] == wbpl2mod);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl1dat", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x110 >> 1] == wbpl1dat);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl2dat", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x112 >> 1] == wbpl2dat);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl3dat", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x114 >> 1] == wbpl3dat);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl4dat", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x116 >> 1] == wbpl4dat);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl5dat", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x118 >> 1] == wbpl5dat);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hook for bpl6dat", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//    REQUIRE(memory_iobank_write[0x11a >> 1] == wbpl6dat);
//  }
//
//  TEST_CASE_METHOD(BitplaneRegistersTests, "Installs memory hooks for wcolor", "[bitplaneregisters]")
//  {
//    bitplane_registers.InstallIOHandlers();
//
//    for (unsigned int colorRegister = 0; colorRegister < 32; colorRegister++)
//    {
//      REQUIRE(memory_iobank_write[(0x180 + colorRegister) >> 1] == wcolor);
//    }
//  }
//}
