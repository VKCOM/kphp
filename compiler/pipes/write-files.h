#pragma once

#include "compiler/code-gen/writer-data.h"
#include "compiler/threading/data-stream.h"

class WriteFilesF {
public:
  void execute(WriterData data, EmptyStream &);
};
