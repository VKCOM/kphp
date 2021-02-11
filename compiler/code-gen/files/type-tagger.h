// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/inferring/type-data.h"

struct TypeTagger : CodeGenRootCmd {
  TypeTagger(std::vector<const TypeData *> &&forkable_types, std::vector<const TypeData *> &&waitable_types);
  void compile(CodeGenerator &W) const final;

private:
  std::vector<const TypeData *> forkable_types;
  std::vector<const TypeData *> waitable_types;
};
