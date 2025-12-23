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
    // "thrown_exception",
    "mixed",
    "array< mixed >",
    "Optional < string >",
    "Optional < array< mixed > >",
    "array< array< mixed > >",
    "class_instance<C$KphpJobWorkerResponse>",
    "class_instance<C$VK$TL$RpcResponse>",
    "array< class_instance<C$VK$TL$RpcResponse> >",
    // "class_instance<C$PDOStatement>",
  };

  for (const auto *type : forkable_types_) {
    sorted_types.emplace(type_out(type, gen_out_style::tagger));
  }

  std::map<int, std::string> hashes;

  for (const auto &type : sorted_types) {
    const auto hash = static_cast<int>(vk::std_hash(type)); // FIXME
    kphp_assert(hash);
    kphp_assert(hashes.emplace(hash, type).second);
  }

  return hashes;
}

void TypeTagger::compile_tagger(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept {
  W << OpenFile{"_tagger.cpp"};
  W << ExternInclude{G->settings().runtime_headers.get()};
  W << includes << NL;

  for (const auto& [hash, type] : hash_of_types) {
    if (G->is_output_mode_k2()) {
      W << "template<>" << NL;
      FunctionSignatureGenerator{W} << "int32_t kphp::forks::details::storage::tagger<" << type << ">::get_tag() " << BEGIN;
      W << "return " << hash << SemicolonAndNL{};
      W << END << NL << NL;
    } else {
      W << "template<>" << NL;
      FunctionSignatureGenerator{W} << "int Storage::tagger<" << type << ">::get_tag() " << BEGIN;
      W << "return " << hash << ";" << NL;
      W << END << NL << NL;
    }
  }

  W << CloseFile{};
}

void TypeTagger::compile_loader_header(CodeGenerator& W, const IncludesCollector& includes, const std::map<int, std::string>& hash_of_types) const noexcept {
  W << OpenFile{loader_file_};
  W << ExternInclude{G->settings().runtime_headers.get()};
  W << includes << NL;

  if (G->is_output_mode_k2()) {
    W << "template<typename T>" << NL;
    FunctionSignatureGenerator{W}
        << "typename kphp::forks::details::storage::loader<T>::loader_function_type kphp::forks::details::storage::loader<T>::get_loader(int32_t tag) "
        << BEGIN;
    W << "switch(tag)" << BEGIN;
    for (const auto& [hash, type] : hash_of_types) {
      W << "case " << hash << ":"
        << " return kphp::forks::details::storage::load_impl<" << type << ", T>;" << NL;
    }
    W << END << NL;
    W << "kphp::log::assertion(false);" << NL;
    W << END << NL;
  } else {
    W << "template<typename T>" << NL;
    FunctionSignatureGenerator{W} << "typename Storage::loader<T>::loader_fun Storage::loader<T>::get_function(int tag)" << BEGIN;
    W << "switch(tag)" << BEGIN;
    for (const auto& [hash, type] : hash_of_types) {
      W << "case " << hash << ":"
        << " return Storage::load_implementation_helper<" << type << ", T>::load;" << NL;
    }
    W << END << NL;
    W << "php_assert(0);" << NL;
    W << END << NL;
  }

  W << CloseFile{};
}

template <typename It>
static void compile_loader_instantiations_batch(CodeGenerator &W, const IncludesCollector &includes, It begin, It end, std::size_t batch) noexcept {
  W << OpenFile{fmt_format("_loader_instantiations{}.cpp", batch)};
  W << ExternInclude{G->settings().runtime_headers.get()};
  W << includes << NL;

  if (G->is_output_mode_k2()) {
    for (auto it = begin; it != end; ++it) {
      W << "template kphp::forks::details::storage::loader<" << *it << ">::loader_function_type kphp::forks::details::storage::loader<" << *it
        << ">::get_loader(int32_t);" << NL;
    }
  } else {
    for (auto it = begin; it != end; ++it) {
      W << "template Storage::loader<" << *it << ">::loader_fun Storage::loader<" << *it << ">::get_function(int);" << NL;
    }
  }

  W << CloseFile{};
}

void TypeTagger::compile_loader_instantiations(CodeGenerator &W) const noexcept {
  IncludesCollector loader_includes;
  loader_includes.add_raw_filename_include(loader_file_);

  std::set<std::string> waitable_types_str;
  for (const auto *type : waitable_types_) {
    waitable_types_str.emplace(type_out(type, gen_out_style::tagger));
  }

  std::size_t batch = 0;
  for (auto it = waitable_types_str.begin(); it != waitable_types_str.end();) {
    const auto left_size = static_cast<std::size_t>(std::distance(it, waitable_types_str.end()));
    const auto shift = std::min(left_size, loader_split_granularity_);
    const auto end = std::next(it, shift);
    compile_loader_instantiations_batch(W, loader_includes, it, end, batch++);
    it = end;
  }
}

void TypeTagger::compile_loader(CodeGenerator &W, const IncludesCollector &includes, const std::map<int, std::string> &hash_of_types) const noexcept {
  compile_loader_header(W, includes, hash_of_types);
  compile_loader_instantiations(W);
}

void TypeTagger::compile(CodeGenerator &W) const {
  const auto includes = collect_includes();
  const auto hash_of_types = collect_hash_of_types();

  compile_tagger(W, includes, hash_of_types);
  compile_loader(W, includes, hash_of_types);
}
