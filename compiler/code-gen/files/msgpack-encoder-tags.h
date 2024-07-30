// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <set>
#include <string>

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/data/data_ptr.h"

class CodeGenerator;

struct MsgPackEncoderTags : CodeGenRootCmd {
  explicit MsgPackEncoderTags(std::set<ClassPtr> &&all_msgpack_encoders) noexcept :
    all_msgpack_encoders_(all_msgpack_encoders) {}

  void compile(CodeGenerator &W) const final;

  static const std::string all_tags_file_;
  static std::string get_cppStructTag_name(const std::string &msgpack_encoder) noexcept;

private:
  const std::set<ClassPtr> all_msgpack_encoders_;
};
