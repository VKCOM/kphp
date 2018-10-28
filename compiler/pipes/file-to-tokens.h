#pragma once

#include "compiler/data_ptr.h"
#include "compiler/pipes/file-and-token.h"
#include "compiler/threading/data-stream.h"

class FileToTokensF {
public:
  void execute(SrcFilePtr file, DataStream<FileAndTokens> &os);
};
