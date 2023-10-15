#pragma once

#include "page_engine.h"
#include <cstdint>
#include <vector>

const size_t PAGE_SIZE = 16384;
const std::string DATA_FILE = "data.ibd";
const std::string DICT_FILE = "dict.ibd";
const std::string DIR_NAME = "storage/";
const int COMPRESSION_LEVEL = 0;

class DummyEngine : public PageEngine {
private:
  std::string _path;
  int _fd;

public:
  static RetCode Open(const std::string &path, PageEngine **eptr);

  explicit DummyEngine(const std::string &path);

  ~DummyEngine() override;

  RetCode pageWrite(uint32_t page_no, const void *buf) override;

  RetCode pageRead(uint32_t page_no, void *buf) override;

  std::string page_no_to_path(uint32_t page_no);
};
