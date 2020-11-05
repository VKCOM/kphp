// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>

#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct InitScriptsCpp {
  std::vector<SrcFilePtr> main_file_ids;
  std::vector<FunctionPtr> all_functions;
  InitScriptsCpp(std::vector<SrcFilePtr> &&main_file_ids, std::vector<FunctionPtr> &&all_functions);
  void compile(CodeGenerator &W) const;
};
