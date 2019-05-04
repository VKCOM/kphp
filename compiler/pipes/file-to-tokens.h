#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"
#include "compiler/token.h"

class FileToTokensF {
public:
  void execute(SrcFilePtr file, DataStream<std::pair<SrcFilePtr, std::vector<Token>>> &os);
};
