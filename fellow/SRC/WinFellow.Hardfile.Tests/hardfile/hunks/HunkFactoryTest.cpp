#include "catch/catch_amalgamated.hpp"
#include <memory>
#include "hardfile/hunks/HunkFactory.h"
#include "hardfile/hunks/HunkID.h"
#include "hardfile/hunks/CodeHunk.h"
#include "hardfile/hunks/DataHunk.h"
#include "hardfile/hunks/BSSHunk.h"
#include "hardfile/hunks/Reloc32Hunk.h"
#include "hardfile/hunks/EndHunk.h"
#include "TestBootstrap.h"

using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  TEST_CASE("Hardfile::Hunks::HunkFactory.CreateInitialHunk creates Hunk of requested kind")
  {
    InitializeTestframework();

    SECTION("Creates CodeHunk")
    {
      unique_ptr<CodeHunk> hunk(dynamic_cast<CodeHunk*>(HunkFactory::CreateInitialHunk(CodeHunkID, 100)));

      REQUIRE(hunk->GetID() == CodeHunkID);
      REQUIRE(hunk->GetAllocateSizeInLongwords() == 100);
    }

    SECTION("Creates DataHunk")
    {
      unique_ptr<DataHunk> hunk(dynamic_cast<DataHunk*>(HunkFactory::CreateInitialHunk(DataHunkID, 100)));

      REQUIRE(hunk->GetID() == DataHunkID);
      REQUIRE(hunk->GetAllocateSizeInLongwords() == 100);
    }

    SECTION("Creates BSSHunk")
    {
      unique_ptr<BSSHunk> hunk(dynamic_cast<BSSHunk*>(HunkFactory::CreateInitialHunk(BSSHunkID, 100)));

      REQUIRE(hunk->GetID() == BSSHunkID);
      REQUIRE(hunk->GetAllocateSizeInLongwords() == 100);
    }

    SECTION("Does not create unknown hunk")
    {
      unique_ptr<InitialHunk> hunk(HunkFactory::CreateInitialHunk(12345678, 100));

      REQUIRE(hunk.get() == nullptr);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::HunkFactory.CreateAdditionalHunk creates Hunk of requested kind")
  {
    InitializeTestframework();

    SECTION("Creates Reloc32Hunk")
    {
      unique_ptr<Reloc32Hunk> hunk(dynamic_cast<Reloc32Hunk*>(HunkFactory::CreateAdditionalHunk(Reloc32HunkID, 0)));

      REQUIRE(hunk->GetID() == Reloc32HunkID);
    }

    SECTION("Creates EndHunk")
    {
      unique_ptr<EndHunk> hunk(dynamic_cast<EndHunk*>(HunkFactory::CreateAdditionalHunk(EndHunkID, 0)));

      REQUIRE(hunk->GetID() == EndHunkID);
    }

    SECTION("Does not create unknown hunk")
    {
      unique_ptr<AdditionalHunk> hunk(HunkFactory::CreateAdditionalHunk(52345678, 0));

      REQUIRE(hunk.get() == nullptr);
    }

    ShutdownTestframework();
  }
}
