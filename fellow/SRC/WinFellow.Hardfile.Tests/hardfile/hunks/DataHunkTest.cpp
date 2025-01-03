#include <memory>
#include "hardfile/hunks/DataHunk.h"
#include "TestBootstrap.h"
#include "catch/catch_amalgamated.hpp"

using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  unique_ptr<uint8_t[]> CreateDataHunkData()
  {
    unique_ptr<uint8_t[]> hunkData(new uint8_t[12]);
    hunkData[0] = 0;
    hunkData[1] = 0;
    hunkData[2] = 0;
    hunkData[3] = 2;
    hunkData[4] = 0;
    hunkData[5] = 0;
    hunkData[6] = 0;
    hunkData[7] = 1;
    hunkData[8] = 0;
    hunkData[9] = 0;
    hunkData[10] = 0;
    hunkData[11] = 2;

    return hunkData;
  }

  TEST_CASE("Hardfile::Hunks::DataHunk.GetAllocateSizeInLongwords() should return the initialized allocated size")
  {
    InitializeTestframework();
    constexpr uint32_t AllocateSizeInLongwords = 4567;
    unique_ptr<DataHunk> _instance(new DataHunk(AllocateSizeInLongwords));

    SECTION("Returns allocated size as longword count")
    {
      uint32_t allocateSizeInLongwords = _instance->GetAllocateSizeInLongwords();
      REQUIRE(allocateSizeInLongwords == AllocateSizeInLongwords);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::DataHunk.GetAllocateSizeInBytes() should return the initialized allocated size")
  {
    InitializeTestframework();
    constexpr uint32_t AllocateSizeInLongwords = 4567;
    unique_ptr<DataHunk> _instance(new DataHunk(AllocateSizeInLongwords));

    SECTION("'returns allocated size as byte count")
    {
      uint32_t allocateSizeInBytes = _instance->GetAllocateSizeInBytes();
      REQUIRE(allocateSizeInBytes == AllocateSizeInLongwords * 4);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::DataHunk.GetID() returns ID for DataHunk")
  {
    InitializeTestframework();
    unique_ptr<DataHunk> _instance(new DataHunk(10));

    SECTION("Returns ID for DataHunk")
    {
      uint32_t id = _instance->GetID();
      REQUIRE(id == 0x3ea);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::DataHunk.Parse() should set content size")
  {
    InitializeTestframework();
    unique_ptr<DataHunk> _instance(new DataHunk(10));

    SECTION("Sets content size based on data read from raw hunk data")
    {
      auto hunkData = CreateDataHunkData();
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), 12));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetContentSizeInLongwords() == 2);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::DataHunk.Parse() should set hunk contents based on the input")
  {
    InitializeTestframework();
    unique_ptr<DataHunk> _instance(new DataHunk(10));

    SECTION("Sets hunk contents based on the input")
    {
      auto hunkData = CreateDataHunkData();
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), 12));

      _instance->Parse(*rawDataReader);
      uint8_t *content = _instance->GetContent();

      REQUIRE(content[3] == 1);
      REQUIRE(content[7] == 2);
    }

    ShutdownTestframework();
  }
}
