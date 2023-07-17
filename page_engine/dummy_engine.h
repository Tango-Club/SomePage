#ifndef PAGE_ENGINE_DUMMY_ENGINE_H_
#define PAGE_ENGINE_DUMMY_ENGINE_H_
#include "page_engine.h"
#include <cstdint>
#include <vector>

/*
 * Dummy sample of page engine
 */

class DummyEngine : public PageEngine {
private:
  std::string _path;
  const size_t page_size{16384};

public:
  static RetCode Open(const std::string &path, PageEngine **eptr);

  explicit DummyEngine(const std::string &path);

  ~DummyEngine() override;

  RetCode pageWrite(uint32_t page_no, const void *buf) override;

  RetCode pageWriteDirect(uint32_t page_no, const void *buf);

  RetCode pageRead(uint32_t page_no, void *buf) override;

  RetCode pageReadDirect(uint32_t page_no, void *buf);

  std::string page_no_to_path(uint32_t page_no);
};

#endif // PAGE_ENGINE_DUMMY_ENGINE_H_
