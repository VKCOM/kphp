#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"
#include "compiler/token.h"

class ParseF {
public:
  void execute(std::pair<SrcFilePtr, std::vector<Token>> file_and_tokens, DataStream<FunctionPtr> &os);
};
