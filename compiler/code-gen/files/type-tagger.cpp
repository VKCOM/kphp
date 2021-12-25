// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/type-tagger.h"

#include "common/algorithms/hashes.h"
#include "compiler/code-gen/code-generator.h"
#include "compiler/code-gen/common.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/naming.h"
#include "compiler/compiler-core.h"
#include "compiler/inferring/type-data.h"
#include "compiler/kphp_assert.h"

TypeTagger::TypeTagger(std::vector<const TypeData *> &&forkable_types, std::vector<const TypeData *> &&waitable_types) noexcept
  : forkable_types_(std::move(forkable_types))
  , waitable_types_(std::move(waitable_types)) {}

IncludesCollector TypeTagger::collect_includes() const noexcept {
  IncludesCollector includes;
  for (const auto *type : forkable_types_) {
    includes.add_all_class_types(*type);
  }
  for (const auto *type : waitable_types_) {
    includes.add_all_class_types(*type);
  }
  return includes;
}

std::map<int, std::string> TypeTagger::collect_hash_of_types() const noexcept {
  // Be care, do not remove spaces from these types
  // TODO fix it?
  std::set<std::string> sorted_types{
    "bool",
    "int64_t",
    "Optional < int64_t >",
    "void",
    "thrown_exception",
    "mixed",
    "array< mixed >",
    "Optional < string >",
    "Optional < array< mixed > >",
    "array< array< mixed > >",
    "class_instance<C$KphpJobWorkerResponse>",
    "class_instance<C$VK$TL$RpcResponse>",
    "array< class_instance<C$VK$TL$RpcResponse> >"
  };

  for (const auto *type : forkable_types_) {
    sorted_types.emplace(type_out(type, gen_out_style::tagger));
  }

  std::map<int, std::string> hashes;

  for (const auto &type : sorted_types) {
    const auto hash = static_cast<int>(vk::std_hash(type));
    kphp_assert(hash);
    kphp_assert(hashes.emplace(hash, type).second);
  }

  return hashes;
}

void TypeTagger::compile_tagger(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept {
  W << OpenFile{"_tagger.cpp"};
  W << ExternInclude{G->settings().runtime_headers.get()};
  W << includes << NL;

  for (const auto &[hash, type] : hash_of_types) {
    W << "template<>" << NL;
    FunctionSignatureGenerator{W} << "int Storage::tagger<" << type << ">::get_tag() " << BEGIN;
    W << "return " << hash << ";" << NL;
    W << END << NL << NL;
  }

  W << CloseFile{};
}

void TypeTagger::compile_loader_header(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept {
  W << OpenFile{loader_file_};
  W << ExternInclude{G->settings().runtime_headers.get()};
  W << includes << NL;

  W << "template<typename T>" << NL;
  FunctionSignatureGenerator{W} << "typename Storage::loader<T>::loader_fun Storage::loader<T>::get_function(int tag)" << BEGIN;
  W << "switch(tag)" << BEGIN;
  for (const auto &[hash, type] : hash_of_types) {
    W << "case " << hash << ":"
      << " return Storage::load_implementation_helper<" << type << ", T>::load;" << NL;
  }
  W << END << NL;
  W << "php_assert(0);" << NL;
  W << END << NL;

  W << CloseFile{};
}

void TypeTagger::compile_loader_instantiations(CodeGenerator &W, const IncludesCollector &includes) const noexcept {
  W << OpenFile{"_loader_instantiations.cpp"};
  W << ExternInclude{G->settings().runtime_headers.get()};
  W << includes << NL;

  IncludesCollector loader_header;
  loader_header.add_raw_filename_include(loader_file_);
  W << loader_header << NL;

  std::set<std::string> waitable_types_str;
  for (const auto *type : waitable_types_) {
    waitable_types_str.emplace(type_out(type, gen_out_style::tagger));
  }
  for (const auto &type : waitable_types_str) {
    W << "template Storage::loader<" << type << ">::loader_fun Storage::loader<" << type << ">::get_function(int);" << NL;
  }

  W << CloseFile{};
}

void TypeTagger::compile_loader(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept {
  compile_loader_header(W, includes, hash_of_types);
  compile_loader_instantiations(W, includes);
}

void TypeTagger::compile(CodeGenerator &W) const {
  const auto includes = collect_includes();
  const auto hash_of_types = collect_hash_of_types();

  compile_tagger(W, includes, hash_of_types);
  compile_loader(W, includes, hash_of_types);
}
