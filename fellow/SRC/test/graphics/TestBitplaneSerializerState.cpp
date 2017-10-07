#include "catch.hpp"
#include "BitplaneUtility.h"
#include "BitplaneSerializerState.h"

TEST_CASE("bitplane serializer state", "[BitplaneSerializerState]")
{
  BitplaneSerializerState *state = new BitplaneSerializerState();

  SECTION("bitplane serializer state is initialized")
  {
    SECTION("is not activated")
    {
      REQUIRE(state->Activated == false);
    }
    SECTION("current line number is 0")
    {
      REQUIRE(state->CurrentLineNumber == 0);
    }
    SECTION("current cylinder points to the start of the line")
    {
      REQUIRE(state->CurrentCylinder == 56);
    }
    SECTION("minimum cylinder is correct")
    {
      REQUIRE(state->MinimumCylinder == 56);
    }
    SECTION("maximum cylinder is correct")
    {
      REQUIRE(state->MaximumCylinder == 25);
    }
    SECTION("bitplane data is cleared")
    {
      REQUIRE(state->Pending[0] == 0);
      REQUIRE(state->Active[0].w == 0);

      REQUIRE(state->Pending[1] == 0);
      REQUIRE(state->Active[1].w == 0);

      REQUIRE(state->Pending[2] == 0);
      REQUIRE(state->Active[2].w == 0);

      REQUIRE(state->Pending[3] == 0);
      REQUIRE(state->Active[3].w == 0);

      REQUIRE(state->Pending[4] == 0);
      REQUIRE(state->Active[4].w == 0);

      REQUIRE(state->Pending[4] == 0);
      REQUIRE(state->Active[5].w == 0);
    }
  }

  SECTION("can commit bitplane data")
  {
    state->Commit(0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020);

    REQUIRE(state->Pending[0] == 0x0101);
    REQUIRE(state->Pending[1] == 0x0202);
    REQUIRE(state->Pending[2] == 0x0404);
    REQUIRE(state->Pending[3] == 0x0808);
    REQUIRE(state->Pending[4] == 0x1010);
    REQUIRE(state->Pending[5] == 0x2020);
  }

  SECTION("can clear active and pending bitplane data")
  {
    for (unsigned int i = 0; i < 6; i++)
    {
      state->Pending[i] = 0xffff;
      state->Active[i].w = 0xeeee;
    }
    state->ClearActiveAndPendingWords();

    REQUIRE(state->Pending[0] == 0);
    REQUIRE(state->Active[0].w == 0);

    REQUIRE(state->Pending[1] == 0);
    REQUIRE(state->Active[1].w == 0);

    REQUIRE(state->Pending[2] == 0);
    REQUIRE(state->Active[2].w == 0);

    REQUIRE(state->Pending[3] == 0);
    REQUIRE(state->Active[3].w == 0);

    REQUIRE(state->Pending[4] == 0);
    REQUIRE(state->Active[4].w == 0);

    REQUIRE(state->Pending[4] == 0);
    REQUIRE(state->Active[5].w == 0);
  }

  SECTION("can copy pending to active playfield 0 data")
  {
    for (unsigned int i = 0; i < 6; i++)
    {
      state->Pending[i] = (i & 1) ? 0xcccc : 0xeeee;
      state->Active[i].w = 0;
    }
    state->CopyPendingToActive(0);

    REQUIRE(state->Pending[0] == 0);
    REQUIRE(state->Active[0].w == 0xeeee);

    REQUIRE(state->Pending[2] == 0);
    REQUIRE(state->Active[2].w == 0xeeee);

    REQUIRE(state->Pending[4] == 0);
    REQUIRE(state->Active[4].w == 0xeeee);

    REQUIRE(state->Pending[1] == 0xcccc);
    REQUIRE(state->Active[1].w == 0);

    REQUIRE(state->Pending[3] == 0xcccc);
    REQUIRE(state->Active[3].w == 0);

    REQUIRE(state->Pending[5] == 0xcccc);
    REQUIRE(state->Active[5].w == 0);
  }

  SECTION("can copy pending to active playfield 1 data")
  {
    for (unsigned int i = 0; i < 6; i++)
    {
      state->Pending[i] = (i & 1) ? 0xcccc : 0xeeee;
      state->Active[i].w = 0;
    }
    state->CopyPendingToActive(1);

    REQUIRE(state->Pending[0] == 0xeeee);
    REQUIRE(state->Active[0].w == 0);

    REQUIRE(state->Pending[2] == 0xeeee);
    REQUIRE(state->Active[2].w == 0);

    REQUIRE(state->Pending[4] == 0xeeee);
    REQUIRE(state->Active[4].w == 0);

    REQUIRE(state->Pending[1] == 0);
    REQUIRE(state->Active[1].w == 0xcccc);

    REQUIRE(state->Pending[3] == 0);
    REQUIRE(state->Active[3].w == 0xcccc);

    REQUIRE(state->Pending[5] == 0);
    REQUIRE(state->Active[5].w == 0xcccc);
  }

  SECTION("can shift active bitplane data 0 positions")
  {
    for (unsigned int i = 0; i < 6; i++)
    {
      state->Active[i].w = 0xaaaa;
    }
    state->ShiftActive(0);

    REQUIRE(state->Active[0].w == 0xaaaa);
    REQUIRE(state->Active[1].w == 0xaaaa);
    REQUIRE(state->Active[2].w == 0xaaaa);
    REQUIRE(state->Active[3].w == 0xaaaa);
    REQUIRE(state->Active[4].w == 0xaaaa);
    REQUIRE(state->Active[5].w == 0xaaaa);
  }

  SECTION("can shift active bitplane data 1 position")
  {
    for (unsigned int i = 0; i < 6; i++)
    {
      state->Active[i].w = 0xaaaa;
    }
    state->ShiftActive(1);

    REQUIRE(state->Active[0].w == 0x5554);
    REQUIRE(state->Active[1].w == 0x5554);
    REQUIRE(state->Active[2].w == 0x5554);
    REQUIRE(state->Active[3].w == 0x5554);
    REQUIRE(state->Active[4].w == 0x5554);
    REQUIRE(state->Active[5].w == 0x5554);
  }

  SECTION("can shift active bitplane data 15 positions")
  {
    for (unsigned int i = 0; i < 6; i++)
    {
      state->Active[i].w = 0x5555;
    }
    state->ShiftActive(15);

    REQUIRE(state->Active[0].w == 0x8000);
    REQUIRE(state->Active[1].w == 0x8000);
    REQUIRE(state->Active[2].w == 0x8000);
    REQUIRE(state->Active[3].w == 0x8000);
    REQUIRE(state->Active[4].w == 0x8000);
    REQUIRE(state->Active[5].w == 0x8000);
  }
}
