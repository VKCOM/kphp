// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/file-to-tokens.h"

#include "compiler/data/src-file.h"
#include "compiler/lexer.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"

void FileToTokensF::execute(SrcFilePtr file, DataStream<std::pair<SrcFilePtr, std::vector<Token>>>& os) {
  stage::set_name("Split file to tokens");
  stage::set_file(file);
  kphp_assert(file);

  kphp_assert(file->loaded);
  auto tokens = php_text_to_tokens(file->text);

  if (stage::has_error()) {
    return;
  }

  os << std::make_pair(file, std::move(tokens));
}
