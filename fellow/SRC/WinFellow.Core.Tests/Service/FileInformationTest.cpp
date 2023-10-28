#include "catch/catch_amalgamated.hpp"
#include "VirtualHost/Core.h"
#include "Service/FileInformation.h"

#include <string>
#include <fstream>

using namespace Service;
using namespace std;

class TestFileReference
{
private:
  void DeleteFile()
  {
    filesystem::permissions(FilePath, filesystem::perms::owner_write | filesystem::perms::others_write | filesystem::perms::group_write, filesystem::perm_options::add);
    filesystem::remove(FilePath);
  }

  void WriteToFile(const string &text)
  {
    ofstream ofs(FilePath);
    ofs << text;
    ofs.close();
  }

  void PrepareFile(bool isReadable, bool isWritable)
  {
    auto filestatus = filesystem::status(FilePath);
    if (filesystem::exists(filestatus))
    {
      DeleteFile();
    }

    WriteToFile("Test");

    if (isWritable)
    {
      filesystem::permissions(FilePath, filesystem::perms::owner_write | filesystem::perms::others_write | filesystem::perms::group_write, filesystem::perm_options::add);
    }
    else
    {
      filesystem::permissions(FilePath, filesystem::perms::owner_write | filesystem::perms::others_write | filesystem::perms::group_write, filesystem::perm_options::remove);
    }

    if (isReadable)
    {
      filesystem::permissions(FilePath, filesystem::perms::owner_read | filesystem::perms::others_read | filesystem::perms::group_read, filesystem::perm_options::add);
    }
    else
    {
      filesystem::permissions(FilePath, filesystem::perms::owner_read | filesystem::perms::others_read | filesystem::perms::group_read, filesystem::perm_options::remove);
    }
  }

public:
  filesystem::path FilePath;

  TestFileReference(const string &filename, bool isReadable, bool isWritable) : FilePath(filename)
  {
    PrepareFile(isReadable, isWritable);
  }

  TestFileReference(const wstring &filename, bool isReadable, bool isWritable) : FilePath(filename)
  {
    PrepareFile(isReadable, isWritable);
  }

  ~TestFileReference()
  {
    auto filestatus = filesystem::status(FilePath);
    if (filesystem::exists(filestatus))
    {
      DeleteFile();
    }
  }
};

TEST_CASE("Service::FileInformation.GetFileProperties() should return the read write status of a file")
{
  Core _core = Core();
  _core.FileInformation = new FileInformation();

  SECTION("IsWritable should be false")
  {
    string filename = "Readonly.test";
    TestFileReference file(filename, true, false);

    unique_ptr<FileProperties> fileinformation(_core.FileInformation->GetFileProperties(filename));

    REQUIRE(fileinformation->IsWritable == false);
  }

  SECTION("IsWritable should be true")
  {
    string filename = "Write.test";
    TestFileReference file(filename, true, true);

    unique_ptr<FileProperties> fileinformation(_core.FileInformation->GetFileProperties(filename));

    REQUIRE(fileinformation->IsWritable == true);
  }
}

TEST_CASE("Service::FileInformation.GetFileProperties() should return the size of a file")
{
  Core _core = Core();
  _core.FileInformation = new FileInformation();

  SECTION("Size should be 4 bytes")
  {
    string filename = "Size.test";
    TestFileReference file(filename, true, true);

    unique_ptr<FileProperties> fileinformation(_core.FileInformation->GetFileProperties(filename));

    REQUIRE(fileinformation->Size == 4);
  }
}

TEST_CASE("Service::FileInformation.GetFileProperties() should return the file type")
{
  Core _core = Core();
  _core.FileInformation = new FileInformation();

  SECTION("Type should be File")
  {
    string filename = "FileType.test";
    TestFileReference file(filename, true, true);

    unique_ptr<FileProperties> fileinformation(_core.FileInformation->GetFileProperties(filename));

    REQUIRE(fileinformation->Type == FileType::File);
  }
}

TEST_CASE("Service::FileInformation.GetFileProperties() should return the filename")
{
  Core _core = Core();
  _core.FileInformation = new FileInformation();

  SECTION("Path should be returned")
  {
    string filename = "Name.test";
    TestFileReference file(filename, true, true);

    unique_ptr<FileProperties> fileinformation(_core.FileInformation->GetFileProperties(filename));

    REQUIRE(fileinformation->Name == filename);
  }
}

TEST_CASE("Service::FileInformation.GetFileProperties() should return nullptr for non-existing file")
{
  Core _core = Core();
  _core.FileInformation = new FileInformation();

  SECTION("Should return nullptr for non-existant file")
  {
    string filename = "DoesNotExist.test";

    unique_ptr<FileProperties> fileinformation(_core.FileInformation->GetFileProperties(filename));

    REQUIRE(fileinformation == nullptr);
  }
}

TEST_CASE("Service::FileInformation.GetFilePropertiesW() should return the filename")
{
  Core _core = Core();
  _core.FileInformation = new FileInformation();

  SECTION("Path should be returned")
  {
    wstring filename = L"😊😊😂😂 unicode filename 🤣🤣😁😁.test";
    TestFileReference file(filename, true, false);

    unique_ptr<FilePropertiesW> fileinformation(_core.FileInformation->GetFilePropertiesW(filename));
    REQUIRE(fileinformation->Name == filename);
  }
}
