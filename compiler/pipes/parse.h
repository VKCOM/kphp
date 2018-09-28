#pragma once

#include "compiler/bicycle.h"
#include "compiler/pipes/file-and-token.h"

class ParseF {
public:
  void on_finish(DataStream<FunctionPtr> &) {}

  void execute(FileAndTokens file_and_tokens, DataStream<FunctionPtr> &os);
};
