// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/files/type-tagger.h"

#include "common/algorithms/hashes.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/naming.h"
#include "compiler/inferring/public.h"
#include "runtime/php_assert.h"

TypeTagger::TypeTagger(std::vector<const TypeData *> &&forkable_types, std::vector<const TypeData *> &&waitable_types) :
  forkable_types(std::move(forkable_types)),
  waitable_types(std::move(waitable_types)) {}

IncludesCollector TypeTagger::collect_includes() const noexcept {
  IncludesCollector includes;
  for (auto type : forkable_types) {
    includes.add_all_class_types(*type);
  }
  for (auto type : waitable_types) {
    includes.add_all_class_types(*type);
  }
  return includes;
}

std::map<int, std::string> TypeTagger::collect_hash_of_types() const noexcept {
  // Be care, do not remove spaces from these types
  // TODO fix it?
  std::set<std::string> sorted_types = {
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

  for (auto type : forkable_types) {
    sorted_types.insert(type_out(type, gen_out_style::tagger));
  }

  std::map<int, std::string> hashes;

  for (const std::string &type : sorted_types) {
    int hash = vk::std_hash(type);
    kphp_assert(hash);
    kphp_assert(hashes.insert({hash, type}).second);
  }

  return hashes;
}

void TypeTagger::compile_tagger(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept {
  W << OpenFile("_tagger.cpp");
  W << ExternInclude(G->settings().runtime_headers.get());
  W << includes << NL;

  for (const auto &[hash, type] : hash_of_types) {
    W << "template<>" << NL;
    FunctionSignatureGenerator(W) << "int Storage::tagger<" << type << ">::get_tag() " << BEGIN;
    W << "return " << hash << ";" << NL;
    W << END << NL << NL;
  }

  W << CloseFile();
}

void TypeTagger::compile_loader(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept {
  W << OpenFile("_loader.cpp");
  W << ExternInclude(G->settings().runtime_headers.get());
  W << includes << NL;

  W << "template<typename T>" << NL;
  FunctionSignatureGenerator(W) << "typename Storage::loader<T>::loader_fun Storage::loader<T>::get_function(int tag)" << BEGIN;
  W << "switch(tag)" << BEGIN;
  for (const auto &[hash, type] : hash_of_types) {
    W << "case " << hash << ":"
      << " return Storage::load_implementation_helper<" << type << ", T>::load;" << NL;
  }
  W << END << NL;
  W << "php_assert(0);" << NL;
  W << END << NL;

  std::set<std::string> waitable_types_str;
  for (auto type : waitable_types) {
    waitable_types_str.insert(type_out(type, gen_out_style::tagger));
  }
  if (!waitable_types_str.empty()) {
    W << "auto unused_loaders_list __attribute__((unused)) = " << BEGIN;
    for (const auto &type : waitable_types_str) {
      W << "(void*)(Storage::loader<" << type << ">::get_function)," << NL;
    }
    W << END << ";" << NL;
  }

  W << CloseFile();
}

void TypeTagger::compile(CodeGenerator &W) const {
  const auto includes = collect_includes();
  const auto hash_of_types = collect_hash_of_types();

  compile_tagger(W, includes, hash_of_types);
  compile_loader(W, includes, hash_of_types);
}
