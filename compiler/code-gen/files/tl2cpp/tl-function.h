// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"

namespace tl2cpp {
struct TlFunctionDecl {
  const vk::tl::combinator *f;

  explicit TlFunctionDecl(const vk::tl::combinator *f) :
    f(f) {}

  static bool does_tl_function_need_typed_fetch_store(const vk::tl::combinator *f) {
    return !!get_php_class_of_tl_function(f);
  }

  static void check_kphp_function(const vk::tl::combinator *f) {
    for (const auto &arg : f->args) {
      kphp_error(!arg->is_forwarded_function(), fmt_format("In TL function {}: Exclamation is not allowed in @kphp TL functions", f->name));
    }
  }

  void compile(CodeGenerator &W) const;
};

class TlFunctionDef {
  const vk::tl::combinator *f;

public:
  explicit TlFunctionDef(const vk::tl::combinator *f) :
    f(f) {}

  void compile(CodeGenerator &W) const;
};
}
