#include <memory>
#include "CppUnitTest.h"

#include "fellow/hardfile/hunks/EndHunk.h"
#include "test/framework/TestBootstrap.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  TEST_CLASS(EndHunkTest)
  {
    unique_ptr<EndHunk> _instance;
    unique_ptr<RawDataReader> _rawDataReader;
    unique_ptr<UBY[]> _hunkData;

    void CreateHunkData()
    {
      _hunkData.reset(new UBY[0]);
    }

    RawDataReader &GetRawDataReaderWithHunkData()
    {
      CreateHunkData();
      _rawDataReader.reset(new RawDataReader(_hunkData.get(), 0));
      return *_rawDataReader;
    }

    TEST_METHOD_INITIALIZE(TestInitialize)
    {
      InitializeTestframework();
      _instance.reset(new EndHunk());
    }

    TEST_METHOD_CLEANUP(TestCleanup)
    {
      ShutdownTestframework();
    }

    TEST_METHOD(CanCreateInstance)
    {
      Assert::IsNotNull(_instance.get());
    }

    TEST_METHOD(GetID_ReturnsIDForEndHunk)
    {
      ULO id = _instance->GetID();
      Assert::AreEqual<ULO>(0x3f2, id);
    }

    TEST_METHOD(Parse_CanBeCalled_DoesNothingWithTheRawDataReader)
    {
      _instance->Parse(GetRawDataReaderWithHunkData()); // RawDataReader will throw exception if used
      Assert::IsTrue(true);
    }
  };
}
