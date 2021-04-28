// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>

#include "compiler/code-gen/code-generator.h"
#include "compiler/data/data_ptr.h"

struct InitScriptsCpp {
  SrcFilePtr main_file_id;
  explicit InitScriptsCpp(SrcFilePtr main_file_id);
  void compile(CodeGenerator &W) const;
};

struct LibVersionHFile {
  void compile(CodeGenerator &W) const;
};

struct CppMainFile {
  void compile(CodeGenerator &W) const;
};
