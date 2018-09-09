#include <memory>
#include "CppUnitTest.h"

#include "fellow/hardfile/rdb/RDBHandler.h"
#include "test/framework/TestBootstrap.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace fellow::hardfile::rdb;

namespace test::fellow::hardfile::rdb
{
  TEST_CLASS(RDBTest)
  {
    unique_ptr<RDB> _instance;
    const STR* BlankWithRDB_SFS = "testdata\\fellow\\hardfile\\rdb\\BlankWithRDB_SFS.hdf";
    const STR* BlankWithRDB_PFS = "testdata\\fellow\\hardfile\\rdb\\BlankWithRDB_PFS.hdf";
    const STR* BlankWithRDB_FFS13 = "testdata\\fellow\\hardfile\\rdb\\BlankWithRDB_FFS13.hdf";
    const STR* BlankWithRDB_FFS31 = "testdata\\fellow\\hardfile\\rdb\\BlankWithRDB_FFS31.hdf";
    const STR* BlankWithoutRDB_OFS = "testdata\\fellow\\hardfile\\rdb\\BlankWithoutRDB_OFS.hdf";
    const STR* ManyPartitionsWithRDB_ManyFS = "testdata\\fellow\\hardfile\\rdb\\ManyPartitionsWithRDB_ManyFS.hdf";

    void GetDriveInformation(const STR* filename)
    {
      _instance.release();
      FILE *F = nullptr;
      fopen_s(&F, filename, "rb");
      if (F != nullptr)
      {
        RDBFileReader fileReader(F);
        _instance.reset(RDBHandler::GetDriveInformation(fileReader));
        fclose(F);
      }
    }

    bool HasRigidDiskBlock(const STR* filename)
    {
      bool result = false;
      FILE *F = nullptr;
      fopen_s(&F, filename, "rb");
      if (F != nullptr)
      {
        RDBFileReader fileReader(F);
        result = RDBHandler::HasRigidDiskBlock(fileReader);
        fclose(F);
      }
      else
      {
        throw std::exception();
      }
      return result;
    }

    TEST_METHOD_INITIALIZE(TestInitialize)
    {
      InitializeTestframework();
    }

    TEST_METHOD_CLEANUP(TestCleanup)
    {
      ShutdownTestframework();
    }

    TEST_METHOD(HasRigidDiskBlock_HDFWithRDBAndSFS_ReturnsTrue)
    {
      bool result = HasRigidDiskBlock(BlankWithRDB_SFS);

      Assert::IsTrue(result);
    }

    TEST_METHOD(HasRigidDiskBlock_HDFWithoutRDB_ReturnsFalse)
    {
      bool result = HasRigidDiskBlock(BlankWithoutRDB_OFS);

      Assert::IsFalse(result);
    }

    TEST_METHOD(GetDriveInformation_HDFWithRDBAndPFS_HeaderValuesReadCorrectly)
    {
      GetDriveInformation(BlankWithRDB_PFS);

      Assert::AreEqual<string>("RDSK", _instance->ID);
      Assert::AreEqual<ULO>(0x40, _instance->SizeInLongs);
      Assert::AreEqual<LON>(0xc9cbe43b, _instance->CheckSum);
      Assert::AreEqual<ULO>(7, _instance->HostID);
      Assert::AreEqual<ULO>(512, _instance->BlockSize);
      Assert::AreEqual<ULO>(0x17, _instance->Flags);
      Assert::AreEqual<ULO>(0xffffffff, _instance->BadBlockList);
      Assert::AreEqual<ULO>(1, _instance->PartitionList);
      Assert::AreEqual<ULO>(2, _instance->FilesystemHeaderList);
      Assert::AreEqual<ULO>(0xffffffff, _instance->DriveInitCode);

      Assert::AreEqual<ULO>(0xa, _instance->Cylinders);
      Assert::AreEqual<ULO>(0x3f, _instance->SectorsPerTrack);
      Assert::AreEqual<ULO>(0x10, _instance->Heads);
      Assert::AreEqual<ULO>(1, _instance->Interleave);
      Assert::AreEqual<ULO>(0xa, _instance->ParkingZone);
      Assert::AreEqual<ULO>(0xa, _instance->WritePreComp);
      Assert::AreEqual<ULO>(0xa, _instance->ReducedWrite);
      Assert::AreEqual<ULO>(3, _instance->StepRate);

      Assert::AreEqual<ULO>(0, _instance->RDBBlockLow);
      Assert::AreEqual<ULO>(0x7df, _instance->RDBBlockHigh);
      Assert::AreEqual<ULO>(2, _instance->LowCylinder);
      Assert::AreEqual<ULO>(9, _instance->HighCylinder);
      Assert::AreEqual<ULO>(0x3f0, _instance->CylinderBlocks);
      Assert::AreEqual<ULO>(0, _instance->AutoParkSeconds);
      Assert::AreEqual<ULO>(0x3d, _instance->HighRDSKBlock);

      Assert::AreEqual<string>("FELLOW  ", _instance->DiskVendor);
      Assert::AreEqual<string>("BlankRDBPFS3    ", _instance->DiskProduct);
      Assert::AreEqual<string>("0.4 ", _instance->DiskRevision);
    }

    TEST_METHOD(GetDriveInformation_HDFWithRDBAndFFS13_PartitionListReadCorrectly)
    {
      GetDriveInformation(BlankWithRDB_FFS13);

      Assert::AreEqual<size_t>(1, _instance->Partitions.size());

      RDBPartition* partition = _instance->Partitions[0].get();

      Assert::AreEqual<string>("PART", partition->ID);
      Assert::AreEqual<ULO>(0x40, partition->SizeInLongs);
      Assert::AreEqual<ULO>(0x6ef5b3c0, partition->CheckSum);
      Assert::AreEqual<ULO>(7, partition->HostID);
      Assert::AreEqual<ULO>(0xffffffff, partition->Next);
      Assert::AreEqual<ULO>(1, partition->Flags);
      Assert::AreEqual<ULO>(0, partition->BadBlockList);
      Assert::AreEqual<ULO>(0, partition->DevFlags);
      Assert::AreEqual<char>(5, partition->DriveNameLength);
      Assert::AreEqual<string>("FFS13", partition->DriveName);

      // DOS Environment Vector
      Assert::AreEqual<ULO>(0x10, partition->SizeOfVector);
      Assert::AreEqual<ULO>(0x80, partition->SizeBlock);
      Assert::AreEqual<ULO>(0, partition->SecOrg);
      Assert::AreEqual<ULO>(0, partition->Interleave);
      Assert::AreEqual<ULO>(0x10, partition->Surfaces);
      Assert::AreEqual<ULO>(1, partition->SectorsPerBlock);
      Assert::AreEqual<ULO>(0x3f, partition->BlocksPerTrack);
      Assert::AreEqual<ULO>(2, partition->Reserved);
      Assert::AreEqual<ULO>(0, partition->PreAlloc);
      Assert::AreEqual<ULO>(0, partition->Interleave);

      Assert::AreEqual<ULO>(2, partition->LowCylinder);
      Assert::AreEqual<ULO>(3, partition->HighCylinder);
      Assert::AreEqual<ULO>(0x1e, partition->NumBuffer);
      Assert::AreEqual<ULO>(0, partition->BufMemType);
      Assert::AreEqual<ULO>(0x00ffffff, partition->MaxTransfer);
      Assert::AreEqual<ULO>(0x7ffffffe, partition->Mask);
      Assert::AreEqual<ULO>(0, partition->BootPri);
      Assert::AreEqual<ULO>(0x444f5303, partition->DOSType);
      Assert::AreEqual<ULO>(0, partition->Baud);
      Assert::AreEqual<ULO>(0, partition->Control);
      Assert::AreEqual<ULO>(0, partition->Bootblocks);

      Assert::IsTrue(partition->IsAutomountable());
      Assert::IsTrue(partition->IsBootable());
    }
  };
}
