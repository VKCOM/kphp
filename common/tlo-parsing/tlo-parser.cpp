// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/tlo-parsing/tlo-parser.h"

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

#include "common/tl/tls.h"
#include "common/tlo-parsing/tl-objects.h"
#include "common/tlo-parsing/tlo-parsing.h"

namespace vk {
namespace tlo_parsing {
tlo_parser::tlo_parser(const char *tlo_path) :
  pos(0) {
  FILE *f = std::fopen(tlo_path, "rb");
  if (f == nullptr) {
    error("%s", strerror(errno));
    return;
  }

  std::fseek(f, 0, SEEK_END);
  auto file_size = std::ftell(f);
  std::rewind(f);
  data.resize(file_size);

  len = fread(data.data(), 1, file_size, f);
  std::fclose(f);
  if (!(len > 0 && len == file_size)) {
    error("Error while reading file %s", tlo_path);
    return;
  }
  tl_sch = std::make_unique<tl_scheme>();
}

std::string tlo_parser::get_string() {
  check_pos(4);
  size_t len = *reinterpret_cast<unsigned char *>(data.data() + pos);
  if (len < 254) {
    pos += 1;
  } else {
    len = *reinterpret_cast<unsigned int *>(data.data() + pos);
    pos += 4;
  }
  check_pos(len);
  size_t old_pos = pos;
  pos += len;
  pos += (-pos & 3);
  return {data.data() + old_pos, len};
}

void tlo_parser::check_pos(size_t size) {
  assert (pos + size <= len);
}

std::unique_ptr<type_expr_base> tlo_parser::read_type_expr() {
  auto magic = get_value<unsigned int>();
  switch (magic) {
    case TL_TLS_TYPE_VAR: {
      return std::make_unique<type_var>(this);
    }
    case TL_TLS_TYPE_EXPR: {
      return std::make_unique<type_expr>(this);
    }
    case TL_TLS_ARRAY: {
      return std::make_unique<type_array>(this);
    }
    default: {
      error("Unexpected type_expr magic: %08x", magic);
      return nullptr;
    }
  }
}

std::unique_ptr<nat_expr_base> tlo_parser::read_nat_expr() {
  auto magic = get_value<unsigned int>();
  switch (magic) {
    case TL_TLS_EXPR_NAT: {
      return std::make_unique<nat_const>(this);
    }
    case TL_TLS_NAT_VAR: {
      return std::make_unique<nat_var>(this);
    }
    default: {
      error("Unexpected nat_expr magic: %08x", magic);
      return nullptr;
    }
  }
}

std::unique_ptr<expr_base> tlo_parser::read_expr() {
  auto magic = get_value<unsigned int>();
  switch (magic) {
    case TL_TLS_EXPR_NAT: {
      return read_nat_expr();
    }
    case TL_TLS_EXPR_TYPE: {
      return read_type_expr();
    }
    default: {
      error("Unexpected expr magic: %08x", magic);
      return nullptr;
    }
  }
}

void tlo_parser::get_schema_version() {
  auto v = get_value<unsigned int>();
  switch (v) {
    case TL_TLS_SCHEMA_V4: {
      tl_sch->scheme_version = 4;
      return;
    }
    case TL_TLS_SCHEMA_V3: {
      tl_sch->scheme_version = 3;
      return;
    }
    case TL_TLS_SCHEMA_V2: {
      tl_sch->scheme_version = 2;
      return;
    }
    default: {
      error("Unexpected tl-scheme version %u", v);
      return;
    }
  }
}

void tlo_parser::get_types(bool rename_all_forbidden_names) {
  int count = get_value<int>();
  for (int i = 0; i < count; ++i) {
    auto magic = get_value<unsigned int>();
    if (magic != TL_TLS_TYPE) {
      error("Unexpected type magic: %08x", magic);
      return;
    }
    auto t = std::make_unique<type>(this);
    if (rename_all_forbidden_names) {
      rename_tl_name_if_forbidden(t->name);
    }
    tl_sch->types[t->id] = std::move(t);
  }
}

void tlo_parser::get_constructors(bool rename_all_forbidden_names) {
  int count = get_value<int>();
  for (int i = 0; i < count; ++i) {
    auto magic = get_value<unsigned int>();
    if (magic != (tl_sch->scheme_version >= 4 ? TL_TLS_COMBINATOR_V4 : TL_TLS_COMBINATOR)) {
      error("Unexpected combinator (constructor) magic: %08x", magic);
      return;
    }
    auto c = std::make_unique<combinator>(this, combinator::combinator_type::CONSTRUCTOR);
    if (rename_all_forbidden_names) {
      rename_tl_name_if_forbidden(c->name);
    }
    tl_sch->types[c->type_id]->constructors.emplace_back(std::move(c));
  }
}

void tlo_parser::get_functions(bool rename_all_forbidden_names) {
  int count = get_value<int>();
  for (int i = 0; i < count; ++i) {
    auto magic = get_value<unsigned int>();
    if (magic != (tl_sch->scheme_version >= 4 ? TL_TLS_COMBINATOR_V4 : TL_TLS_COMBINATOR)) {
      error("Unexpected combinator (function) magic: %08x", magic);
      return;
    }
    auto f = std::make_unique<combinator>(this, combinator::combinator_type::FUNCTION);
    if (rename_all_forbidden_names) {
      rename_tl_name_if_forbidden(f->name);
    }
    tl_sch->functions[f->id] = std::move(f);
  }
}

void tlo_parser::error(const char *format, ...) {
  constexpr size_t BUFF_SZ = 1024;
  char buff[BUFF_SZ];
  va_list args;
  va_start (args, format);
  int sz = vsnprintf(buff, BUFF_SZ, format, args);
  std::string error_msg = std::string(buff, static_cast<size_t>(sz));
  va_end(args);
  throw std::runtime_error(error_msg + "\n");
}

TLOParsingResult::TLOParsingResult() = default;
TLOParsingResult::TLOParsingResult(TLOParsingResult &&) = default;
TLOParsingResult::~TLOParsingResult() = default;

TLOParsingResult parse_tlo(const char *tlo_path, bool rename_all_forbidden_names) {
  TLOParsingResult result;
  try {
    tlo_parser reader{tlo_path};
    reader.get_schema_version();
    reader.get_value<int>();
    reader.get_value<int>();
    reader.get_types(rename_all_forbidden_names);
    reader.get_constructors(rename_all_forbidden_names);
    reader.get_functions(rename_all_forbidden_names);
    for (auto &item : reader.tl_sch->types) {
      reader.tl_sch->magics[item.second->name] = item.first;
      for (auto &ctor : item.second->constructors) {
        reader.tl_sch->magics[ctor->name] = ctor->id;
        reader.tl_sch->owner_type_magics[ctor->id] = item.second->id;
      }
    }
    for (auto &item : reader.tl_sch->functions) {
      reader.tl_sch->magics[item.second->name] = item.first;
    }
    result.parsed_schema = std::move(reader.tl_sch);
  } catch (const std::exception &e) {
    result.error = e.what();
  }
  return result;
}

void rename_tl_name_if_forbidden(std::string &tl_name) {
  static const std::unordered_map<std::string, std::string> RENAMING_MAP = {
    {"ReqResult",       "RpcResponse"},
    {"_",               "rpcResponseOk"},
    {"reqResultHeader", "rpcResponseHeader"},
    {"reqError",        "rpcResponseError"}
  };
  auto it = RENAMING_MAP.find(tl_name);
  if (it != RENAMING_MAP.end()) {
    tl_name = it->second;
    return;
  }
}

} // namespace tlo_parsing
} // namespace vk
