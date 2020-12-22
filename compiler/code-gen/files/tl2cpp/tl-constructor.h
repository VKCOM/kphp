// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"

namespace tl2cpp {
struct TlConstructorDecl {
  const vk::tlo_parsing::combinator *constructor;

  explicit TlConstructorDecl(const vk::tlo_parsing::combinator *constructor) :
    constructor(constructor) {};

  static bool does_tl_constructor_need_typed_fetch_store(const vk::tlo_parsing::combinator *c) {
    return !get_all_php_classes_of_tl_constructor(c).empty();
  }

  static std::string get_optional_args_for_decl(const vk::tlo_parsing::combinator *c);

  void compile(CodeGenerator &W) const;
};

struct TlConstructorDef {
  const vk::tlo_parsing::combinator *constructor;

  explicit TlConstructorDef(const vk::tlo_parsing::combinator *constructor) :
    constructor(constructor) {};

  void compile(CodeGenerator &W) const;
};
}
