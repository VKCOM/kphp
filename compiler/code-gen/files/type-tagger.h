// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <map>
#include <vector>

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/inferring/type-data.h"

struct IncludesCollector;

struct TypeTagger : CodeGenRootCmd {
  TypeTagger(std::vector<const TypeData *> &&forkable_types, std::vector<const TypeData *> &&waitable_types);
  void compile(CodeGenerator &W) const final;

private:
  IncludesCollector collect_includes() const noexcept;
  std::map<int, std::string> collect_hash_of_types() const noexcept;
  void compile_tagger(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept;
  void compile_loader(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept;

  std::vector<const TypeData *> forkable_types;
  std::vector<const TypeData *> waitable_types;
};
