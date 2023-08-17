#pragma once

#include "zstd/lib/zstd.h"
#include "page_engine.h"
#include <cstdint>
#include <vector>
#include <atomic>

const size_t PAGE_SIZE = 16384;
const std::string DATA_FILE = "data.ibd";
const std::string DICT_FILE = "dict.ibd";
const std::string DIR_NAME = "storage/";
const int COMPRESSION_LEVEL = 2;
const int MAX_DICT_PAGE_NUM = 500;
const size_t DICT_SIZE = PAGE_SIZE * MAX_DICT_PAGE_NUM;


class DummyEngine : public PageEngine {
private:
  int _fd_data;
  int _fd_dict;
  std::string _path;

  ZSTD_CCtx* cctx{nullptr};
  ZSTD_DCtx* dctx{nullptr};
  ZSTD_CDict* cdict{nullptr};
  ZSTD_DDict* ddict{nullptr};

  void* dictBuffer{nullptr};
  std::atomic<int> dict_page_num{0};
  std::atomic<bool> dict_loaded{false};

public:
  static RetCode Open(const std::string &path, PageEngine **eptr);

  explicit DummyEngine(const std::string &path);

  ~DummyEngine() override;

  RetCode pageWrite(uint32_t page_no, const void *buf) override;
  RetCode pageWriteWithDic(uint32_t page_no, const void *buf);
  RetCode pageWriteWithoutDic(uint32_t page_no, const void *buf);

  RetCode pageRead(uint32_t page_no, void *buf) override;
  RetCode pageReadWithDic(uint32_t page_no, void *buf);
  RetCode pageReadWithoutDic(uint32_t page_no, void *buf);

  std::string page_no_to_path(uint32_t page_no);
};