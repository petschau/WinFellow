#include <memory>
#include "TestBootstrap.h"
#include "hardfile/hunks/HeaderHunk.h"
#include "catch/catch_amalgamated.hpp"

using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  unsigned int CreateResidentLibraryList(uint8_t* hunkData, unsigned int residentLibraryCount, unsigned int index)
  {
    for (unsigned int i = 0; i < residentLibraryCount; i++)
    {
      unsigned int sizeInLongWords = (i + 1);
      hunkData[index++] = 0;
      hunkData[index++] = 0;
      hunkData[index++] = 0;
      hunkData[index++] = static_cast<uint8_t>(sizeInLongWords);

      for (unsigned int j = 0; j < sizeInLongWords; j++)
      {
        hunkData[index++] = 'A';
        hunkData[index++] = 'B';
        hunkData[index++] = 'C';
        hunkData[index++] = (j == (sizeInLongWords - 1)) ? '\0' : 'D';
      }
    }

    // Terminate list
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 0;

    return index;
  }

  unsigned int CreateHunkTable(uint8_t* hunkData, unsigned int index)
  {
    // Table size
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 2;

    // First hunk
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 1;

    // Last hunk
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 0;
    hunkData[index++] = 4;

    // Hunk 1 size any memory flag
    hunkData[index++] = 1;
    hunkData[index++] = 2;
    hunkData[index++] = 3;
    hunkData[index++] = 4;

    // Hunk 2 size chip memory flag
    hunkData[index++] = 0x41;
    hunkData[index++] = 2;
    hunkData[index++] = 3;
    hunkData[index++] = 5;

    // Hunk 3 size additional flags
    hunkData[index++] = 0xFF;
    hunkData[index++] = 0x60;
    hunkData[index++] = 0x70;
    hunkData[index++] = 0x80;

    hunkData[index++] = 0x11;
    hunkData[index++] = 0x22;
    hunkData[index++] = 0x33;
    hunkData[index++] = 0x44;

    // Hunk 4 size fast memory flag
    hunkData[index++] = 0x81;
    hunkData[index++] = 2;
    hunkData[index++] = 3;
    hunkData[index++] = 7;

    return index;
  }

  unsigned int FillHeaderHunkData(uint8_t* hunkData, unsigned int residentLibraryCount)
  {
    unsigned int index = 0;
    index = CreateResidentLibraryList(hunkData, residentLibraryCount, index);
    return CreateHunkTable(hunkData, index);
  }

  unique_ptr<uint8_t[]> CreateHeaderHunkData(unsigned int allocateSize)
  {
    return unique_ptr<uint8_t[]>(new uint8_t[allocateSize]);
  }

  TEST_CASE("Hardfile::Hunks::HeaderHunk.GetID() returns ID for HeaderHunk")
  {
    InitializeTestframework();
    unique_ptr<HeaderHunk> _instance(new HeaderHunk());

    SECTION("Returns ID for EndHunk")
    {
      uint32_t id = _instance->GetID();
      REQUIRE(id == 0x3f3);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::HeaderHunk.Parse() should find two resident library names")
  {
    InitializeTestframework();
    unique_ptr<HeaderHunk> _instance(new HeaderHunk());

    SECTION("Should find two resident library names")
    {
      auto hunkData = CreateHeaderHunkData(128);
      unsigned int actualHunkDataSize = FillHeaderHunkData(hunkData.get(), 2);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), actualHunkDataSize));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetResidentLibraryCount() == 2);
      REQUIRE(_instance->GetResidentLibrary(0) == "ABC");
      REQUIRE(_instance->GetResidentLibrary(1) == "ABCDABC");
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::HeaderHunk.Parse() should find four hunk size table entries")
  {
    InitializeTestframework();
    unique_ptr<HeaderHunk> _instance(new HeaderHunk());

    SECTION("Should find four hunk size table entries")
    {
      auto hunkData = CreateHeaderHunkData(128);
      unsigned int actualHunkDataSize = FillHeaderHunkData(hunkData.get(), 2);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), actualHunkDataSize));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetHunkSizeCount() == 4);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::HeaderHunk.Parse() first load hunk should be 1")
  {
    InitializeTestframework();
    unique_ptr<HeaderHunk> _instance(new HeaderHunk());

    SECTION("First load hunk should be 1")
    {
      auto hunkData = CreateHeaderHunkData(128);
      unsigned int actualHunkDataSize = FillHeaderHunkData(hunkData.get(), 2);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), actualHunkDataSize));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetFirstLoadHunk() == 1);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::HeaderHunk.Parse() last load hunk should be 4")
  {
    InitializeTestframework();
    unique_ptr<HeaderHunk> _instance(new HeaderHunk());

    SECTION("Last load hunk should be 4")
    {
      auto hunkData = CreateHeaderHunkData(128);
      unsigned int actualHunkDataSize = FillHeaderHunkData(hunkData.get(), 2);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), actualHunkDataSize));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetLastLoadHunk() == 4);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::HeaderHunk.Parse() should read first hunk size and flags correctly")
  {
    InitializeTestframework();
    unique_ptr<HeaderHunk> _instance(new HeaderHunk());

    SECTION("Should read first hunk size and flags correctly")
    {
      auto hunkData = CreateHeaderHunkData(128);
      unsigned int actualHunkDataSize = FillHeaderHunkData(hunkData.get(), 2);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), actualHunkDataSize));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetHunkSize(0).SizeInLongwords == 0x01020304);
      REQUIRE(_instance->GetHunkSize(0).MemoryFlags == 0);
      REQUIRE(_instance->GetHunkSize(0).AdditionalFlags == 0);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::HeaderHunk.Parse() should read second hunk size and flags correctly")
  {
    InitializeTestframework();
    unique_ptr<HeaderHunk> _instance(new HeaderHunk());

    SECTION("Should read second hunk size and flags correctly")
    {
      auto hunkData = CreateHeaderHunkData(128);
      unsigned int actualHunkDataSize = FillHeaderHunkData(hunkData.get(), 2);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), actualHunkDataSize));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetHunkSize(1).SizeInLongwords == 0x01020305);
      REQUIRE(_instance->GetHunkSize(1).MemoryFlags == 1);
      REQUIRE(_instance->GetHunkSize(1).AdditionalFlags == 0);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::HeaderHunk.Parse() should read third hunk size and flags correctly")
  {
    InitializeTestframework();
    unique_ptr<HeaderHunk> _instance(new HeaderHunk());

    SECTION("Should read third hunk size and flags correctly")
    {
      auto hunkData = CreateHeaderHunkData(128);
      unsigned int actualHunkDataSize = FillHeaderHunkData(hunkData.get(), 2);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), actualHunkDataSize));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetHunkSize(2).SizeInLongwords == 0x3F607080);
      REQUIRE(_instance->GetHunkSize(2).MemoryFlags == 3);
      REQUIRE(_instance->GetHunkSize(2).AdditionalFlags == 0x11223344);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::HeaderHunk.Parse() should read fourth hunk size and flags correctly")
  {
    InitializeTestframework();
    unique_ptr<HeaderHunk> _instance(new HeaderHunk());

    SECTION("Should read fourth hunk size and flags correctly")
    {
      auto hunkData = CreateHeaderHunkData(128);
      unsigned int actualHunkDataSize = FillHeaderHunkData(hunkData.get(), 2);
      unique_ptr<RawDataReader> rawDataReader(new RawDataReader(hunkData.get(), actualHunkDataSize));

      _instance->Parse(*rawDataReader);

      REQUIRE(_instance->GetHunkSize(3).SizeInLongwords == 0x01020307);
      REQUIRE(_instance->GetHunkSize(3).MemoryFlags == 2);
      REQUIRE(_instance->GetHunkSize(3).AdditionalFlags == 0);
    }

    ShutdownTestframework();
  }
}
