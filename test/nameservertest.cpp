#include <cstdio>

#include "gtest/gtest.h"

#include "nameserver.h"
#include "leveldbfilesystem.h"

static const char* DB_PATH = "./tmp";

using namespace vfs;

class NameServerTest : public ::testing::Test {
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

/*
TEST_F(NameServerTest, Start) {
  NameServer server(fs, 8888);
  server.Start();
}
*/