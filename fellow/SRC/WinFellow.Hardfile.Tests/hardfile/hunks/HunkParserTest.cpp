#include "catch/catch_amalgamated.hpp"
#include <memory>
#include "hardfile/hunks/HunkParser.h"
#include "hardfile/hunks/Reloc32Hunk.h"
#include "TestBootstrap.h"

using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{

  long GetFilesystemFilesize(const char *filename)
  {
    FILE *F = fopen(filename, "rb");
    if (F == nullptr)
    {
      throw std::exception();
    }

    fseek(F, 0, SEEK_END);
    long size = ftell(F);
    fclose(F);

    return size;
  }

  unique_ptr<uint8_t[]> LoadFilesystemFile(const char *filename, long filesize)
  {
    FILE *F = fopen(filename, "rb");
    if (F == nullptr)
    {
      throw std::exception();
    }

    unique_ptr<uint8_t[]> rawData(new uint8_t[filesize]);
    fseek(F, 0, SEEK_SET);
    fread(rawData.get(), 1, filesize, F);
    fclose(F);

    return rawData;
  }

  unique_ptr<HunkParser> CreateInstanceWithFile(long filesize, uint8_t *rawData, FileImage &fileImage)
  {
    return std::make_unique<HunkParser>(rawData, filesize, fileImage);
  }

  TEST_CASE("Hardfile::Hunks::HunkParser.Parse() should parse filesystem binaries")
  {
    InitializeTestframework();

    SECTION("Parses SmartFilesystem")
    {
      const char *SmartFilesystemFilename = R"(testdata\hardfile\hunks\SmartFilesystem)";
      long filesize = GetFilesystemFilesize(SmartFilesystemFilename);
      unique_ptr<uint8_t[]> rawData = LoadFilesystemFile(SmartFilesystemFilename, filesize);
      FileImage fileImage;

      unique_ptr<HunkParser> instance(CreateInstanceWithFile(filesize, rawData.get(), fileImage));

      bool result = instance->Parse();

      REQUIRE(result == true);
      REQUIRE(fileImage.GetHeader()->GetFirstLoadHunk() == 0);
      REQUIRE(fileImage.GetHeader()->GetLastLoadHunk() == 0);
      REQUIRE(fileImage.GetHeader()->GetHunkSizeCount() == 1);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(0).SizeInLongwords == 0x5fbe);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(0).MemoryFlags == 0);

      REQUIRE(fileImage.GetInitialHunkCount() == 1);
      REQUIRE(fileImage.GetAdditionalHunkCount() == 1);

      REQUIRE(fileImage.GetInitialHunk(0)->GetID() == CodeHunkID);
      REQUIRE(fileImage.GetInitialHunk(0)->GetAllocateSizeInBytes() == 0x5fbe * 4);
      REQUIRE(fileImage.GetInitialHunk(0)->GetContentSizeInBytes() == 0x5fbe * 4);

      uint8_t firstByte = fileImage.GetInitialHunk(0)->GetContent()[0];
      uint8_t middleByte = fileImage.GetInitialHunk(0)->GetContent()[0xbf7d];
      uint8_t lastByte = fileImage.GetInitialHunk(0)->GetContent()[0x5fbe * 4 - 1];
      REQUIRE(firstByte == 0x60);
      REQUIRE(middleByte == 0x0c);
      REQUIRE(lastByte == 0);

      REQUIRE(fileImage.GetAdditionalHunk(0)->GetID() == Reloc32HunkID);
      Reloc32Hunk *reloc32Hunk = dynamic_cast<Reloc32Hunk *>(fileImage.GetAdditionalHunk(0));

      REQUIRE(reloc32Hunk->GetOffsetTableCount() == 1);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex() == 0);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetOffsetCount() == 0x28);

      uint32_t firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      uint32_t middleOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(14);
      uint32_t lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(39);

      REQUIRE(firstOffset == 0x86);
      REQUIRE(middleOffset == 0x7d1e);
      REQUIRE(lastOffset == 0x17db6);
    }

    SECTION("Parses ProfFileSystem 195")
    {
      const char *ProfFileSystem195Filename = R"(testdata\hardfile\hunks\ProfFileSystem_195)";
      long filesize = GetFilesystemFilesize(ProfFileSystem195Filename);
      unique_ptr<uint8_t[]> rawData = LoadFilesystemFile(ProfFileSystem195Filename, filesize);
      FileImage fileImage;

      unique_ptr<HunkParser> instance(CreateInstanceWithFile(filesize, rawData.get(), fileImage));

      bool result = instance->Parse();

      REQUIRE(result == true);
      REQUIRE(fileImage.GetHeader()->GetFirstLoadHunk() == 0);
      REQUIRE(fileImage.GetHeader()->GetLastLoadHunk() == 1);
      REQUIRE(fileImage.GetHeader()->GetHunkSizeCount() == 2);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(0).SizeInLongwords == 0x1b4f);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(0).MemoryFlags == 0);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(1).SizeInLongwords == 0x13d);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(1).MemoryFlags == 0);

      REQUIRE(fileImage.GetInitialHunkCount() == 2);
      REQUIRE(fileImage.GetAdditionalHunkCount() == 2);

      // Code hunk
      REQUIRE(fileImage.GetInitialHunk(0)->GetID() == CodeHunkID);
      REQUIRE(fileImage.GetInitialHunk(0)->GetAllocateSizeInBytes() == 0x1b4f * 4);
      REQUIRE(fileImage.GetInitialHunk(0)->GetContentSizeInBytes() == 0x1b4f * 4);

      uint8_t firstByte = fileImage.GetInitialHunk(0)->GetContent()[0];
      uint8_t middleByte = fileImage.GetInitialHunk(0)->GetContent()[0x2000];
      uint8_t lastByte = fileImage.GetInitialHunk(0)->GetContent()[0x1b4f * 4 - 1];
      REQUIRE(firstByte == 0x9e);
      REQUIRE(middleByte == 0x01);
      REQUIRE(lastByte == 0x75);

      // Data hunk
      REQUIRE(fileImage.GetInitialHunk(1)->GetID() == DataHunkID);
      REQUIRE(fileImage.GetInitialHunk(1)->GetAllocateSizeInBytes() == 0x13d * 4);
      REQUIRE(fileImage.GetInitialHunk(1)->GetContentSizeInBytes() == 0x13d * 4);

      firstByte = fileImage.GetInitialHunk(1)->GetContent()[0];
      middleByte = fileImage.GetInitialHunk(1)->GetContent()[0x200];
      lastByte = fileImage.GetInitialHunk(1)->GetContent()[0x13d * 4 - 1];
      REQUIRE(firstByte == 0);
      REQUIRE(middleByte == 0x25);
      REQUIRE(lastByte == 0);

      // First reloc 32 hunk
      REQUIRE(fileImage.GetAdditionalHunk(0)->GetID() == Reloc32HunkID);
      Reloc32Hunk *reloc32Hunk = dynamic_cast<Reloc32Hunk *>(fileImage.GetAdditionalHunk(0));

      REQUIRE(reloc32Hunk->GetOffsetTableCount() == 1);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex() == 1);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetOffsetCount() == 2);

      uint32_t firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      uint32_t lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(1);

      REQUIRE(firstOffset == 0x1dae);
      REQUIRE(lastOffset == 0xa);

      // Second reloc 32 hunk
      REQUIRE(fileImage.GetAdditionalHunk(1)->GetID() == Reloc32HunkID);
      reloc32Hunk = dynamic_cast<Reloc32Hunk *>(fileImage.GetAdditionalHunk(1));

      REQUIRE(reloc32Hunk->GetOffsetTableCount() == 1);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex() == 1);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetOffsetCount() == 4);

      firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(3);

      REQUIRE(firstOffset == 0x2fe);
      REQUIRE(lastOffset == 0x3a);
    }

    SECTION("Parses ProfFileSystem 195 68020")
    {
      const char *ProfFileSystem195_68020Filename = R"(testdata\hardfile\hunks\ProfFileSystem_195_68020+)";
      long filesize = GetFilesystemFilesize(ProfFileSystem195_68020Filename);
      unique_ptr<uint8_t[]> rawData = LoadFilesystemFile(ProfFileSystem195_68020Filename, filesize);
      FileImage fileImage;

      unique_ptr<HunkParser> instance(CreateInstanceWithFile(filesize, rawData.get(), fileImage));

      bool result = instance->Parse();

      REQUIRE(result == true);
      REQUIRE(fileImage.GetHeader()->GetFirstLoadHunk() == 0);
      REQUIRE(fileImage.GetHeader()->GetLastLoadHunk() == 1);
      REQUIRE(fileImage.GetHeader()->GetHunkSizeCount() == 2);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(0).SizeInLongwords == 0x1abd);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(0).MemoryFlags == 0);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(1).SizeInLongwords == 0x13d);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(1).MemoryFlags == 0);

      REQUIRE(fileImage.GetInitialHunkCount() == 2);
      REQUIRE(fileImage.GetAdditionalHunkCount() == 2);

      // Code hunk
      REQUIRE(fileImage.GetInitialHunk(0)->GetID() == CodeHunkID);
      REQUIRE(fileImage.GetInitialHunk(0)->GetAllocateSizeInBytes() == 0x1abd * 4);
      REQUIRE(fileImage.GetInitialHunk(0)->GetContentSizeInBytes() == 0x1abd * 4);

      uint8_t firstByte = fileImage.GetInitialHunk(0)->GetContent()[0];
      uint8_t middleByte = fileImage.GetInitialHunk(0)->GetContent()[0x2000];
      uint8_t lastByte = fileImage.GetInitialHunk(0)->GetContent()[0x1abd * 4 - 1];
      REQUIRE(firstByte == 0x9e);
      REQUIRE(middleByte == 0x30);
      REQUIRE(lastByte == 0x75);

      // Data hunk
      REQUIRE(fileImage.GetInitialHunk(1)->GetID() == DataHunkID);
      REQUIRE(fileImage.GetInitialHunk(1)->GetAllocateSizeInBytes() == 0x13d * 4);
      REQUIRE(fileImage.GetInitialHunk(1)->GetContentSizeInBytes() == 0x13d * 4);

      firstByte = fileImage.GetInitialHunk(1)->GetContent()[0];
      middleByte = fileImage.GetInitialHunk(1)->GetContent()[0x200];
      lastByte = fileImage.GetInitialHunk(1)->GetContent()[0x13d * 4 - 1];
      REQUIRE(firstByte == 0);
      REQUIRE(middleByte == 0x25);
      REQUIRE(lastByte == 0);

      // First reloc 32 hunk
      REQUIRE(fileImage.GetAdditionalHunk(0)->GetID() == Reloc32HunkID);
      Reloc32Hunk *reloc32Hunk = dynamic_cast<Reloc32Hunk *>(fileImage.GetAdditionalHunk(0));

      REQUIRE(reloc32Hunk->GetOffsetTableCount() == 1);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex() == 1);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetOffsetCount() == 2);

      uint32_t firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      uint32_t lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(1);

      REQUIRE(firstOffset == 0x1d7e);
      REQUIRE(lastOffset == 0xa);

      // Second reloc 32 hunk
      REQUIRE(fileImage.GetAdditionalHunk(1)->GetID() == Reloc32HunkID);
      reloc32Hunk = dynamic_cast<Reloc32Hunk *>(fileImage.GetAdditionalHunk(1));

      REQUIRE(reloc32Hunk->GetOffsetTableCount() == 1);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex() == 1);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetOffsetCount() == 4);

      firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(3);

      REQUIRE(firstOffset == 0x2fe);
      REQUIRE(lastOffset == 0x3a);
    }

    SECTION("Parses Fast file system 13")
    {
      const char *FastFileSystem13Filename = R"(testdata\hardfile\hunks\FastFileSystem13)";
      long filesize = GetFilesystemFilesize(FastFileSystem13Filename);
      unique_ptr<uint8_t[]> rawData = LoadFilesystemFile(FastFileSystem13Filename, filesize);
      FileImage fileImage;

      unique_ptr<HunkParser> instance(CreateInstanceWithFile(filesize, rawData.get(), fileImage));

      bool result = instance->Parse();

      REQUIRE(result == true);
      REQUIRE(fileImage.GetHeader()->GetFirstLoadHunk() == 0);
      REQUIRE(fileImage.GetHeader()->GetLastLoadHunk() == 0);
      REQUIRE(fileImage.GetHeader()->GetHunkSizeCount() == 1);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(0).SizeInLongwords == 0xbed);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(0).MemoryFlags == 0);

      REQUIRE(fileImage.GetInitialHunkCount() == 1);
      REQUIRE(fileImage.GetAdditionalHunkCount() == 0);

      // Code hunk
      REQUIRE(fileImage.GetInitialHunk(0)->GetID() == CodeHunkID);
      REQUIRE(fileImage.GetInitialHunk(0)->GetAllocateSizeInBytes() == 0xbed * 4);
      REQUIRE(fileImage.GetInitialHunk(0)->GetContentSizeInBytes() == 0xbed * 4);

      uint8_t firstByte = fileImage.GetInitialHunk(0)->GetContent()[0];
      uint8_t middleByte = fileImage.GetInitialHunk(0)->GetContent()[0x2000];
      uint8_t lastByte = fileImage.GetInitialHunk(0)->GetContent()[0xbed * 4 - 1];
      REQUIRE(firstByte == 0x60);
      REQUIRE(middleByte == 0x67);
      REQUIRE(lastByte == 0);
    }

    SECTION("Parses Fast filesystem 31")
    {
      const char *FastFileSystem31Filename = R"(testdata\hardfile\hunks\FastFileSystem31)";
      long filesize = GetFilesystemFilesize(FastFileSystem31Filename);
      unique_ptr<uint8_t[]> rawData = LoadFilesystemFile(FastFileSystem31Filename, filesize);
      FileImage fileImage;

      unique_ptr<HunkParser> instance(CreateInstanceWithFile(filesize, rawData.get(), fileImage));

      bool result = instance->Parse();

      REQUIRE(result == true);
      REQUIRE(fileImage.GetHeader()->GetFirstLoadHunk() == 0);
      REQUIRE(fileImage.GetHeader()->GetLastLoadHunk() == 1);
      REQUIRE(fileImage.GetHeader()->GetHunkSizeCount() == 2);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(0).SizeInLongwords == 0x17ed);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(0).MemoryFlags == 0);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(1).SizeInLongwords == 0);
      REQUIRE(fileImage.GetHeader()->GetHunkSize(1).MemoryFlags == 0);

      REQUIRE(fileImage.GetInitialHunkCount() == 2);
      REQUIRE(fileImage.GetAdditionalHunkCount() == 1);

      // Code hunk
      REQUIRE(fileImage.GetInitialHunk(0)->GetID() == CodeHunkID);
      REQUIRE(fileImage.GetInitialHunk(0)->GetAllocateSizeInBytes() == 0x17ed * 4);
      REQUIRE(fileImage.GetInitialHunk(0)->GetContentSizeInBytes() == 0x17ed * 4);

      uint8_t firstByte = fileImage.GetInitialHunk(0)->GetContent()[0];
      uint8_t middleByte = fileImage.GetInitialHunk(0)->GetContent()[0x2000];
      uint8_t lastByte = fileImage.GetInitialHunk(0)->GetContent()[0x17ed * 4 - 1];
      REQUIRE(firstByte == 0x72);
      REQUIRE(middleByte == 0x4c);
      REQUIRE(lastByte == 0x75);

      // Data hunk
      REQUIRE(fileImage.GetInitialHunk(1)->GetID() == DataHunkID);
      REQUIRE(fileImage.GetInitialHunk(1)->GetAllocateSizeInBytes() == 0);
      REQUIRE(fileImage.GetInitialHunk(1)->GetContentSizeInBytes() == 0);

      // First reloc 32 hunk
      REQUIRE(fileImage.GetAdditionalHunk(0)->GetID() == Reloc32HunkID);
      Reloc32Hunk *reloc32Hunk = dynamic_cast<Reloc32Hunk *>(fileImage.GetAdditionalHunk(0));

      REQUIRE(reloc32Hunk->GetOffsetTableCount() == 1);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetRelatedHunkIndex() == 0);
      REQUIRE(reloc32Hunk->GetOffsetTable(0)->GetOffsetCount() == 5);

      uint32_t firstOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(0);
      uint32_t lastOffset = reloc32Hunk->GetOffsetTable(0)->GetOffset(4);

      REQUIRE(firstOffset == 0x28);
      REQUIRE(lastOffset == 0x3c);
    }

    ShutdownTestframework();
  }
}
