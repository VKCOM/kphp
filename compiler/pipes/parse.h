// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"
#include "compiler/token.h"

class ParseF {
public:
  void execute(std::pair<SrcFilePtr, std::vector<Token>> file_and_tokens, DataStream<FunctionPtr>& os);
};
