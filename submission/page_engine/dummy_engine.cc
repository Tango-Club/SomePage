#include "dummy_engine.h"
#include "common.h"

#include <cassert>
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

DummyEngine::DummyEngine(const std::string &path)
    : _path(path) {
  mkdir(_path.c_str(), O_RDWR | O_CREAT);
  std::string data_file = pathJoin(_path, DATA_FILE);
  _fd_data = open(data_file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (_fd_data == -1)	printf("Message : %s\n", strerror(errno));
  assert(_fd_data >= 0);
  std::string dict_file = pathJoin(_path, DICT_FILE);
  _fd_dict = open(dict_file.c_str(), O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
  if (_fd_dict == -1)	printf("Message : %s\n", strerror(errno));
  assert(_fd_dict >= 0);
}

DummyEngine::~DummyEngine() {
  if (dict_loaded.load()) {
    if (dictBuffer != nullptr) {
      free(dictBuffer);
    }
  }
  if (_fd_data >= 0) {
    close(_fd_data);
  }
  if (_fd_dict >= 0) {
    close(_fd_data);
  }
  // unlink(pathJoin(_path, DICT_FILE).c_str());
}

RetCode DummyEngine::pageWrite(uint32_t page_no, const void *buf) {
  if (dict_loaded.load()) {
    return pageWriteWithDic(page_no, buf);
  }
  if (dict_page_num.load() < MAX_DICT_PAGE_NUM) {
    write(_fd_dict, buf, PAGE_SIZE);
    if (++dict_page_num == MAX_DICT_PAGE_NUM) {
      dictBuffer = malloc(DICT_SIZE);
      read(_fd_dict, dictBuffer, DICT_SIZE);
      cdict = ZSTD_createCDict(dictBuffer, DICT_SIZE, COMPRESSION_LEVEL); 
      CHECK(cdict != NULL, "ZSTD_createCDict() failed!");
      ddict = ZSTD_createDDict(dictBuffer, DICT_SIZE);
      CHECK(ddict != NULL, "ZSTD_createDDict() failed!");
      dict_loaded.store(true);
      puts("load finished!");
    }
  }
  return pageWriteWithoutDic(page_no, buf);
}

RetCode DummyEngine::pageWriteWithDic(uint32_t page_no, const void *buf) {
  size_t const cBuffSize = ZSTD_compressBound(PAGE_SIZE);
  std::vector<char> dst;
  dst.resize(cBuffSize);

  ZSTD_CCtx* const cctx = ZSTD_createCCtx();
  CHECK(cctx != NULL, "ZSTD_createCCtx() failed!");
  size_t const cSize =
      ZSTD_compress_usingCDict(cctx, dst.data(), cBuffSize, buf, PAGE_SIZE, cdict);
  CHECK_ZSTD(cSize);

  bool isUsingDic = true;
  short real_size = cSize;
  assert(real_size + 3 <= PAGE_SIZE);

  pwrite(_fd_data, &isUsingDic, sizeof(isUsingDic), page_no * PAGE_SIZE);
  pwrite(_fd_data, &real_size, sizeof(real_size), page_no * PAGE_SIZE + 1);
  ssize_t nwrite = pwrite(_fd_data, dst.data(), real_size, page_no * PAGE_SIZE + 3);
  ZSTD_freeCCtx(cctx);
  return kSucc;
}

RetCode DummyEngine::pageWriteWithoutDic(uint32_t page_no, const void *buf) {
  size_t const cBuffSize = ZSTD_compressBound(PAGE_SIZE);
  std::vector<char> dst;
  dst.resize(cBuffSize);

  size_t const cSize =
      ZSTD_compress(dst.data(), cBuffSize, buf, PAGE_SIZE, COMPRESSION_LEVEL);

  bool isUsingDic = false;
  short real_size = cSize;
  assert(real_size + 3 <= PAGE_SIZE);

  pwrite(_fd_data, &isUsingDic, sizeof(isUsingDic), page_no * PAGE_SIZE);
  pwrite(_fd_data, &real_size, sizeof(real_size), page_no * PAGE_SIZE + 1);
  ssize_t nwrite = pwrite(_fd_data, dst.data(), real_size, page_no * PAGE_SIZE + 3);
  return kSucc;
}

RetCode DummyEngine::pageRead(uint32_t page_no, void *buf) {
  bool isUsingDic = false;
  pread(_fd_data, &isUsingDic, sizeof(isUsingDic), page_no * PAGE_SIZE);

  short real_size = 0;
  pread(_fd_data, &real_size, sizeof(real_size), page_no * PAGE_SIZE + 1);

  std::vector<char> dst;
  dst.resize(real_size);

  ssize_t nwrite = pread(_fd_data, dst.data(), real_size, page_no * PAGE_SIZE + 3);
  size_t cSize = 0;
  if (isUsingDic) {
    ZSTD_DCtx* const dctx = ZSTD_createDCtx();
    CHECK(dctx != NULL, "ZSTD_createDCtx() failed!");
    cSize = ZSTD_decompress_usingDDict(dctx, buf, PAGE_SIZE, dst.data(), dst.size(), ddict);
    CHECK_ZSTD(cSize);
    ZSTD_freeDCtx(dctx);
  } else {
    cSize = ZSTD_decompress(buf, PAGE_SIZE, dst.data(), dst.size());
  }
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