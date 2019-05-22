#pragma once

#include <string>
#include <vector>

#include "compiler/code-gen/code-generator.h"
#include "compiler/inferring/type-data.h"

struct TypeTagger {
  TypeTagger(std::vector<const TypeData *> &&forkable_types, std::vector<const TypeData *> &&waitable_types);
  void compile(CodeGenerator &W) const;

private:
  std::vector<const TypeData *> forkable_types;
  std::vector<const TypeData *> waitable_types;
};
