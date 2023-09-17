#include <memory>
#include "hardfile/hunks/Reloc32Hunk.h"
#include "TestBootstrap.h"
#include "catch/catch_amalgamated.hpp"

using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{

  unsigned int CreateOffsetTable(uint8_t* hunkData, unsigned int entries, unsigned int index)
  {
    // Entry count
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = static_cast<uint8_t>(entries);

    // Related hunk number
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = static_cast<uint8_t>(entries + 1);

    for (unsigned int i = 0; i < entries; i++)
    {
      hunkData[index++] = 0;
      hunkData[index++] = 0;
      hunkData[index++] = 0;
      hunkData[index++] = static_cast<uint8_t>(i + 1);
    }

    return index;
  }

  unique_ptr<uint8_t[]> CreateReloc32HunkData(unsigned int offsetTableCount)
  {
    unsigned int index = 0;
    unique_ptr<uint8_t[]> hunkData(new uint8_t[128]);

    for (unsigned int i = 0; i < offsetTableCount; i++)
    {
      index = CreateOffsetTable(hunkData.get(), offsetTableCount + 2, index);
    }

    // Terminate offset list
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 0;

    return hunkData;
  }

  TEST_CASE("Hardfile::Hunks::Reloc32Hunk.GetID() returns ID for Reloc32Hunk")
  {
    InitializeTestframework();
    constexpr unsigned int _sourceHunkIndex = 1234;
    unique_ptr<Reloc32Hunk> _instance(new Reloc32Hunk(_sourceHunkIndex));

    SECTION("Returns ID for Reloc32Hunk")
    {
      uint32_t id = _instance->GetID();
      REQUIRE(id == 0x3ec);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::Reloc32Hunk.GetSourceHunkIndex() returns source hunk index")
  {
    InitializeTestframework();
    constexpr unsigned int _sourceHunkIndex = 1234;
    unique_ptr<Reloc32Hunk> _instance(new Reloc32Hunk(_sourceHunkIndex));

    SECTION("Returns source hunk index")
    {
      uint32_t id = _instance->GetSourceHunkIndex();
      REQUIRE(id == _sourceHunkIndex);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::Reloc32Hunk.Parse() should parse input")
  {
    InitializeTestframework();
    constexpr unsigned int _sourceHunkIndex = 1234;
    unique_ptr<Reloc32Hunk> _instance(new Reloc32Hunk(_sourceHunkIndex));

    SECTION("There should be no offset tables when no offset table was present in the input")
    {
      auto hunkData = CreateReloc32HunkData(0);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), 128));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetOffsetTableCount() == 0);
    }

    SECTION("Should have information about one offset table when one offset table was present in the input")
    {
      auto hunkData = CreateReloc32HunkData(1);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), 128));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetOffsetTableCount() == 1);

      REQUIRE(_instance->GetOffsetTable(0)->GetOffsetCount() == 3);
      REQUIRE(_instance->GetOffsetTable(0)->GetRelatedHunkIndex() == 4);
      REQUIRE(_instance->GetOffsetTable(0)->GetOffset(0) == 1);
      REQUIRE(_instance->GetOffsetTable(0)->GetOffset(1) == 2);
      REQUIRE(_instance->GetOffsetTable(0)->GetOffset(2) == 3);
    }

    SECTION("Should have information about two offset tables when two offset tables were present in the input")
    {
      auto hunkData = CreateReloc32HunkData(2);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), 128));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetOffsetTableCount() == 2);
      REQUIRE(_instance->GetOffsetTable(1)->GetOffsetCount() == 4);
      REQUIRE(_instance->GetOffsetTable(1)->GetRelatedHunkIndex() == 5);
      REQUIRE(_instance->GetOffsetTable(1)->GetOffset(0) == 1);
      REQUIRE(_instance->GetOffsetTable(1)->GetOffset(1) == 2);
      REQUIRE(_instance->GetOffsetTable(1)->GetOffset(2) == 3);
      REQUIRE(_instance->GetOffsetTable(1)->GetOffset(3) == 4);
    }
    ShutdownTestframework();
  }
}
