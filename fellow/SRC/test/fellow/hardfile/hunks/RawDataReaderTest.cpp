#include <memory>
#include <stdexcept>
#include "CppUnitTest.h"

#include "fellow/hardfile/hunks/RawDataReader.h"
#include "test/framework/TestBootstrap.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  TEST_CLASS(RawDataReaderTest)
  {
    unique_ptr<UBY[]> _rawData;
    unique_ptr<RawDataReader> _instance;

    void CreateRawData()
    {
      int index = 0;
      _rawData.reset(new UBY[12]);
      _rawData[index++] = 'A';
      _rawData[index++] = 'B';
      _rawData[index++] = 'C';
      _rawData[index++] = '\0';

      _rawData[index++] = 'A';
      _rawData[index++] = 'B';
      _rawData[index++] = 'C';
      _rawData[index++] = 'D';
      _rawData[index++] = 'E';
      _rawData[index++] = 'F';
      _rawData[index++] = 'G';
      _rawData[index++] = 'H';
    }

    TEST_METHOD_INITIALIZE(TestInitialize)
    {
      InitializeTestframework();
      CreateRawData();
      _instance.reset(new RawDataReader(_rawData.get(), 12));
    }

    TEST_METHOD_CLEANUP(TestCleanup)
    {
      ShutdownTestframework();
    }

    TEST_METHOD(CanCreateInstance)
    {
      Assert::IsNotNull(_instance.get());
    }

    TEST_METHOD(GetNextString_ReadOneString_ReturnsCorrectString)
    {
      string value = _instance->GetNextString(1);
      Assert::AreEqual(value, string("ABC"));
    }

    TEST_METHOD(GetNextString_ReadTwoStrings_ReturnsCorrectStrings)
    {
      _instance->GetNextString(1);
      string value = _instance->GetNextString(2);
      Assert::AreEqual(value, string("ABCDEFGH"));
    }

    TEST_METHOD(GetNextString_ReadEmptyString_ReturnsEmptyString)
    {
      string value = _instance->GetNextString(0);
      Assert::IsTrue(value.empty());
    }

    TEST_METHOD(GetNextString_ReadStringTerminatedEarly_ReturnsShortStringAndNextIndexIsAfterFullBlock)
    {
      string value = _instance->GetNextString(3);
      Assert::AreEqual(value, string("ABC"));
      Assert::AreEqual<ULO>(12, _instance->GetIndex());
    }

    TEST_METHOD(GetNextString_ReadStringPastDataLength_ThrowsException)
    {
      RawDataReader *reader = _instance.get();
      Assert::ExpectException<out_of_range>([reader] {reader->GetNextString(4); });
    }

    TEST_METHOD(GetNextByteswappedLong_ReadOneLong_ReturnsCorrectLong)
    {
      ULO value = _instance->GetNextByteswappedLong();
      Assert::AreEqual<ULO>(0x41424300, value);
    }

    TEST_METHOD(GetNextByteswappedLong_ReadTwoLongs_ReturnsCorrectLongs)
    {
      ULO value1 = _instance->GetNextByteswappedLong();
      ULO value2 = _instance->GetNextByteswappedLong();
      Assert::AreEqual<ULO>(0x41424300, value1);
      Assert::AreEqual<ULO>(0x41424344, value2);
    }

    TEST_METHOD(GetNextByteswappedLong_ReadLongwordsPastDataLength_ThrowsException)
    {
      RawDataReader *reader = _instance.get();
      reader->GetNextByteswappedLong();
      reader->GetNextByteswappedLong();
      reader->GetNextByteswappedLong();
      Assert::ExpectException<out_of_range>([reader] {reader->GetNextByteswappedLong(); });
    }

    TEST_METHOD(GetNextBytes_ReadTwoLongwords_ReturnsPointerToBytes)
    {
      unique_ptr<UBY> value(_instance->GetNextBytes(2));
      Assert::AreEqual<UBY>(0x41, value.get()[0]);
      Assert::AreEqual<UBY>(0x42, value.get()[1]);
      Assert::AreEqual<UBY>(0x43, value.get()[2]);
      Assert::AreEqual<UBY>(0x00, value.get()[3]);
      Assert::AreEqual<UBY>(0x41, value.get()[4]);
      Assert::AreEqual<UBY>(0x42, value.get()[5]);
      Assert::AreEqual<UBY>(0x43, value.get()[6]);
      Assert::AreEqual<UBY>(0x44, value.get()[7]);
    }

    TEST_METHOD(GetNextBytes_ReadBytesPastDataLength_ThrowsException)
    {
      RawDataReader *reader = _instance.get();
      Assert::ExpectException<out_of_range>([reader] {reader->GetNextBytes(4); });
    }

    TEST_METHOD(GetNextBytes_ReadLongwordTwoTimes_ReturnsPointersToCorrectBytes)
    {
      unique_ptr<UBY> value1(_instance->GetNextBytes(1));
      unique_ptr<UBY> value2(_instance->GetNextBytes(2));
      Assert::AreEqual<UBY>(0x41, value1.get()[0]);
      Assert::AreEqual<UBY>(0x42, value1.get()[1]);
      Assert::AreEqual<UBY>(0x43, value1.get()[2]);
      Assert::AreEqual<UBY>(0x00, value1.get()[3]);
      Assert::AreEqual<UBY>(0x41, value2.get()[0]);
      Assert::AreEqual<UBY>(0x42, value2.get()[1]);
      Assert::AreEqual<UBY>(0x43, value2.get()[2]);
      Assert::AreEqual<UBY>(0x44, value2.get()[3]);
      Assert::AreEqual<UBY>(0x45, value2.get()[4]);
      Assert::AreEqual<UBY>(0x46, value2.get()[5]);
      Assert::AreEqual<UBY>(0x47, value2.get()[6]);
      Assert::AreEqual<UBY>(0x48, value2.get()[7]);
    }
  };
}
