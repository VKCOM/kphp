#pragma once

#include <string>
#include <vector>

#include "common/smart_ptrs/make_unique.h"
#include "common/type_traits/traits.h"

#include "compiler/code-gen/writer-data.h"
#include "compiler/code-gen/writer.h"
#include "compiler/data/data_ptr.h"

struct CGContext {
  std::vector<std::string> catch_labels;
  std::vector<int> catch_label_used;
  FunctionPtr parent_func;
  bool use_safe_integer_arithmetic{false};
  bool resumable_flag{false};
  bool namespace_opened{false};
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

  void use_safe_integer_arithmetic(bool flag = true) {
    context.use_safe_integer_arithmetic = flag;
  }

  void create_writer() {
    assert (writer == nullptr);
    writer = vk::make_unique<Writer>(os);
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
vk::enable_if_t<std::is_constructible<vk::string_view, T>::value, CodeGenerator&> operator<<(CodeGenerator &c, const T &value) {
  c.get_writer().append(value);
  return c;
}

inline CodeGenerator& operator<<(CodeGenerator &c, char value) {
  c.get_writer().append(value);
  return c;
}

