// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/json-encoder-tags.h"

#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/data/class-data.h"
#include "compiler/utils/string-utils.h"

const std::string JsonEncoderTags::all_tags_file_ = "_json_tags.h";

std::string JsonEncoderTags::get_cppStructTag_name(const std::string &json_encoder) noexcept {
  return replace_backslashes(json_encoder) + "Tag";
}

void JsonEncoderTags::compile(CodeGenerator &W) const {
  if (all_json_encoders_.empty()) {
    return;
  }

  W << OpenFile{all_tags_file_};
  W << "#pragma once" << NL;
  for (ClassPtr json_encoder : all_json_encoders_) {
    W << "struct " << get_cppStructTag_name(json_encoder->name) << "{};" << NL;
  }
  W << CloseFile{};
}
