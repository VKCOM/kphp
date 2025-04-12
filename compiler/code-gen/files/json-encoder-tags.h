// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <set>
#include <string>

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/data/data_ptr.h"

class CodeGenerator;

struct JsonEncoderTags : CodeGenRootCmd {
  explicit JsonEncoderTags(std::set<ClassPtr> &&all_json_encoders) noexcept :
    all_json_encoders_(all_json_encoders) {}

  void compile(CodeGenerator &W) const final;

  static const std::string all_tags_file_;
  static std::string get_cppStructTag_name(const std::string &json_encoder) noexcept;

private:
  const std::set<ClassPtr> all_json_encoders_;
};
