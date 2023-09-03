#include <memory>
#include "CppUnitTest.h"

#include "hardfile/hunks/Reloc32Hunk.h"
#include "framework/TestBootstrap.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  TEST_CLASS(Reloc32HunkTest)
  {
    unique_ptr<Reloc32Hunk> _instance;
    const int SourceHunkIndex = 1234;
    unique_ptr<RawDataReader> _rawDataReader;
    unique_ptr<UBY[]> _hunkData;

    ULO CreateOffsetTable(int entries, ULO index)
    {
      // Entry count
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = static_cast<UBY>(entries);

      // Related hunk number
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = static_cast<UBY>(entries + 1);

      for (int i = 0; i < entries; i++)
      {
        _hunkData[index++] = 0;
        _hunkData[index++] = 0;
        _hunkData[index++] = 0;
        _hunkData[index++] = static_cast<UBY>(i + 1);
      }

      return index;
    }

    void CreateHunkData(int offsetTableCount)
    {
      ULO index = 0;
      _hunkData.reset(new UBY[128]);

      for (int i = 0; i < offsetTableCount; i++)
      {
        index = CreateOffsetTable(offsetTableCount + 2, index);
      }

      // Terminate offset list
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
    }

    RawDataReader& GetNewRawDataReader()
    {
      _rawDataReader.reset(new RawDataReader(_hunkData.get(), 128));
      return *_rawDataReader;
    }

    TEST_METHOD_INITIALIZE(TestInitialize)
    {
      InitializeTestframework();
      _instance.reset(new Reloc32Hunk(SourceHunkIndex));
    }

    TEST_METHOD_CLEANUP(TestCleanup)
    {
      ShutdownTestframework();
    }

    TEST_METHOD(CanCreateInstance)
    {
      Assert::IsNotNull(_instance.get());
    }

    TEST_METHOD(GetID_ReturnsIDForReloc32Hunk)
    {
      ULO id = _instance->GetID();
      Assert::AreEqual<ULO>(0x3ec, id);
    }

    TEST_METHOD(GetSourceHunkIndex_ReturnsSourceHunkIndexForReloc32Hunk)
    {
      ULO sourceHunkIndex = _instance->GetSourceHunkIndex();
      Assert::AreEqual<ULO>(SourceHunkIndex, sourceHunkIndex);
    }

    TEST_METHOD(Parse_NoOffsetTables_OffsetTableCountIsZero)
    {
      CreateHunkData(0);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<size_t>(0, _instance->GetOffsetTableCount());
    }

    TEST_METHOD(Parse_OneOffsetTable_OffsetTableContentIsCorrect)
    {
      CreateHunkData(1);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<size_t>(1, _instance->GetOffsetTableCount());
      Assert::AreEqual<size_t>(3, _instance->GetOffsetTable(0)->GetOffsetCount());
      Assert::AreEqual<ULO>(4, _instance->GetOffsetTable(0)->GetRelatedHunkIndex());
      Assert::AreEqual<ULO>(1, _instance->GetOffsetTable(0)->GetOffset(0));
      Assert::AreEqual<ULO>(2, _instance->GetOffsetTable(0)->GetOffset(1));
      Assert::AreEqual<ULO>(3, _instance->GetOffsetTable(0)->GetOffset(2));
    }

    TEST_METHOD(Parse_TwoOffsetTables_OffsetTableContentIsCorrect)
    {
      CreateHunkData(2);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<size_t>(2, _instance->GetOffsetTableCount());
      Assert::AreEqual<size_t>(4, _instance->GetOffsetTable(1)->GetOffsetCount());
      Assert::AreEqual<ULO>(5, _instance->GetOffsetTable(1)->GetRelatedHunkIndex());
      Assert::AreEqual<ULO>(1, _instance->GetOffsetTable(1)->GetOffset(0));
      Assert::AreEqual<ULO>(2, _instance->GetOffsetTable(1)->GetOffset(1));
      Assert::AreEqual<ULO>(3, _instance->GetOffsetTable(1)->GetOffset(2));
      Assert::AreEqual<ULO>(4, _instance->GetOffsetTable(1)->GetOffset(3));
    }
  };
}
