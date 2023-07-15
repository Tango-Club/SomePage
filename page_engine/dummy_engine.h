// Copyright [2023] Alibaba Cloud All rights reserved
#ifndef PAGE_ENGINE_DUMMY_ENGINE_H_
#define PAGE_ENGINE_DUMMY_ENGINE_H_
#include "page_engine.h"

/*
 * Dummy sample of page engine
 */

class DummyEngine : public PageEngine {
private:
  int fd;
  const size_t page_size{16384};

public:
  static RetCode Open(const std::string &path, PageEngine **eptr);

  explicit DummyEngine(const std::string &path);

  ~DummyEngine() override;

  RetCode pageWrite(uint32_t page_no, const void *buf) override;

  RetCode pageRead(uint32_t page_no, void *buf) override;
};

#endif // PAGE_ENGINE_DUMMY_ENGINE_H_
