#include "dummy_engine.h"
#include "zstd/lib/zstd.h"
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

const int bucket_size = 256;
const std::string dir_name = "storage/";

size_t get_file_size(std::string filename) {
  std::filesystem::path p{filename};
  if (!std::filesystem::exists(p)) {
    return 0;
  }
  return std::filesystem::file_size(p);
}

/*
 * Dummy sample of page engine
 */

RetCode PageEngine::Open(const std::string &path, PageEngine **eptr) {
  return DummyEngine::Open(path, eptr);
}

static std::string pathJoin(const std::string &p1, const std::string &p2) {
  char sep = '/';
  std::string tmp = p1;

#ifdef _WIN32
  sep = '\\';
#endif

  if (p1[p1.length() - 1] != sep) {
    tmp += sep;
    return tmp + p2;
  } else {
    return p1 + p2;
  }
}

RetCode DummyEngine::Open(const std::string &path, PageEngine **eptr) {
  DummyEngine *engine = new DummyEngine(path);
  *eptr = engine;
  return kSucc;
}

DummyEngine::DummyEngine(const std::string &path)
    : _path(pathJoin(path, dir_name)) {
  mkdir(_path.c_str(), O_RDWR | O_CREAT);
  std::string data_file = pathJoin(path, "data.ibd");
  _fd = open(data_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  assert(_fd >= 0);
}

DummyEngine::~DummyEngine() {
  if (_fd >= 0) {
    close(_fd);
  }
}

RetCode DummyEngine::pageWrite(uint32_t page_no, const void *buf) {
  size_t const cBuffSize = ZSTD_compressBound(page_size);
  std::vector<char> dst;
  dst.resize(cBuffSize);

  size_t const cSize = ZSTD_compress(dst.data(), cBuffSize, buf, page_size, 22);

  short real_size = cSize;
  assert(real_size + 2 <= page_size);
  pwrite(_fd, &real_size, sizeof(real_size), page_no * page_size);
  ssize_t nwrite = pwrite(_fd, dst.data(), real_size, page_no * page_size + 2);
  return kSucc;
}

RetCode DummyEngine::pageRead(uint32_t page_no, void *buf) {
  short real_size = 0;
  pread(_fd, &real_size, sizeof(real_size), page_no * page_size);
  std::vector<char> dst;
  dst.resize(real_size);
  ssize_t nwrite = pread(_fd, dst.data(), real_size, page_no * page_size + 2);
  size_t const cSize = ZSTD_decompress(buf, page_size, dst.data(), dst.size());
  if (cSize != page_size) {
    return kIOError;
  }
  return kSucc;
}

std::string DummyEngine::page_no_to_path(uint32_t page_no) {
  std::string dir_path = pathJoin(_path, std::to_string(page_no / bucket_size));
  mkdir(dir_path.c_str(), O_RDWR | O_CREAT);
  return pathJoin(dir_path, std::to_string(page_no % bucket_size));
}