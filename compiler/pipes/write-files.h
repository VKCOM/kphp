// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/writer-data.h"
#include "compiler/threading/data-stream.h"

class WriteFilesF {
public:
  void execute(WriterData *data, EmptyStream &);
};
