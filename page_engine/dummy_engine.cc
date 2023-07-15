// Copyright [2023] Alibaba Cloud All rights reserved
#include "dummy_engine.h"
#include <cassert>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

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

DummyEngine::DummyEngine(const std::string &path) {
  std::string data_file = pathJoin(path, "data.ibd");
  fd = open(data_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  assert(fd >= 0);
}

DummyEngine::~DummyEngine() {
  if (fd >= 0) {
    close(fd);
  }
}

RetCode DummyEngine::pageWrite(uint32_t page_no, const void *buf) {
  ssize_t nwrite = pwrite(fd, buf, page_size, page_no * page_size);

  if (nwrite != page_size) {
    return kIOError;
  }

  return kSucc;
}

RetCode DummyEngine::pageRead(uint32_t page_no, void *buf) {
  ssize_t nwrite = pread(fd, buf, page_size, page_no * page_size);

  if (nwrite != page_size) {
    return kIOError;
  }

  return kSucc;
}
