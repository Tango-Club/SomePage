#include "dummy_engine.h"

#include "zstd/lib/zstd.h"
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

inline std::string pathJoin(const std::string &p1, const std::string &p2) {
  char sep = '/';
  std::string tmp = p1;

  if (p1[p1.length() - 1] != sep) {
    tmp += sep;
    return tmp + p2;
  } else {
    return p1 + p2;
  }
}

RetCode PageEngine::Open(const std::string &path, PageEngine **eptr) {
  return DummyEngine::Open(path, eptr);
}

RetCode DummyEngine::Open(const std::string &path, PageEngine **eptr) {
  DummyEngine *engine = new DummyEngine(path);
  *eptr = engine;
  return kSucc;
}

DummyEngine::DummyEngine(const std::string &path) : _path(path) {
  mkdir(_path.c_str(), O_RDWR | O_CREAT);
  std::string data_file = pathJoin(_path, DATA_FILE);
  _fd = open(data_file.c_str(), O_RDWR | O_CREAT, 0777);
  if (_fd < 0) {
    perror("open faile");
  }
}

DummyEngine::~DummyEngine() {
  if (_fd >= 0) {
    close(_fd);
  }
}

thread_local std::vector<char> write_dst;
RetCode DummyEngine::pageWrite(uint32_t page_no, const void *buf) {
  size_t const cBuffSize = ZSTD_compressBound(PAGE_SIZE);
  write_dst.resize(cBuffSize);

  size_t const cSize = ZSTD_compress(write_dst.data(), cBuffSize, buf,
                                     PAGE_SIZE, COMPRESSION_LEVEL);

  short real_size = cSize;
  pwrite(_fd, &real_size, sizeof(real_size), page_no * PAGE_SIZE);
  ssize_t nwrite =
      pwrite(_fd, write_dst.data(), real_size, page_no * PAGE_SIZE + 2);
  return kSucc;
}

thread_local std::vector<char> read_dst;
RetCode DummyEngine::pageRead(uint32_t page_no, void *buf) {
  short real_size = 0;
  pread(_fd, &real_size, sizeof(real_size), page_no * PAGE_SIZE);

  read_dst.resize(real_size);
  ssize_t nwrite =
      pread(_fd, read_dst.data(), real_size, page_no * PAGE_SIZE + 2);
  size_t const cSize =
      ZSTD_decompress(buf, PAGE_SIZE, read_dst.data(), read_dst.size());
  if (cSize != PAGE_SIZE) {
    return kIOError;
  }
  return kSucc;
}

/*
std::string DummyEngine::page_no_to_path(uint32_t page_no) {
  std::string dir_path = pathJoin(_path, std::to_string(page_no / bucket_size));
  mkdir(dir_path.c_str(), O_RDWR | O_CREAT);
  return pathJoin(dir_path, std::to_string(page_no % bucket_size));
}
*/