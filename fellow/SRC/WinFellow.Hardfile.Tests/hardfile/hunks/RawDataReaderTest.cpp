#include <memory>
#include <stdexcept>
#include "hardfile/hunks/RawDataReader.h"
#include "TestBootstrap.h"
#include "catch/catch_amalgamated.hpp"

using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  unique_ptr<uint8_t[]> CreateRawData()
  {
    int index = 0;
    unique_ptr<uint8_t[]> rawData(new uint8_t[12]);
    rawData[index++] = 'A';
    rawData[index++] = 'B';
    rawData[index++] = 'C';
    rawData[index++] = '\0';

    rawData[index++] = 'A';
    rawData[index++] = 'B';
    rawData[index++] = 'C';
    rawData[index++] = 'D';
    rawData[index++] = 'E';
    rawData[index++] = 'F';
    rawData[index++] = 'G';
    rawData[index] = 'H';
    return rawData;
  }

  TEST_CASE("Hardfile::Hunks::RawDataReader.GetNextString() returns string")
  {
    InitializeTestframework();
    auto _rawData = CreateRawData();
    unique_ptr<RawDataReader> _instance(new RawDataReader(_rawData.get(), 12));

    SECTION("Reads string")
    {
      string value = _instance->GetNextString(1);
      REQUIRE(value == "ABC");
    }

    SECTION("Reads two strings")
    {
      _instance->GetNextString(1);
      string value = _instance->GetNextString(2);
      REQUIRE(value == "ABCDEFGH");
    }

    SECTION("Reads empty string")
    {
      string value = _instance->GetNextString(0);
      REQUIRE(value.empty() == true);
    }

    SECTION("Reads string that terminates early and next read index is after full block")
    {
      string value = _instance->GetNextString(3);
      REQUIRE(value == "ABC");
      REQUIRE(_instance->GetIndex() == 12);
    }

    SECTION("Throws out_of_range exception when reading past rawData length")
    {
      REQUIRE_THROWS_AS(_instance->GetNextString(4), out_of_range);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::RawDataReader.GetNextByteswappedLong() returns long")
  {
    InitializeTestframework();
    auto _rawData = CreateRawData();
    unique_ptr<RawDataReader> _instance(new RawDataReader(_rawData.get(), 12));

    SECTION("Reads long")
    {
      uint32_t value = _instance->GetNextByteswappedLong();
      REQUIRE(value == 0x41424300);
    }

    SECTION("Reads two longs")
    {
      uint32_t value1 = _instance->GetNextByteswappedLong();
      uint32_t value2 = _instance->GetNextByteswappedLong();
      REQUIRE(value1 == 0x41424300);
      REQUIRE(value2 == 0x41424344);
    }

    SECTION("Throws out_of_range exception when reading past rawData length")
    {
      _instance->GetNextByteswappedLong();
      _instance->GetNextByteswappedLong();
      _instance->GetNextByteswappedLong();
      REQUIRE_THROWS_AS(_instance->GetNextByteswappedLong(), out_of_range);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::Hunks::RawDataReader.GetNextBytes() returns array of bytes")
  {
    InitializeTestframework();
    auto _rawData = CreateRawData();
    unique_ptr<RawDataReader> _instance(new RawDataReader(_rawData.get(), 12));

    SECTION("Reads next two longwords as byte array")
    {
      unique_ptr<UBY> value(_instance->GetNextBytes(2));
      REQUIRE(value.get()[0] == 0x41);
      REQUIRE(value.get()[1] == 0x42);
      REQUIRE(value.get()[2] == 0x43);
      REQUIRE(value.get()[3] == 0x00);
      REQUIRE(value.get()[4] == 0x41);
      REQUIRE(value.get()[5] == 0x42);
      REQUIRE(value.get()[6] == 0x43);
      REQUIRE(value.get()[7] == 0x44);
    }

    SECTION("Reads twice and second byte array contains data from the correct index")
    {
      unique_ptr<UBY> value1(_instance->GetNextBytes(1));
      unique_ptr<UBY> value2(_instance->GetNextBytes(2));
      REQUIRE(value2.get()[0] == 0x41);
      REQUIRE(value2.get()[1] == 0x42);
      REQUIRE(value2.get()[2] == 0x43);
      REQUIRE(value2.get()[3] == 0x44);
      REQUIRE(value2.get()[4] == 0x45);
      REQUIRE(value2.get()[5] == 0x46);
      REQUIRE(value2.get()[6] == 0x47);
      REQUIRE(value2.get()[7] == 0x48);
    }

    SECTION("Throws out_of_range exception when reading past rawData length")
    {
      REQUIRE_THROWS_AS(_instance->GetNextBytes(4), out_of_range);
    }

    ShutdownTestframework();
  }
}
