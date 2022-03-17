#include <cstdio>
#include <fstream>

#include "gtest/gtest.h"

#include "leveldbfilesystem.h"

static const char* DB_PATH = "./tmp";

using namespace vfs;

static bool file_exists(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

class LevelDbFileSystemTest : public ::testing::Test {
 protected:
  void SetUp() override {
    fs = new LevelDbFileSystem(DB_PATH);
  }

  void TearDown() override {
    printf("Clear all data\n");
    fs->Clear();
    delete fs;
  }

  LevelDbFileSystem* fs = nullptr;
};

TEST_F(LevelDbFileSystemTest, CreateObject) {
  ABObject* obj = new ABObject();
  obj->set_name("test");
  obj->set_handle(2);
  obj->set_dbpath("/test");

  bool ret = fs->CreateObject(obj, -1);
  EXPECT_TRUE(ret);

  delete obj;
}

TEST_F(LevelDbFileSystemTest, CreateObjects) {
  std::shared_ptr<ABObject> obj = std::make_shared<ABObject>();
  obj->set_name("test");
  obj->set_handle(2);
  obj->set_dbpath("/test");

  std::shared_ptr<ABObject> obj2 = std::make_shared<ABObject>();
  obj2->set_name("test2");
  obj2->set_handle(3);
  obj2->set_dbpath("/test2");

  bool ret = fs->CreateObjects({obj, obj2}, -1);
  EXPECT_TRUE(ret);
}

TEST_F(LevelDbFileSystemTest, ListDirectory) {
  // create object first
  ABObject* obj = new ABObject();
  obj->set_name("test");
  obj->set_handle(2);
  obj->set_dbpath("/test");
  bool ret = fs->CreateObject(obj, -1);
  EXPECT_TRUE(ret);

  IVirtualFileSystem::ObjectList list = fs->ListDirectory(-1);
  EXPECT_EQ(list.size(), 1);
  for (auto item : list) {
    printf("item: handle=%lld, name=%s, dbPath=%s\n", item->handle(), item->name().c_str(), item->dbpath().c_str());
  }
}

TEST_F(LevelDbFileSystemTest, RemoveObject) {
  // create object first
  ABObject* obj = new ABObject();
  obj->set_name("test");
  obj->set_handle(2);
  obj->set_dbpath("/test");
  bool ret = fs->CreateObject(obj, -1);
  EXPECT_TRUE(ret);

  // then delete it
  ret = fs->RemoveObject(obj);
  EXPECT_TRUE(ret);
}

TEST_F(LevelDbFileSystemTest, DeleteDirectory) {
  // create parent
  ABObject* parent = new ABObject();
  parent->set_name("test");
  parent->set_handle(2);
  bool ret = fs->CreateObject(parent, -1);
  EXPECT_TRUE(ret);

  // create children
  {
    std::shared_ptr<ABObject> obj = std::make_shared<ABObject>();
    obj->set_name("test");
    obj->set_handle(3);
    obj->set_parenthandle(parent->handle());

    std::shared_ptr<ABObject> obj2 = std::make_shared<ABObject>();
    obj2->set_name("test2");
    obj2->set_handle(4);
    obj->set_parenthandle(parent->handle());

    bool ret = fs->CreateObjects({obj, obj2}, parent->handle());
    EXPECT_TRUE(ret);
  }

  std::vector<std::string> files_removed;
  ret = fs->DeleteDirectory(parent, true, &files_removed);
  EXPECT_TRUE(ret);
  for (auto file : files_removed) {
    std::cout << "files_removed: " << file.c_str() << std::endl;
  }
  EXPECT_EQ(files_removed.size(), 3); // one self + two children

  IVirtualFileSystem::ObjectList list = fs->ListDirectory(parent->handle());
  EXPECT_EQ(list.size(), 0); // no more child left
}

TEST_F(LevelDbFileSystemTest, UpdateObject) {
  // create object first
  ABObject* obj = new ABObject();
  obj->set_name("test");
  obj->set_handle(2);
  obj->set_dbpath("/test");
  bool ret = fs->CreateObject(obj, -1);
  EXPECT_TRUE(ret);

  // then update it
  obj->set_name("newtest");
  ret = fs->UpdateObject(obj);
  EXPECT_TRUE(ret);

  // TODO: double check
}

TEST_F(LevelDbFileSystemTest, FindObject) {
  // create objects first
  ABObject* obj = new ABObject();
  obj->set_name("dir1");
  obj->set_handle(2);
  obj->set_dbpath("/dir1");
  bool ret = fs->CreateObject(obj, -1);
  EXPECT_TRUE(ret);

  obj = new ABObject();
  obj->set_name("dir2");
  obj->set_parenthandle(2);
  obj->set_handle(3);
  obj->set_dbpath("/dir1/dir2");
  ret = fs->CreateObject(obj, 2);
  EXPECT_TRUE(ret);

  obj = new ABObject();
  obj->set_name("file3");
  obj->set_flags(AB_FLAG_FILE_ITEM);
  obj->set_parenthandle(3);
  obj->set_handle(4);
  obj->set_dbpath("/dir1/dir2/file3");
  ret = fs->CreateObject(obj, 3);
  EXPECT_TRUE(ret);

  fs->Dump();

  // then find them
  auto found = fs->FindObject("dir1");
  EXPECT_TRUE(found);
  EXPECT_EQ(found->handle(), 2);

  found = fs->FindObject("dir1/dir2");
  EXPECT_TRUE(found);
  EXPECT_EQ(found->handle(), 3);

  found = fs->FindObject("dir1/dir2/file3");
  EXPECT_TRUE(found);
  EXPECT_EQ(found->handle(), 4);
  // TODO: double check
}

TEST_F(LevelDbFileSystemTest, RenameObject) {
  // create objects first
  ABObject* obj = new ABObject();
  obj->set_name("dir1");
  obj->set_handle(2);
  obj->set_dbpath("/dir1");
  bool ret = fs->CreateObject(obj, -1);
  EXPECT_TRUE(ret);

  fs->Dump();
  EXPECT_TRUE(fs->FindObject("dir1"));

  // then rename it
  std::cout << "fs->RenameObject()" << std::endl;
  ret = fs->RenameObject("dir1", "dir2");
  //EXPECT_TRUE(ret);

  //EXPECT_FALSE(fs->FindObject("dir1"));
  //auto new_obj = fs->FindObject("dir2");
  //EXPECT_EQ(new_obj->name(), "dir2");

  // TODO: between dirs
}

TEST_F(LevelDbFileSystemTest, Version) {
  int64_t version = fs->Version();
  std::cout << "old version: " << version << std::endl;

  // update
  version = version + 10000;
  bool ret = fs->UpdateVersion(version);
  EXPECT_TRUE(ret);

  int64_t new_version = fs->Version(true);
  EXPECT_EQ(new_version, version);
  std::cout << "new version: " << new_version << std::endl;
}

TEST_F(LevelDbFileSystemTest, Dump) {
  fs->Dump();
}

TEST_F(LevelDbFileSystemTest, Backup) {
  // create objects first
  ABObject* obj = new ABObject();
  obj->set_name("dir1");
  obj->set_handle(2);
  obj->set_dbpath("/dir1");
  bool ret = fs->CreateObject(obj, -1);
  EXPECT_TRUE(ret);

  obj = new ABObject();
  obj->set_name("dir2");
  obj->set_parenthandle(2);
  obj->set_handle(3);
  obj->set_dbpath("/dir1/dir2");
  ret = fs->CreateObject(obj, 2);
  EXPECT_TRUE(ret);

  obj = new ABObject();
  obj->set_name("file3");
  obj->set_flags(AB_FLAG_FILE_ITEM);
  obj->set_parenthandle(3);
  obj->set_handle(4);
  obj->set_dbpath("/dir1/dir2/file3");
  ret = fs->CreateObject(obj, 3);
  EXPECT_TRUE(ret);

  // backup to file
  std::string backup_filename = "backup_test.bin";
  fs->Backup(backup_filename);
  EXPECT_TRUE(file_exists(backup_filename.c_str()));

  // clear all data and reload then from backup
  fs->Dump();
  fs->Clear();
  fs->Dump();
  fs->LoadBackup(backup_filename);
  fs->Dump();

  auto found = fs->FindObject(obj->name(), obj->parenthandle());
  EXPECT_TRUE(found != nullptr);
  EXPECT_EQ(found->handle(), obj->handle());
  remove(backup_filename.c_str());
}
