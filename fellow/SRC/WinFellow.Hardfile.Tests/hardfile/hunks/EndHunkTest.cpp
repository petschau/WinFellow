#include <memory>
#include "hardfile/hunks/EndHunk.h"
#include "TestBootstrap.h"
#include "catch/catch_amalgamated.hpp"

using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  unique_ptr<uint8_t[]> CreateEndHunkData()
  {
    return unique_ptr<uint8_t[]>(new uint8_t[0]);
  }

  TEST_CASE("Hardfile::Hunks::EndHunk.GetID() returns ID for EndHunk")
  {
    InitializeTestframework();
    unique_ptr<EndHunk> _instance(new EndHunk());

    SECTION("Returns ID for EndHunk")
    {
      uint32_t id = _instance->GetID();
      REQUIRE(id == 0x3f2);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::EndHunk.Parse() can be called and reads nothing")
  {
    InitializeTestframework();
    unique_ptr<EndHunk> _instance(new EndHunk());

    SECTION("Parse() reads nothing from input")
    {
      auto hunkData = CreateEndHunkData();
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), 0));

      REQUIRE_NOTHROW(_instance->Parse(*rawDataReader)); // RawDataReader will throw exception if used
    }

    ShutdownTestframework();
  }
}
