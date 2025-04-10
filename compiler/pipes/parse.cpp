// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/parse.h"

#include "compiler/gentree.h"

void ParseF::execute(std::pair<SrcFilePtr, std::vector<Token>> file_and_tokens, DataStream<FunctionPtr>& os) {
  stage::set_name("Parse file");
  stage::set_file(file_and_tokens.first);
  kphp_assert(file_and_tokens.first);

  GenTree{std::move(file_and_tokens.second), file_and_tokens.first, os}.run();
}
