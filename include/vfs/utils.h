#ifndef VFS_UTILS_H
#define VFS_UTILS_H

#include <cstdint>
#include "ab.h"

// copy from: https://github.com/baidu/tera/blob/dbcd28af792d879d961bf9fc7eb60de81b437646/src/io/coding.h

inline void EncodeBigEndian32(char* buf, uint32_t value) {
  buf[0] = (value >> 24) & 0xff;
  buf[1] = (value >> 16) & 0xff;
  buf[2] = (value >> 8) & 0xff;
  buf[3] = value & 0xff;
}

inline uint32_t DecodeBigEndian32(const char* ptr) {
  return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[3]))) |
          (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 8) |
          (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 16) |
          (static_cast<uint32_t>(static_cast<unsigned char>(ptr[0])) << 24));
}

inline void EncodeBigEndian(char* buf, uint64_t value) {
  buf[0] = (value >> 56) & 0xff;
  buf[1] = (value >> 48) & 0xff;
  buf[2] = (value >> 40) & 0xff;
  buf[3] = (value >> 32) & 0xff;
  buf[4] = (value >> 24) & 0xff;
  buf[5] = (value >> 16) & 0xff;
  buf[6] = (value >> 8) & 0xff;
  buf[7] = value & 0xff;
}

inline uint64_t DecodeBigEndian(const char* ptr) {
  uint64_t lo = DecodeBigEndian32(ptr + 4);
  uint64_t hi = DecodeBigEndian32(ptr);
  return (hi << 32) | lo;
}

inline bool isDirectory(int32_t flags) {
  return !((flags & AB_FLAG_FILE_ITEM) == AB_FLAG_FILE_ITEM) // not a file item
         && !((flags & AB_FLAG_LINK_ITEM) == AB_FLAG_LINK_ITEM); // and not a link item
}

int compress_backup_file(const char* source_file, uint8_t file_version, int64_t db_version, const char* dest_file);
int decompress_backup_file(const char* source_file, const char* dest_file, uint8_t* file_version, int64_t* db_version);


#endif // VFS_UTILS_H
