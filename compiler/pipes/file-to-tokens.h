// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <utility>
#include <vector>

#include "compiler/data/data_ptr.h"
#include "compiler/threading/data-stream.h"
#include "compiler/token.h"

class Token;
template<class DataT>
class DataStream;
class FileToTokensF {

public:
  void execute(SrcFilePtr file, DataStream<std::pair<SrcFilePtr, std::vector<Token>>>& os);
};
