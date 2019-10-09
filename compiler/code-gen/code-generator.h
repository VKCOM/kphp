#pragma once
#include <cinttypes>
#include <memory>
#include <string>
#include <vector>

#include "compiler/code-gen/writer-data.h"
#include "compiler/code-gen/writer.h"
#include "compiler/data/data_ptr.h"
#include "compiler/stage.h"
#include "common/wrappers/fmt_format.h"

struct CGContext {
  std::vector<std::string> catch_labels;
  std::vector<int> catch_label_used;
  FunctionPtr parent_func;
  bool resumable_flag{false};
  bool namespace_opened{false};
  int inside_macro{0};
};

class CodeGenerator {
private:
  std::unique_ptr<Writer> writer;
  DataStream<WriterData> &os;
  CGContext context;
public:

  CodeGenerator(DataStream<WriterData> &os) :
    writer(nullptr),
    os(os),
    context() {
  }

  CodeGenerator(const CodeGenerator &from) :
    writer(nullptr),
    os(from.os),
    context(from.context) {
  }

  CodeGenerator &operator=(const CodeGenerator &) = delete;

  void create_writer() {
    assert (writer == nullptr);
    writer = std::make_unique<Writer>(os);
  }

  void clear_writer() {
    assert (writer != nullptr);
    writer = nullptr;
  }

  Writer &get_writer() {
    assert (writer != nullptr);
    return *writer;
  }

  CGContext &get_context() {
    return context;
  }
};

template<class T, class = decltype(&T::compile)>
CodeGenerator &operator<<(CodeGenerator &c, const T &value) {
  value.compile(c);
  return c;
}

template<class T>
std::enable_if_t<std::is_constructible<vk::string_view, T>::value, CodeGenerator&> operator<<(CodeGenerator &c, const T &value) {
  c.get_writer().append(value);
  return c;
}

inline CodeGenerator &operator<<(CodeGenerator &c, int64_t value) {
  return c << std::to_string(value);
}

inline CodeGenerator &operator<<(CodeGenerator &c, uint64_t value) {
  return c << std::to_string(value);
}

inline CodeGenerator &operator<<(CodeGenerator &c, int32_t value) {
  return c << std::to_string(value);
}

inline CodeGenerator &operator<<(CodeGenerator &c, uint32_t value) {
  return c << std::to_string(value);
}

inline CodeGenerator& operator<<(CodeGenerator &c, char value) {
  c.get_writer().append(value);
  return c;
}

