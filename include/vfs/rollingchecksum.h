#ifndef VFS_ROLLINGCHECKSUM_H
#define VFS_ROLLINGCHECKSUM_H

#include <cstdint>

namespace vfs {

class RollingChecksum {
 public:
  RollingChecksum() { Reset(); }

  void Reset() {
    count_ = 0;
    s1_ = 0;
    s2_ = 0;
  }

  void Update(const unsigned char* buffer, size_t size);

  void RollingIn(unsigned char in);
  void RollingIn(const unsigned char* buffer, size_t size);

  void RollingOut(const unsigned char* buffer, size_t size);
  void RollingOut(unsigned char out);

  uint32_t Get() {
      return ((uint32_t)s2_ << 16) | ((uint32_t)s1_ & 0xffff);
  }

 private:
  static const int CHAR_OFFSET = 31;
  size_t count_;
  uint_fast16_t s1_;
  uint_fast16_t s2_;
};

} // namespace

#endif  // VFS_ROLLINGCHECKSUM_H
