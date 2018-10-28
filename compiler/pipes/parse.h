#pragma once

#include "compiler/pipes/file-and-token.h"
#include "compiler/threading/data-stream.h"

class ParseF {
public:
  void execute(FileAndTokens file_and_tokens, DataStream<FunctionPtr> &os);
};
