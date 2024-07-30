// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/msgpack-encoder-tags.h"

#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/data/class-data.h"
#include "compiler/utils/string-utils.h"

const std::string MsgPackEncoderTags::all_tags_file_ = "_msgpack_tags.h";

std::string MsgPackEncoderTags::get_cppStructTag_name(const std::string &msgpack_encoder) noexcept {
  return replace_backslashes(msgpack_encoder) + "Tag";
}

void MsgPackEncoderTags::compile(CodeGenerator &W) const {
  if (all_msgpack_encoders_.empty()) {
    return;
  }

  W << OpenFile{all_tags_file_};
  W << "#pragma once" << NL;
  for (ClassPtr msgpack_encoder : all_msgpack_encoders_) {
    W << "struct " << get_cppStructTag_name(msgpack_encoder->name) << "{};" << NL;
  }
  W << CloseFile{};
}
