#include <memory>
#include "CppUnitTest.h"

#include "fellow/hardfile/hunks/HeaderHunk.h"
#include "test/framework/TestBootstrap.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  TEST_CLASS(HeaderHunkTest)
  {
    unique_ptr<HeaderHunk> _instance;
    unique_ptr<RawDataReader> _rawDataReader;
    unique_ptr<UBY[]> _hunkData;
    ULO _hunkDataSize;

    int CreateResidentLibraryList(int residentLibraryCount, ULO index)
    {
      for (int i = 0; i < residentLibraryCount; i++)
      {
        int sizeInLongWords = (i + 1);
        _hunkData[index++] = 0;
        _hunkData[index++] = 0;
        _hunkData[index++] = 0;
        _hunkData[index++] = static_cast<UBY>(sizeInLongWords);

        for (int j = 0; j < sizeInLongWords; j++)
        {
          _hunkData[index++] = 'A';
          _hunkData[index++] = 'B';
          _hunkData[index++] = 'C';
          _hunkData[index++] = (j == (sizeInLongWords - 1)) ? '\0' : 'D';
        }
      }

      // Terminate list
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;

      return index;
    }

    ULO CreateHunkTable(ULO index)
    {
      // Table size
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 2;

      // First hunk
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 1;

      // Last hunk
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 0;
      _hunkData[index++] = 4;

      // Hunk 1 size any memory flag
      _hunkData[index++] = 1;
      _hunkData[index++] = 2;
      _hunkData[index++] = 3;
      _hunkData[index++] = 4;

      // Hunk 2 size chip memory flag
      _hunkData[index++] = 0x41;
      _hunkData[index++] = 2;
      _hunkData[index++] = 3;
      _hunkData[index++] = 5;

      // Hunk 3 size additional flags
      _hunkData[index++] = 0xFF;
      _hunkData[index++] = 0x60;
      _hunkData[index++] = 0x70;
      _hunkData[index++] = 0x80;

      _hunkData[index++] = 0x11;
      _hunkData[index++] = 0x22;
      _hunkData[index++] = 0x33;
      _hunkData[index++] = 0x44;

      // Hunk 4 size fast memory flag
      _hunkData[index++] = 0x81;
      _hunkData[index++] = 2;
      _hunkData[index++] = 3;
      _hunkData[index++] = 7;

      return index;
    }

    void CreateHunkData(int residentLibraryCount)
    {
      ULO index = 0;
      _hunkData.reset(new UBY[128]);

      index = CreateResidentLibraryList(residentLibraryCount, index);
      _hunkDataSize = CreateHunkTable(index);
    }

    RawDataReader& GetNewRawDataReader()
    {
      _rawDataReader.reset(new RawDataReader(_hunkData.get(), _hunkDataSize));
      return *_rawDataReader;
    }

    TEST_METHOD_INITIALIZE(TestInitialize)
    {
      InitializeTestframework();
      _instance.reset(new HeaderHunk());
    }

    TEST_METHOD_CLEANUP(TestCleanup)
    {
      ShutdownTestframework();
    }

    TEST_METHOD(CanCreateInstance)
    {
      Assert::IsNotNull(_instance.get());
    }

    TEST_METHOD(GetID_ReturnsIDForHeaderHunk)
    {
      ULO id = _instance->GetID();

      Assert::AreEqual<ULO>(0x3f3, id);
    }

    TEST_METHOD(Parse_TwoResidentLibraries_ResidentLibraryNamesFound)
    {
      CreateHunkData(2);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<size_t>(2, _instance->GetResidentLibraryCount());
      Assert::AreEqual<string>("ABC", _instance->GetResidentLibrary(0));
      Assert::AreEqual<string>("ABCDABC", _instance->GetResidentLibrary(1));
    }

    TEST_METHOD(Parse_HunkTableSize_HunkSizeTableHasFourEntries)
    {
      CreateHunkData(2);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<size_t>(4, _instance->GetHunkSizeCount());
    }

    TEST_METHOD(Parse_HunkTableFirstLoadHunk_FirstLoadHunkIs1)
    {
      CreateHunkData(2);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<ULO>(1, _instance->GetFirstLoadHunk());
    }

    TEST_METHOD(Parse_HunkTableLastLoadHunk_LastLoadHunkIs4)
    {
      CreateHunkData(2);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<ULO>(4, _instance->GetLastLoadHunk());
    }

    TEST_METHOD(Parse_HunkTableSizes_FirstHunkSizeAndAnyMemoryFlag)
    {
      CreateHunkData(2);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<ULO>(0x01020304, _instance->GetHunkSize(0).SizeInLongwords);
      Assert::AreEqual<ULO>(0, _instance->GetHunkSize(0).MemoryFlags);
      Assert::AreEqual<ULO>(0, _instance->GetHunkSize(0).AdditionalFlags);
    }

    TEST_METHOD(Parse_HunkTableSizes_SecondHunkSizeAndChipMemoryFlag)
    {
      CreateHunkData(2);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<ULO>(0x01020305, _instance->GetHunkSize(1).SizeInLongwords);
      Assert::AreEqual<ULO>(1, _instance->GetHunkSize(1).MemoryFlags);
      Assert::AreEqual<ULO>(0, _instance->GetHunkSize(1).AdditionalFlags);
    }

    TEST_METHOD(Parse_HunkTableSizes_ThirdHunkSizeAndAdditionalMemoryFlags)
    {
      CreateHunkData(2);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<ULO>(0x3F607080, _instance->GetHunkSize(2).SizeInLongwords);
      Assert::AreEqual<ULO>(3, _instance->GetHunkSize(2).MemoryFlags);
      Assert::AreEqual<ULO>(0x11223344, _instance->GetHunkSize(2).AdditionalFlags);
    }

    TEST_METHOD(Parse_HunkTableSizes_LastHunkSizeAndFastMemoryFlag)
    {
      CreateHunkData(2);

      _instance->Parse(GetNewRawDataReader());

      Assert::AreEqual<ULO>(0x01020307, _instance->GetHunkSize(3).SizeInLongwords);
      Assert::AreEqual<ULO>(2, _instance->GetHunkSize(3).MemoryFlags);
      Assert::AreEqual<ULO>(0, _instance->GetHunkSize(3).AdditionalFlags);
    }
  };
}
