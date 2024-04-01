// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <string>

#include "compiler/code-gen/code-gen-root-cmd.h"

class CodeGenerator;

struct ShapeKeys : CodeGenRootCmd {
  explicit ShapeKeys(std::map<std::int64_t, std::string> shape_keys_storage) noexcept
    : shape_keys_storage_(std::move(shape_keys_storage)) {}

  void compile(CodeGenerator &W) const final;

  static std::string get_function_name() noexcept;
  static std::string get_function_declaration() noexcept;

private:
  const std::map<std::int64_t, std::string> shape_keys_storage_;
};
