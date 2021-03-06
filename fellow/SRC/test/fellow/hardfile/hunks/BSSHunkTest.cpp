#include <memory>
#include "CppUnitTest.h"
#include "fellow/hardfile/hunks/BSSHunk.h"
#include "test/framework/TestBootstrap.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  TEST_CLASS(BSSHunkTest)
  {
    unique_ptr<BSSHunk> _instance;
    const ULO AllocateSizeInLongwords = 4567;
    const ULO AllocateSizeInBytes = 18268;
    unique_ptr<RawDataReader> _rawDataReader;
    unique_ptr<UBY[]> _hunkData;

    void CreateHunkData()
    {
      _hunkData.reset(new UBY[12]);
      _hunkData[0] = 0;
      _hunkData[1] = 0;
      _hunkData[2] = 0;
      _hunkData[3] = 2;
      _hunkData[4] = 0;
      _hunkData[5] = 0;
      _hunkData[6] = 0;
      _hunkData[7] = 1;
      _hunkData[8] = 0;
      _hunkData[9] = 0;
      _hunkData[10] = 0;
      _hunkData[11] = 2;
    }

    RawDataReader& GetRawDataReaderWithHunkData()
    {
      CreateHunkData();
      _rawDataReader.reset(new RawDataReader(_hunkData.get(), 12));
      return *_rawDataReader;
    }

    TEST_METHOD_INITIALIZE(TestInitialize)
    {
      InitializeTestframework();
      _instance.reset(new BSSHunk(AllocateSizeInLongwords));
    }

    TEST_METHOD_CLEANUP(TestCleanup)
    {
      ShutdownTestframework();
    }

    TEST_METHOD(CanCreateInstance)
    {
      Assert::IsNotNull(_instance.get());
    }

    TEST_METHOD(GetAllocateSizeInLongwords_AllocateSizeIsSetInConstructor_ReturnsCorrectAllocateSize)
    {
      ULO allocateSizeInLongwords = _instance->GetAllocateSizeInLongwords();
      Assert::AreEqual(AllocateSizeInLongwords, allocateSizeInLongwords);
    }

    TEST_METHOD(GetAllocateSizeInBytes_AllocateSizeIsSetInConstructor_ReturnsCorrectAllocateSize)
    {
      ULO allocateSizeInBytes = _instance->GetAllocateSizeInBytes();
      Assert::AreEqual(AllocateSizeInBytes, allocateSizeInBytes);
    }

    TEST_METHOD(GetID_ReturnsIDForBSSHunk)
    {
      ULO id = _instance->GetID();
      Assert::AreEqual<ULO>(0x3eb, id);
    }

    TEST_METHOD(Parse_SetsContentSize)
    {
      _instance->Parse(GetRawDataReaderWithHunkData());
      Assert::AreEqual<ULO>(2, _instance->GetContentSizeInLongwords());
    }

    TEST_METHOD(Parse_SetsRawDataToZeroedContent)
    {
      _instance->Parse(GetRawDataReaderWithHunkData());
      UBY *content = _instance->GetContent();
      Assert::AreEqual<UBY>(0, content[0]);
      Assert::AreEqual<UBY>(0, content[1]);
      Assert::AreEqual<UBY>(0, content[2]);
      Assert::AreEqual<UBY>(0, content[3]);
      Assert::AreEqual<UBY>(0, content[4]);
      Assert::AreEqual<UBY>(0, content[5]);
      Assert::AreEqual<UBY>(0, content[6]);
      Assert::AreEqual<UBY>(0, content[7]);
    }
  };
}
