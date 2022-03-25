#include "rollingchecksum.h"

using namespace vfs;

#define ADD1(buffer, i) \
  {                     \
    s1 += buffer[i];    \
    s2 += s1;           \
  }
#define ADD2(buffer, i) \
  ADD1(buffer, i);      \
  ADD1(buffer, i + 1);
#define ADD4(buffer, i) \
  ADD2(buffer, i);      \
  ADD2(buffer, i + 2);
#define ADD8(buffer, i) \
  ADD4(buffer, i);      \
  ADD4(buffer, i + 4);
#define ADD16(buffer) \
  ADD8(buffer, 0);    \
  ADD8(buffer, 8);

#define SUB1(buffer, i)                      \
  {                                          \
    s1 -= (buffer[i] + CHAR_OFFSET);         \
    s2 -= count * (buffer[i] + CHAR_OFFSET); \
    count--;                                 \
  }
#define SUB2(buffer, i) \
  SUB1(buffer, i);      \
  SUB1(buffer, i + 1);
#define SUB4(buffer, i) \
  SUB2(buffer, i);      \
  SUB2(buffer, i + 2);
#define SUB8(buffer, i) \
  SUB4(buffer, i);      \
  SUB4(buffer, i + 4);
#define SUB16(buffer) \
  SUB8(buffer, 0);    \
  SUB8(buffer, 8);


void RollingChecksum::Update(const unsigned char* buffer, size_t size) {
  size_t n = size;
  uint_fast16_t s1 = s1_;
  uint_fast16_t s2 = s2_;

  while (n >= 16) {
    ADD16(buffer);
    buffer += 16;
    n -= 16;
  }
  while (n != 0) {
    s1 += *buffer++;
    s2 += s1;
    n--;
  }
  s1 += size * CHAR_OFFSET;
  s2 += ((size * (size + 1)) / 2) * CHAR_OFFSET;
  count_ += size;
  s1_ = s1;
  s2_ = s2;
}

void RollingChecksum::RollingIn(const unsigned char in) {
  s1_ += in + CHAR_OFFSET;
  s2_ += s1_;
  count_++;
}

void RollingChecksum::RollingIn(const unsigned char* buffer,
                                size_t buffer_size) {
  Update(buffer, buffer_size);
}

void RollingChecksum::RollingOut(const unsigned char out) {
  s1_ -= out + CHAR_OFFSET;
  s2_ -= count_ * (out + CHAR_OFFSET);
  count_--;
}

void RollingChecksum::RollingOut(const unsigned char* buffer, size_t size) {
  size_t n = size;
  size_t count = count_;
  uint_fast16_t s1 = s1_;
  uint_fast16_t s2 = s2_;

  while (n >= 16) {
    SUB16(buffer);
    buffer += 16;
    n -= 16;
  }
  while (n != 0) {
    s1 -= *buffer + CHAR_OFFSET;
    s2 -= count * (*buffer + CHAR_OFFSET);
    buffer++;
    count--;
    n--;
  }
  s1_ = s1;
  s2_ = s2;
  count_ = count;
}