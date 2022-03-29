#include "utils.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <iostream>

#include "zlib.h"

int decompress_backup_file(const char* source_file, const char* dest_file, int64_t* db_version) {
  FILE* source = fopen(source_file, "rb");
  fseek(source, 0, SEEK_END);
  long file_size = ftell(source);
  fseek(source, 0, SEEK_SET);
  std::cout << "source file " << source_file << " size: " << file_size << std::endl;

  // read file header
  char magic[4];
  fread(&magic, 1, 4, source);
  std::cout << "magic: " << magic << " " << std::endl;
  if (strcmp(magic, "VFS") != 0) {
    std::cout << "It's not a vfs backup file" << std::endl;
    return Z_DATA_ERROR;
  }
  uint8_t version;
  fread(&version, sizeof(uint8_t), 1, source);
  std::cout << "version: " << version << " " << std::endl;
  uint8_t compressed;
  fread(&compressed, sizeof(uint8_t), 1, source);
  std::cout << "compressed: " << compressed << " " << std::endl;
  (*db_version) = 0;
  fread(db_version, sizeof(int64_t), 1, source);
  std::cout << "db_version: " << db_version << " " << std::endl;

  std::string filename = dest_file;

  // decompress data and write then to dest file
  FILE* dest = fopen(filename.c_str(), "wb");
  const int CHUNK = 16384;
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit(&strm);
  if (ret != Z_OK) {
    std::cout << "deflateInit error" << std::endl;
    return ret;
  }

  /* decompress until deflate stream ends or end of file */
  do {
    strm.avail_in = fread(in, 1, CHUNK, source);
    if (ferror(source)) {
      (void)inflateEnd(&strm);
      std::cout << "fread error" << std::endl;
      return Z_ERRNO;
    }
    if (strm.avail_in == 0) break;
    strm.next_in = in;

    /* run inflate() on input until output buffer not full */
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = inflate(&strm, Z_NO_FLUSH);
      assert(ret != Z_STREAM_ERROR); /* state not clobbered */
      switch (ret) {
        case Z_NEED_DICT:
          ret = Z_DATA_ERROR; /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          (void)inflateEnd(&strm);
          std::cout << "inflate error" << std::endl;
          return Z_ERRNO;
      }
      have = CHUNK - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
        (void)inflateEnd(&strm);
        std::cout << "fwrite error" << std::endl;
        return Z_ERRNO;
      }
    } while (strm.avail_out == 0);

    /* done when inflate() says it's done */
  } while (ret != Z_STREAM_END);

  /* clean up and return */
  (void)inflateEnd(&strm);

  fclose(source);
  fclose(dest);

  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int compress_backup_file(const char* source_file, uint8_t file_version, int64_t db_version, const char* dest_file) {
  const int CHUNK = 16384;
  int ret, flush;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
  if (ret != Z_OK) {
    std::cout << "deflateInit error" << std::endl;
    return ret;
  }

  /* compress until end of file */
  FILE* source = fopen(source_file, "rb");
  fseek(source, 0, SEEK_END);
  long file_size = ftell(source);
  fseek(source, 0, SEEK_SET);
  std::cout << "source file size: " << file_size << std::endl;

  FILE* dest = fopen(dest_file, "wb");
  fwrite("VFS", strlen("VFS") + 1, 1, dest);
  fwrite((void*)&file_version, 1, sizeof(uint8_t), dest);
  uint8_t compressed = 1;
  fwrite((void*)&compressed, 1, sizeof(uint8_t), dest);
  fwrite((void*)&db_version, 1, sizeof(int64_t), dest);
  do {
    strm.avail_in = fread(in, 1, CHUNK, source);
    if (ferror(source)) {
      (void)deflateEnd(&strm);
      std::cout << "fread error" << std::endl;
      return Z_ERRNO;
    }
    flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
    strm.next_in = in;

    /* run deflate() on input until output buffer not full, finish
        compression if all of source has been read in */
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = deflate(&strm, flush);   /* no bad return value */
      assert(ret != Z_STREAM_ERROR); /* state not clobbered */
      have = CHUNK - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
        (void)deflateEnd(&strm);
        std::cout << "fwrite error" << std::endl;
        return Z_ERRNO;
      }

    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0); /* all input will be used */

    /* done when last data in file processed */
  } while (flush != Z_FINISH);
  assert(ret == Z_STREAM_END); /* stream will be complete */

  /* clean up and return */
  (void)deflateEnd(&strm);

  fclose(source);
  fclose(dest);
  return Z_OK;
}
