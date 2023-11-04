#include "catch/catch_amalgamated.hpp"
#include <memory>
#include "hardfile/rdb/RDBHandler.h"
#include "TestBootstrap.h"

using namespace std;
using namespace fellow::hardfile::rdb;
using namespace Module::Hardfile;

namespace test::fellow::hardfile::rdb
{
  rdb_status CallHasRigidDiskBlock(const char *filename)
  {
    FILE *F = fopen(filename, "rb");
    if (F == nullptr)
    {
      throw std::exception();
    }

    RDBFileReader fileReader(F);
    rdb_status result = RDBHandler::HasRigidDiskBlock(fileReader);
    fclose(F);
    return result;
  }

  unique_ptr<RDB> CallGetDriveInformation(const char *filename)
  {
    FILE *F = fopen(filename, "rb");
    if (F == nullptr)
    {
      throw std::exception();
    }

    RDBFileReader fileReader(F);
    unique_ptr<RDB> rdb(RDBHandler::GetDriveInformation(fileReader));
    fclose(F);
    return rdb;
  }

  TEST_CASE("Hardfile::RDB::RDBHandler.HasRigidDiskBlock() should return RDB_FOUND for RDB hardfile with SFS filesystem")
  {
    InitializeTestframework();

    SECTION("Returns RDB_FOUND for RDB hardfile with SFS filesystem")
    {
      const char *BlankWithRDB_SFS = R"(testdata\hardfile\rdb\BlankWithRDB_SFS.hdf)";
      rdb_status result = CallHasRigidDiskBlock(BlankWithRDB_SFS);

      REQUIRE(result == rdb_status::RDB_FOUND);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::RDB::RDBHandler.HasRigidDiskBlock() should return RDB_NOT_FOUND for hardfile without RDB")
  {
    InitializeTestframework();

    SECTION("Returns RDB_NOT_FOUND for hardfile without RDB")
    {
      const char *BlankWithoutRDB_OFS = R"(testdata\hardfile\rdb\BlankWithoutRDB_OFS.hdf)";
      rdb_status result = CallHasRigidDiskBlock(BlankWithoutRDB_OFS);

      REQUIRE(result == rdb_status::RDB_NOT_FOUND);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::RDB::RDBHandler.HasRigidDiskBlock() should return RDB_FOUND_WITH_HEADER_CHECKSUM_ERROR for RDB hardfile with invalid checksum")
  {
    InitializeTestframework();

    SECTION("Returns RDB_FOUND_WITH_HEADER_CHECKSUM_ERROR for RDB hardfile with invalid checksum")
    {
      const char *RDBInvalidCheckSum = R"(testdata\hardfile\rdb\WithRDSKMarkerInvalidChecksum.hdf)";
      rdb_status result = CallHasRigidDiskBlock(RDBInvalidCheckSum);

      REQUIRE(result == rdb_status::RDB_FOUND_WITH_HEADER_CHECKSUM_ERROR);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::RDB::RDBHandler.HasRigidDiskBlock() should return RDB_FOUND_WITH_PARTITION_ERROR for RDB hardfile with invalid partition checksum")
  {
    InitializeTestframework();

    SECTION("Returns RDB_FOUND_WITH_PARTITION_ERROR for RDB hardfile with invalid partition checksum")
    {
      const char *RDBPartitionInvalidCheckSum = R"(testdata\hardfile\rdb\WithRDBInvalidPartitionCheckSum.hdf)";
      rdb_status result = CallHasRigidDiskBlock(RDBPartitionInvalidCheckSum);

      REQUIRE(result == rdb_status::RDB_FOUND_WITH_PARTITION_ERROR);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::RDB::RDBHandler.GetDriveInformation() should find error in RDB hardfile with invalid partition checksum")
  {
    InitializeTestframework();

    SECTION("Should find error in RDB hardfile with invalid partition checksum")
    {
      const char *RDBPartitionInvalidCheckSum = R"(testdata\hardfile\rdb\WithRDBInvalidPartitionCheckSum.hdf)";
      auto rdb = CallGetDriveInformation(RDBPartitionInvalidCheckSum);

      REQUIRE(rdb->HasPartitionErrors == true);
      REQUIRE(rdb->HasFileSystemHandlerErrors == false);
      REQUIRE(rdb->HasValidCheckSum == true);
      REQUIRE(rdb->Partitions.empty() == true);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::RDB::RDBHandler.GetDriveInformation() should find error in RDB hardfile with invalid filesystem header checksum")
  {
    InitializeTestframework();

    SECTION("Should find error in RDB hardfile with invalid filesystem header checksum")
    {
      const char *RDBFileSystemHeaderInvalidCheckSum = R"(testdata\hardfile\rdb\WithRDBInvalidFileSystemHeaderCheckSum.hdf)";
      auto rdb = CallGetDriveInformation(RDBFileSystemHeaderInvalidCheckSum);

      REQUIRE(rdb->HasFileSystemHandlerErrors == true);
      REQUIRE(rdb->FileSystemHeaders.empty() == true);
      REQUIRE(rdb->HasPartitionErrors == false);
      REQUIRE(rdb->HasValidCheckSum == true);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::RDB::RDBHandler.GetDriveInformation() should find error in RDB hardfile with invalid LSeg block checksum")
  {
    InitializeTestframework();

    SECTION("Should find error in RDB hardfile with invalid LSeg block checksum")
    {
      const char *RDBLSegBlockInvalidCheckSum = R"(testdata\hardfile\rdb\WithRDBInvalidLSegBlockCheckSum.hdf)";
      auto rdb = CallGetDriveInformation(RDBLSegBlockInvalidCheckSum);

      REQUIRE(rdb->HasFileSystemHandlerErrors == true);
      REQUIRE(rdb->FileSystemHeaders.empty() == true);
      REQUIRE(rdb->HasPartitionErrors == false);
      REQUIRE(rdb->HasValidCheckSum == true);
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::RDB::RDBHandler.GetDriveInformation() should read header correctly for RDB hardfile with PFS")
  {
    InitializeTestframework();

    SECTION("Should read header correctly for RDB hardfile with PFS")
    {
      const char *BlankWithRDB_PFS = R"(testdata\hardfile\rdb\BlankWithRDB_PFS.hdf)";
      auto rdb = CallGetDriveInformation(BlankWithRDB_PFS);

      REQUIRE(rdb->ID == "RDSK");
      REQUIRE(rdb->SizeInLongs == 0x40);
      REQUIRE(rdb->CheckSum == (int32_t)0xc9cbe43b);
      REQUIRE(rdb->HostID == 7);
      REQUIRE(rdb->BlockSize == 512);
      REQUIRE(rdb->Flags == 0x17);
      REQUIRE(rdb->BadBlockList == 0xffffffff);
      REQUIRE(rdb->PartitionList == 1);
      REQUIRE(rdb->FilesystemHeaderList == 2);
      REQUIRE(rdb->DriveInitCode == 0xffffffff);

      REQUIRE(rdb->Cylinders == 0xa);
      REQUIRE(rdb->SectorsPerTrack == 0x3f);
      REQUIRE(rdb->Heads == 0x10);
      REQUIRE(rdb->Interleave == 1);
      REQUIRE(rdb->ParkingZone == 0xa);
      REQUIRE(rdb->WritePreComp == 0xa);
      REQUIRE(rdb->ReducedWrite == 0xa);
      REQUIRE(rdb->StepRate == 3);

      REQUIRE(rdb->RDBBlockLow == 0);
      REQUIRE(rdb->RDBBlockHigh == 0x7df);
      REQUIRE(rdb->LowCylinder == 2);
      REQUIRE(rdb->HighCylinder == 9);
      REQUIRE(rdb->CylinderBlocks == 0x3f0);
      REQUIRE(rdb->AutoParkSeconds == 0);
      REQUIRE(rdb->HighRDSKBlock == 0x3d);

      REQUIRE(rdb->DiskVendor == "FELLOW  ");
      REQUIRE(rdb->DiskProduct == "BlankRDBPFS3    ");
      REQUIRE(rdb->DiskRevision == "0.4 ");
    }

    ShutdownTestframework();
  }

  TEST_CASE("Hardfile::RDB::RDBHandler.GetDriveInformation() should read partition list correctly for RDB hardfile with FFS13")
  {
    InitializeTestframework();

    SECTION("Should read partition list correctly for RDB hardfile with FFS13")
    {
      const char *BlankWithRDB_FFS13 = R"(testdata\hardfile\rdb\BlankWithRDB_FFS13.hdf)";
      auto rdb = CallGetDriveInformation(BlankWithRDB_FFS13);

      REQUIRE(rdb->Partitions.size() == 1);

      RDBPartition *partition = rdb->Partitions[0].get();

      REQUIRE(partition->ID == "PART");
      REQUIRE(partition->SizeInLongs == 0x40);
      REQUIRE(partition->CheckSum == 0x6ef5b3c0);
      REQUIRE(partition->HostID == 7);
      REQUIRE(partition->Next == 0xffffffff);
      REQUIRE(partition->Flags == 1);
      REQUIRE(partition->BadBlockList == 0);
      REQUIRE(partition->DevFlags == 0);
      REQUIRE(partition->DriveNameLength == 5);
      REQUIRE(partition->DriveName == "FFS13");

      // DOS Environment Vector
      REQUIRE(partition->SizeOfVector == 0x10);
      REQUIRE(partition->SizeBlock == 0x80);
      REQUIRE(partition->SecOrg == 0);
      REQUIRE(partition->Interleave == 0);
      REQUIRE(partition->Surfaces == 0x10);
      REQUIRE(partition->SectorsPerBlock == 1);
      REQUIRE(partition->BlocksPerTrack == 0x3f);
      REQUIRE(partition->Reserved == 2);
      REQUIRE(partition->PreAlloc == 0);

      REQUIRE(partition->LowCylinder == 2);
      REQUIRE(partition->HighCylinder == 3);
      REQUIRE(partition->NumBuffer == 0x1e);
      REQUIRE(partition->BufMemType == 0);
      REQUIRE(partition->MaxTransfer == 0x00ffffff);
      REQUIRE(partition->Mask == 0x7ffffffe);
      REQUIRE(partition->BootPri == 0);
      REQUIRE(partition->DOSType == 0x444f5303);
      REQUIRE(partition->Baud == 0);
      REQUIRE(partition->Control == 0);
      REQUIRE(partition->Bootblocks == 0);
      REQUIRE(partition->IsAutomountable() == true);
      REQUIRE(partition->IsBootable() == true);
    }

    ShutdownTestframework();
  }
}
