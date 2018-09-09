#include <memory>
#include "CppUnitTest.h"

#include "fellow/hardfile/hunks/HunkFactory.h"
#include "fellow/hardfile/hunks/HunkID.h"
#include "fellow/hardfile/hunks/CodeHunk.h"
#include "fellow/hardfile/hunks/DataHunk.h"
#include "fellow/hardfile/hunks/BSSHunk.h"
#include "fellow/hardfile/hunks/Reloc32Hunk.h"
#include "fellow/hardfile/hunks/EndHunk.h"
#include "test/framework/TestBootstrap.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace fellow::hardfile::hunks;

namespace test::fellow::hardfile::hunks
{
  TEST_CLASS(HunkFactoryTest)
  {
    TEST_METHOD_INITIALIZE(TestInitialize)
    {
      InitializeTestframework();
    }

    TEST_METHOD_CLEANUP(TestCleanup)
    {
      ShutdownTestframework();
    }

    TEST_METHOD(CreateInitialHunk_CodeHunk_ReturnsCodeHunkWithAllocateSizeSet)
    {
      unique_ptr<CodeHunk> hunk(dynamic_cast<CodeHunk*>(HunkFactory::CreateInitialHunk(CodeHunkID, 100)));

      Assert::IsNotNull(hunk.get());
      Assert::AreEqual<ULO>(100, hunk->GetAllocateSizeInLongwords());
    }

    TEST_METHOD(CreateInitialHunk_DataHunk_ReturnsDataHunkWithAllocateSizeSet)
    {
      unique_ptr<DataHunk> hunk(dynamic_cast<DataHunk*>(HunkFactory::CreateInitialHunk(DataHunkID, 100)));

      Assert::IsNotNull(hunk.get());
      Assert::AreEqual<ULO>(100, hunk->GetAllocateSizeInLongwords());
    }

    TEST_METHOD(CreateInitialHunk_BSSHunk_ReturnsBSSHunkWithAllocateSizeSet)
    {
      unique_ptr<BSSHunk> hunk(dynamic_cast<BSSHunk*>(HunkFactory::CreateInitialHunk(BSSHunkID, 100)));

      Assert::IsNotNull(hunk.get());
      Assert::AreEqual<ULO>(100, hunk->GetAllocateSizeInLongwords());
    }

    TEST_METHOD(CreateInitialHunk_UnknownHunk_ReturnsNullptr)
    {
      unique_ptr<InitialHunk> hunk(HunkFactory::CreateInitialHunk(12345678, 100));

      Assert::IsNull(hunk.get());
    }

    TEST_METHOD(CreateAdditionalHunk_Reloc32Hunk_ReturnsReloc32Hunk)
    {
      unique_ptr<Reloc32Hunk> hunk(dynamic_cast<Reloc32Hunk*>(HunkFactory::CreateAdditionalHunk(Reloc32HunkID, 0)));

      Assert::IsNotNull(hunk.get());
    }

    TEST_METHOD(CreateAdditionalHunk_EndHunk_ReturnsEndHunk)
    {
      unique_ptr<EndHunk> hunk(dynamic_cast<EndHunk*>(HunkFactory::CreateAdditionalHunk(EndHunkID, 0)));

      Assert::IsNotNull(hunk.get());
    }

    TEST_METHOD(CreateAdditionalHunk_UnknownHunkID_ReturnsNullptr)
    {
      unique_ptr<AdditionalHunk> hunk(HunkFactory::CreateAdditionalHunk(234567, 0));

      Assert::IsNull(hunk.get());
    }
  };
}
