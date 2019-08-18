#include <memory>
#include "CppUnitTest.h"

#include "fellow/hardfile/hunks/HunkParser.h"
#include "fellow/hardfile/hunks/Reloc32Hunk.h"
#include "test/framework/TestBootstrap.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  TEST_CLASS(HunkParserTest)
  {
    unique_ptr<HunkParser> _instance;
    unique_ptr<UBY> _rawData;
    FileImage _fileImage;

    ULO LoadFile(const char *filename)
    {
      FILE *F = fopen(filename, "rb");
      fseek(F, 0, SEEK_END);
      long size = ftell(F);
      _rawData.reset(new UBY[size]);
      fseek(F, 0, SEEK_SET);
      fread(_rawData.get(), 1, size, F);
      fclose(F);
      return size;
    }

    void CreateInstanceWithFile(const char *filename)
    {
      ULO size = LoadFile(filename);
      _instance.reset(new HunkParser(_rawData.get(), size, _fileImage));
    }

    TEST_METHOD_INITIALIZE(TestInitialize)
    {
      InitializeTestframework();
    }

    TEST_METHOD_CLEANUP(TestCleanup)
    {
      ShutdownTestframework();
    }

    TEST_METHOD(Parse_SmartFilesystem_FileReadsOK)
    {
      CreateInstanceWithFile(R"(testdata\fellow\hardfile\hunks\SmartFilesystem)");

      bool result = _instance->Parse();

      Assert::IsTrue(result);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetFirstLoadHunk());
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetLastLoadHunk());
      Assert::AreEqual<size_t>(1, _fileImage.GetHeader()->GetHunkSizeCount());
      Assert::AreEqual<ULO>(0x5fbe, _fileImage.GetHeader()->GetHunkSize(0).SizeInLongwords);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetHunkSize(0).MemoryFlags);

      Assert::AreEqual<size_t>(1, _fileImage.GetInitialHunkCount());
      Assert::AreEqual<size_t>(1, _fileImage.GetAdditionalHunkCount());

      Assert::AreEqual<ULO>(CodeHunkID, _fileImage.GetInitialHunk(0)->GetID());
      Assert::AreEqual<ULO>(0x5fbe * 4, _fileImage.GetInitialHunk(0)->GetAllocateSizeInBytes());
      Assert::AreEqual<ULO>(0x5fbe * 4, _fileImage.GetInitialHunk(0)->GetContentSizeInBytes());

      UBY firstByte = _fileImage.GetInitialHunk(0)->GetContent()[0];
      UBY middleByte = _fileImage.GetInitialHunk(0)->GetContent()[0xbf7d];
      UBY lastByte = _fileImage.GetInitialHunk(0)->GetContent()[0x5fbe * 4 - 1];
      Assert::AreEqual<UBY>(0x60, firstByte);
      Assert::AreEqual<UBY>(0x0c, middleByte);
      Assert::AreEqual<UBY>(0, lastByte);

      Assert::AreEqual<ULO>(Reloc32HunkID, _fileImage.GetAdditionalHunk(0)->GetID());
      Reloc32Hunk *reloc32Hunk = dynamic_cast<Reloc32Hunk *>(_fileImage.GetAdditionalHunk(0));

      Assert::AreEqual<size_t>(1, reloc32Hunk->GetOffsetTableCount());
      Assert::AreEqual<ULO>(0, reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex());
      Assert::AreEqual<size_t>(0x28, reloc32Hunk->GetOffsetTable(0)->GetOffsetCount());

      ULO firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      ULO middleOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(14);
      ULO lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(39);

      Assert::AreEqual<ULO>(0x86, firstOffset);
      Assert::AreEqual<ULO>(0x7d1e, middleOffset);
      Assert::AreEqual<ULO>(0x17db6, lastOffset);
    }

    TEST_METHOD(Parse_ProfFileSystem195_FileReadsOK)
    {
      CreateInstanceWithFile(R"(testdata\fellow\hardfile\hunks\ProfFileSystem_195)");

      bool result = _instance->Parse();

      Assert::IsTrue(result);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetFirstLoadHunk());
      Assert::AreEqual<ULO>(1, _fileImage.GetHeader()->GetLastLoadHunk());
      Assert::AreEqual<size_t>(2, _fileImage.GetHeader()->GetHunkSizeCount());
      Assert::AreEqual<ULO>(0x1b4f, _fileImage.GetHeader()->GetHunkSize(0).SizeInLongwords);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetHunkSize(0).MemoryFlags);
      Assert::AreEqual<ULO>(0x13d, _fileImage.GetHeader()->GetHunkSize(1).SizeInLongwords);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetHunkSize(1).MemoryFlags);

      Assert::AreEqual<size_t>(2, _fileImage.GetInitialHunkCount());
      Assert::AreEqual<size_t>(2, _fileImage.GetAdditionalHunkCount());

      // Code hunk
      Assert::AreEqual<ULO>(CodeHunkID, _fileImage.GetInitialHunk(0)->GetID());
      Assert::AreEqual<ULO>(0x1b4f * 4, _fileImage.GetInitialHunk(0)->GetAllocateSizeInBytes());
      Assert::AreEqual<ULO>(0x1b4f * 4, _fileImage.GetInitialHunk(0)->GetContentSizeInBytes());

      UBY firstByte = _fileImage.GetInitialHunk(0)->GetContent()[0];
      UBY middleByte = _fileImage.GetInitialHunk(0)->GetContent()[0x2000];
      UBY lastByte = _fileImage.GetInitialHunk(0)->GetContent()[0x1b4f * 4 - 1];
      Assert::AreEqual<UBY>(0x9e, firstByte);
      Assert::AreEqual<UBY>(0x01, middleByte);
      Assert::AreEqual<UBY>(0x75, lastByte);

      // Data hunk
      Assert::AreEqual<ULO>(DataHunkID, _fileImage.GetInitialHunk(1)->GetID());
      Assert::AreEqual<ULO>(0x13d * 4, _fileImage.GetInitialHunk(1)->GetAllocateSizeInBytes());
      Assert::AreEqual<ULO>(0x13d * 4, _fileImage.GetInitialHunk(1)->GetContentSizeInBytes());

      firstByte = _fileImage.GetInitialHunk(1)->GetContent()[0];
      middleByte = _fileImage.GetInitialHunk(1)->GetContent()[0x200];
      lastByte = _fileImage.GetInitialHunk(1)->GetContent()[0x13d * 4 - 1];
      Assert::AreEqual<UBY>(0, firstByte);
      Assert::AreEqual<UBY>(0x25, middleByte);
      Assert::AreEqual<UBY>(0, lastByte);

      // First reloc 32 hunk
      Assert::AreEqual<ULO>(Reloc32HunkID, _fileImage.GetAdditionalHunk(0)->GetID());
      Reloc32Hunk *reloc32Hunk = dynamic_cast<Reloc32Hunk *>(_fileImage.GetAdditionalHunk(0));

      Assert::AreEqual<size_t>(1, reloc32Hunk->GetOffsetTableCount());
      Assert::AreEqual<ULO>(1, reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex());
      Assert::AreEqual<size_t>(2, reloc32Hunk->GetOffsetTable(0)->GetOffsetCount());

      ULO firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      ULO lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(1);

      Assert::AreEqual<ULO>(0x1dae, firstOffset);
      Assert::AreEqual<ULO>(0xa, lastOffset);

      // Second reloc 32 hunk
      Assert::AreEqual<ULO>(Reloc32HunkID, _fileImage.GetAdditionalHunk(1)->GetID());
      reloc32Hunk = dynamic_cast<Reloc32Hunk *>(_fileImage.GetAdditionalHunk(1));

      Assert::AreEqual<size_t>(1, reloc32Hunk->GetOffsetTableCount());
      Assert::AreEqual<ULO>(1, reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex());
      Assert::AreEqual<size_t>(4, reloc32Hunk->GetOffsetTable(0)->GetOffsetCount());

      firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(3);

      Assert::AreEqual<ULO>(0x2fe, firstOffset);
      Assert::AreEqual<ULO>(0x3a, lastOffset);
    }

    TEST_METHOD(Parse_ProfFileSystem195_68020_FileReadsOK)
    {
      CreateInstanceWithFile(R"(testdata\fellow\hardfile\hunks\ProfFileSystem_195_68020+)");

      bool result = _instance->Parse();

      Assert::IsTrue(result);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetFirstLoadHunk());
      Assert::AreEqual<ULO>(1, _fileImage.GetHeader()->GetLastLoadHunk());
      Assert::AreEqual<size_t>(2, _fileImage.GetHeader()->GetHunkSizeCount());
      Assert::AreEqual<ULO>(0x1abd, _fileImage.GetHeader()->GetHunkSize(0).SizeInLongwords);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetHunkSize(0).MemoryFlags);
      Assert::AreEqual<ULO>(0x13d, _fileImage.GetHeader()->GetHunkSize(1).SizeInLongwords);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetHunkSize(1).MemoryFlags);

      Assert::AreEqual<size_t>(2, _fileImage.GetInitialHunkCount());
      Assert::AreEqual<size_t>(2, _fileImage.GetAdditionalHunkCount());

      // Code hunk
      Assert::AreEqual<ULO>(CodeHunkID, _fileImage.GetInitialHunk(0)->GetID());
      Assert::AreEqual<ULO>(0x1abd * 4, _fileImage.GetInitialHunk(0)->GetAllocateSizeInBytes());
      Assert::AreEqual<ULO>(0x1abd * 4, _fileImage.GetInitialHunk(0)->GetContentSizeInBytes());

      UBY firstByte = _fileImage.GetInitialHunk(0)->GetContent()[0];
      UBY middleByte = _fileImage.GetInitialHunk(0)->GetContent()[0x2000];
      UBY lastByte = _fileImage.GetInitialHunk(0)->GetContent()[0x1abd * 4 - 1];
      Assert::AreEqual<UBY>(0x9e, firstByte);
      Assert::AreEqual<UBY>(0x30, middleByte);
      Assert::AreEqual<UBY>(0x75, lastByte);

      // Data hunk
      Assert::AreEqual<ULO>(DataHunkID, _fileImage.GetInitialHunk(1)->GetID());
      Assert::AreEqual<ULO>(0x13d * 4, _fileImage.GetInitialHunk(1)->GetAllocateSizeInBytes());
      Assert::AreEqual<ULO>(0x13d * 4, _fileImage.GetInitialHunk(1)->GetContentSizeInBytes());

      firstByte = _fileImage.GetInitialHunk(1)->GetContent()[0];
      middleByte = _fileImage.GetInitialHunk(1)->GetContent()[0x200];
      lastByte = _fileImage.GetInitialHunk(1)->GetContent()[0x13d * 4 - 1];
      Assert::AreEqual<UBY>(0, firstByte);
      Assert::AreEqual<UBY>(0x25, middleByte);
      Assert::AreEqual<UBY>(0, lastByte);

      // First reloc 32 hunk
      Assert::AreEqual<ULO>(Reloc32HunkID, _fileImage.GetAdditionalHunk(0)->GetID());
      Reloc32Hunk *reloc32Hunk = dynamic_cast<Reloc32Hunk *>(_fileImage.GetAdditionalHunk(0));

      Assert::AreEqual<size_t>(1, reloc32Hunk->GetOffsetTableCount());
      Assert::AreEqual<ULO>(1, reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex());
      Assert::AreEqual<size_t>(2, reloc32Hunk->GetOffsetTable(0)->GetOffsetCount());

      ULO firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      ULO lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(1);

      Assert::AreEqual<ULO>(0x1d7e, firstOffset);
      Assert::AreEqual<ULO>(0xa, lastOffset);

      // Second reloc 32 hunk
      Assert::AreEqual<ULO>(Reloc32HunkID, _fileImage.GetAdditionalHunk(1)->GetID());
      reloc32Hunk = dynamic_cast<Reloc32Hunk *>(_fileImage.GetAdditionalHunk(1));

      Assert::AreEqual<size_t>(1, reloc32Hunk->GetOffsetTableCount());
      Assert::AreEqual<ULO>(1, reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex());
      Assert::AreEqual<size_t>(4, reloc32Hunk->GetOffsetTable(0)->GetOffsetCount());

      firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(3);

      Assert::AreEqual<ULO>(0x2fe, firstOffset);
      Assert::AreEqual<ULO>(0x3a, lastOffset);
    }

    TEST_METHOD(Parse_FastFileSystem13_FileReadsOK)
    {
      CreateInstanceWithFile(R"(testdata\fellow\hardfile\hunks\FastFileSystem13)");

      bool result = _instance->Parse();

      Assert::IsTrue(result);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetFirstLoadHunk());
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetLastLoadHunk());
      Assert::AreEqual<size_t>(1, _fileImage.GetHeader()->GetHunkSizeCount());
      Assert::AreEqual<ULO>(0xbed, _fileImage.GetHeader()->GetHunkSize(0).SizeInLongwords);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetHunkSize(0).MemoryFlags);

      Assert::AreEqual<size_t>(1, _fileImage.GetInitialHunkCount());
      Assert::AreEqual<size_t>(0, _fileImage.GetAdditionalHunkCount());

      // Code hunk
      Assert::AreEqual<ULO>(CodeHunkID, _fileImage.GetInitialHunk(0)->GetID());
      Assert::AreEqual<ULO>(0xbed * 4, _fileImage.GetInitialHunk(0)->GetAllocateSizeInBytes());
      Assert::AreEqual<ULO>(0xbed * 4, _fileImage.GetInitialHunk(0)->GetContentSizeInBytes());

      UBY firstByte = _fileImage.GetInitialHunk(0)->GetContent()[0];
      UBY middleByte = _fileImage.GetInitialHunk(0)->GetContent()[0x2000];
      UBY lastByte = _fileImage.GetInitialHunk(0)->GetContent()[0xbed * 4 - 1];
      Assert::AreEqual<UBY>(0x60, firstByte);
      Assert::AreEqual<UBY>(0x67, middleByte);
      Assert::AreEqual<UBY>(0x00, lastByte);
    }

    TEST_METHOD(Parse_FastFileSystem31_FileReadsOK)
    {
      CreateInstanceWithFile(R"(testdata\fellow\hardfile\hunks\FastFileSystem31)");

      bool result = _instance->Parse();

      Assert::IsTrue(result);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetFirstLoadHunk());
      Assert::AreEqual<ULO>(1, _fileImage.GetHeader()->GetLastLoadHunk());
      Assert::AreEqual<size_t>(2, _fileImage.GetHeader()->GetHunkSizeCount());
      Assert::AreEqual<ULO>(0x17ed, _fileImage.GetHeader()->GetHunkSize(0).SizeInLongwords);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetHunkSize(0).MemoryFlags);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetHunkSize(1).SizeInLongwords);
      Assert::AreEqual<ULO>(0, _fileImage.GetHeader()->GetHunkSize(1).MemoryFlags);

      Assert::AreEqual<size_t>(2, _fileImage.GetInitialHunkCount());
      Assert::AreEqual<size_t>(1, _fileImage.GetAdditionalHunkCount());

      // Code hunk
      Assert::AreEqual<ULO>(CodeHunkID, _fileImage.GetInitialHunk(0)->GetID());
      Assert::AreEqual<ULO>(0x17ed * 4, _fileImage.GetInitialHunk(0)->GetAllocateSizeInBytes());
      Assert::AreEqual<ULO>(0x17ed * 4, _fileImage.GetInitialHunk(0)->GetContentSizeInBytes());

      UBY firstByte = _fileImage.GetInitialHunk(0)->GetContent()[0];
      UBY middleByte = _fileImage.GetInitialHunk(0)->GetContent()[0x2000];
      UBY lastByte = _fileImage.GetInitialHunk(0)->GetContent()[0x17ed * 4 - 1];
      Assert::AreEqual<UBY>(0x72, firstByte);
      Assert::AreEqual<UBY>(0x4c, middleByte);
      Assert::AreEqual<UBY>(0x75, lastByte);

      // Data hunk
      Assert::AreEqual<ULO>(DataHunkID, _fileImage.GetInitialHunk(1)->GetID());
      Assert::AreEqual<ULO>(0, _fileImage.GetInitialHunk(1)->GetAllocateSizeInBytes());
      Assert::AreEqual<ULO>(0, _fileImage.GetInitialHunk(1)->GetContentSizeInBytes());

      // First reloc 32 hunk
      Assert::AreEqual<ULO>(Reloc32HunkID, _fileImage.GetAdditionalHunk(0)->GetID());
      Reloc32Hunk *reloc32Hunk = dynamic_cast<Reloc32Hunk *>(_fileImage.GetAdditionalHunk(0));

      Assert::AreEqual<size_t>(1, reloc32Hunk->GetOffsetTableCount());
      Assert::AreEqual<ULO>(0, reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex());
      Assert::AreEqual<size_t>(5, reloc32Hunk->GetOffsetTable(0)->GetOffsetCount());

      ULO firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      ULO lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(4);

      Assert::AreEqual<ULO>(0x28, firstOffset);
      Assert::AreEqual<ULO>(0x3c, lastOffset);
    }
  };
}
