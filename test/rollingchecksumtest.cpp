#include <cstdio>

#include "gtest/gtest.h"

#include "rollingchecksum.h"

using namespace vfs;

TEST(RollingChecksumTest, Update) {

  size_t size = 1024;
  const unsigned char* buffer = static_cast<const unsigned char*>(malloc(size));

  RollingChecksum checksum;
  checksum.Update(buffer, size);
  uint32_t sum = checksum.Get();
  printf("Checksum %lu\n", sum);
}