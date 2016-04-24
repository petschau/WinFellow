#include "catch.hpp"
#include "BitplaneUtility.h"
#include "PixelSerializer.h"

TEST_CASE("pixel serializer", "[PixelSerializer]")
{
  PixelSerializer *serializer = new PixelSerializer();

  SECTION("pixel serializer is initialized")
  {
    REQUIRE(serializer->GetActivated() == false);
    REQUIRE(serializer->GetIsWaitingForActivationOnLine() == true);
    REQUIRE(serializer->GetLastCylinderOutput() == 55);
    REQUIRE(serializer->GetFirstPossibleOutputCylinder() == 56);
    REQUIRE(serializer->GetLastPossibleOutputCylinder() == 25);
  }

  SECTION("can commit bitplane data")
  {
    serializer->Commit(0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020);
    
    SECTION("bitplane 1 data is committed")
    {
      REQUIRE(serializer->GetPendingWord(0) == 0x0101);
    }
    SECTION("bitplane 2 data is committed")
    {
      REQUIRE(serializer->GetPendingWord(1) == 0x0202);
    }
    SECTION("bitplane 3 data is committed")
    {
      REQUIRE(serializer->GetPendingWord(2) == 0x0404);
    }
    SECTION("bitplane 4 data is committed")
    {
      REQUIRE(serializer->GetPendingWord(3) == 0x0808);
    }
    SECTION("bitplane 5 data is committed")
    {
      REQUIRE(serializer->GetPendingWord(4) == 0x1010);
    }
    SECTION("bitplane 6 data is committed")
    {
      REQUIRE(serializer->GetPendingWord(5) == 0x2020);
    }
  }

}