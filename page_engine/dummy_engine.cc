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
}

DummyEngine::~DummyEngine() {}

RetCode DummyEngine::pageWrite(uint32_t page_no, const void *buf) {
  size_t const cBuffSize = ZSTD_compressBound(page_size);
  std::vector<char> dst;
  dst.resize(cBuffSize);

  size_t const cSize = ZSTD_compress(dst.data(), cBuffSize, buf, page_size, 22);

  std::string data_file = page_no_to_path(page_no);

  int fd = open(data_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  ssize_t nwrite = pwrite(fd, dst.data(), cSize, 0);
  close(fd);

  return kSucc;
}

RetCode DummyEngine::pageWriteDirect(uint32_t page_no, const void *buf) {
  std::string data_file = page_no_to_path(page_no);

  int fd = open(data_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  ssize_t nwrite = pwrite(fd, buf, page_size, 0);
  close(fd);

  return kSucc;
}

RetCode DummyEngine::pageRead(uint32_t page_no, void *buf) {
  std::string data_file = page_no_to_path(page_no);
  size_t file_size = get_file_size(data_file);
  if (file_size == 0) {
    memset(buf, 0, page_size);
    return kSucc;
  }
  std::vector<char> dst;
  dst.resize(file_size);

  int fd = open(data_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  ssize_t nwrite = pread(fd, dst.data(), dst.size(), 0);
  close(fd);

  size_t const cSize = ZSTD_decompress(buf, page_size, dst.data(), dst.size());
  return kSucc;
}

RetCode DummyEngine::pageReadDirect(uint32_t page_no, void *buf) {
  std::string data_file = page_no_to_path(page_no);

  int fd = open(data_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  ssize_t nwrite = pread(fd, buf, page_size, 0);
  close(fd);

  return kSucc;
}

std::string DummyEngine::page_no_to_path(uint32_t page_no) {
  std::string dir_path = pathJoin(_path, std::to_string(page_no / bucket_size));
  mkdir(dir_path.c_str(), O_RDWR | O_CREAT);
  return pathJoin(dir_path, std::to_string(page_no % bucket_size));
}