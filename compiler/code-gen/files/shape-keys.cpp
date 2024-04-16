// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/shape-keys.h"

#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/naming.h"
#include "compiler/compiler-core.h"

std::string ShapeKeys::get_function_name() noexcept {
  return "init_shape_demangler";
}

void ShapeKeys::compile(CodeGenerator &W) const {
  W << OpenFile{"_shape_keys.cpp"};
  W << ExternInclude{G->settings().runtime_headers.get()};

  FunctionSignatureGenerator{W} << "void " << get_function_name() << "()" << BEGIN;
  W << "std::unordered_map<std::int64_t, std::string_view> shape_keys_storage{" << NL;

  for (const auto &[hash, key] : shape_keys_storage_) {
    W << Indent{2} << "{" << hash << ", \"" << key.data() << "\"}," << NL << Indent{-2};
  }

  W << "};" << NL << NL;

  W << "vk::singleton<ShapeKeyDemangle>::get().init(std::move(shape_keys_storage));" << NL;

  W << END << NL << NL;

  W << CloseFile{};
}
