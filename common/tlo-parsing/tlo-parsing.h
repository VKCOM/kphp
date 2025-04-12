// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <string>

namespace vk {
namespace tlo_parsing {

struct tl_scheme;

struct TLOParsingResult {
  TLOParsingResult();
  TLOParsingResult(TLOParsingResult&&);
  ~TLOParsingResult();

  std::unique_ptr<tl_scheme> parsed_schema;
  std::string error;
};

TLOParsingResult parse_tlo(const char* tlo_path, bool rename_all_forbidden_names);
void rename_tl_name_if_forbidden(std::string& tl_name);

void remove_cyclic_types(tl_scheme& scheme);
void remove_exclamation_types(tl_scheme& scheme);
void replace_anonymous_args(tl_scheme& scheme);
void perform_flat_optimization(tl_scheme& scheme);
void tl_scheme_final_check(const tl_scheme& scheme);

} // namespace tlo_parsing
} // namespace vk
