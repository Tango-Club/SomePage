#include <iostream>
#include <string>
#include "zstd/lib/zstd.h"

int main() {
  // 原始字符串
  std::string src = "Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.Hello, world! This is a test string for zstd compression.";
  printf("before compress string size : %d\n", src.length());
  // 字典文件名
  std::string dictName = "./dictionary";

  // 加载字典文件
  FILE* dictFile = fopen(dictName.c_str(), "rb");
  if (dictFile == NULL) {
    printf("Message : %s\n", stderr);
    std::cerr << "Cannot open dictionary file: " << dictName << std::endl;
    return 1;
  }
  // 获取字典文件大小
  fseek(dictFile, 0, SEEK_END);
  size_t const dictSize = ftell(dictFile);
  fseek(dictFile, 0, SEEK_SET);
  // 分配内存并读取字典文件内容
  void* const dictBuffer = malloc(dictSize);
  size_t const readSize = fread(dictBuffer, 1, dictSize, dictFile);
  if (readSize != dictSize) {
    std::cerr << "Error reading dictionary file: " << dictName << std::endl;
    return 2;
  }
  fclose(dictFile);

  ZSTD_CCtx* cctx{nullptr};
  ZSTD_DCtx* dctx{nullptr};
  cctx = ZSTD_createCCtx();
  dctx = ZSTD_createDCtx();

  // 创建压缩和解压用的字典对象
  ZSTD_CDict* const cdict = ZSTD_createCDict(dictBuffer, dictSize, 3); // 压缩级别为3
  ZSTD_DDict* const ddict = ZSTD_createDDict(dictBuffer, dictSize);

  ZSTD_CCtx_loadDictionary(cctx, dictBuffer, dictSize);
  ZSTD_DCtx_loadDictionary(dctx, dictBuffer, dictSize);

  // 获取原始字符串的指针和大小
  char const* srcPtr = src.c_str();
  size_t const srcSize = src.size();

  // 计算压缩后的最大大小
  size_t const dstCapacity = ZSTD_compressBound(srcSize);

  // 分配内存并压缩字符串
  void* const dstBuffer = malloc(dstCapacity);
  size_t const dstSize = ZSTD_compress_usingCDict(cctx, dstBuffer, dstCapacity, srcPtr, srcSize, cdict);

  // 检查是否有错误
  if (ZSTD_isError(dstSize)) {
    std::cerr << "Compression error: " << ZSTD_getErrorName(dstSize) << std::endl;
    return 3;
  }

  // 构造压缩后的字符串对象
  std::string dst(static_cast<char*>(dstBuffer), dstSize);

  // 输出压缩后的字符串大小
  std::cout << "Compressed string size: " << dst.size() << std::endl;

  // 分配内存并解压字符串
  void* const resBuffer = malloc(srcSize);
  size_t const resSize = ZSTD_decompress_usingDDict(dctx, resBuffer, srcSize, dst.c_str(), dst.size(), ddict);

  // 检查是否有错误
  if (ZSTD_isError(resSize)) {
    std::cerr << "Decompression error: " << ZSTD_getErrorName(resSize) << std::endl;
    return 4;
  }

  // 构造解压后的字符串对象
  std::string res(static_cast<char*>(resBuffer), resSize);

  // 输出解压后的字符串内容
  std::cout << "Decompressed string: " << res << std::endl;

  // 释放内存并销毁字典对象
  free(dictBuffer);
  free(dstBuffer);
  free(resBuffer);
  ZSTD_freeCDict(cdict);
  ZSTD_freeDDict(ddict);

  return 0;
}
