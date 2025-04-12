// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/ffi/ffi_types.h"
#include "compiler/ffi/c_parser/lexer.h"
#include "compiler/ffi/c_parser/parsing_types.h"
#include "compiler/ffi/c_parser/types-allocator.h"
#include "compiler/ffi/c_parser/yy_parser_generated.hpp"

#include <unordered_map>
#include <map>

namespace ffi {

// How to re-generate lexer & parser:
//
//   $ cd compiler/ffi/c_parser/
//   $ ragel -C -G2 lexer.rl -o lexer_generated.cpp
//   $ bison -o yy_parser_generated.cpp c.y
//
// Ragel: github.com/adrian-thurston/ragel
// Bison: https://www.gnu.org/software/bison/
//
// Verified versions:
// Ragel 6.10
// Bison 3.7
class ParsingDriver {
public:
  struct Result {
    std::vector<FFIType*> types;
    std::map<std::string, int> enum_constants;
    uint64_t num_allocated;
    uint64_t num_deleted;
  };

  class ParseError : public std::exception {
  public:
    ParseError(ffi::Location location, std::string message)
      : location{location}
      , message{std::move(message)} {}

    ffi::Location location;
    std::string message;
  };

  explicit ParsingDriver(const std::string &src, FFITypedefs &typedefs, bool expr_mode = false):
    typedefs(typedefs),
    expr_mode(expr_mode),
    lexer(typedefs, src),
    yy_parser(lexer, *this) {
  }

  Result parse();

  FFIType *get_aliased_type(string_span name) {
    return typedefs[name.to_string()];
  }

  void add_type(FFIType *type);
  void add_typedef(FFIType *type, FFIType *declarator);
  void add_enumerator(FFIType *enum_list, FFIType *enumerator);

  int stoi(string_span s);

  FFIType *combine(const TypeSpecifier &type_spec, const SpecifierQualifierList &list);
  FFIType *combine(const TypeQualifier &type_qualifier, const SpecifierQualifierList &list);
  FFIType *combine(const TypeSpecifier &type_spec, const DeclarationSpecifiers &decl_specs);
  FFIType *combine(const TypeQualifier &type_qualifier, const DeclarationSpecifiers &decl_specs);
  FFIType *combine(const DeclarationSpecifiers &decl_specs, const InitDeclaratorList &init_declarator_list);
  FFIType *combine(const DeclarationSpecifiers &decl_specs, const Declarator &declarator);
  FFIType *combine(const DeclarationSpecifiers &decl_specs, const AbstractDeclarator &declarator);
  FFIType *combine(const Pointer &pointer, const DirectDeclarator &declarator);
  FFIType *combine(const Pointer &pointer, const DirectAbstractDeclarator &declarator);

  FFIType *make_simple_type(FFITypeKind kind) {
    return alloc.new_type(kind);
  }

  FFIType *make_simple_type(FFITypeKind kind, FFIType::Flag flags) { return alloc.new_type(kind, flags); }

  FFIType *make_enum_member(string_span name, int value);
  FFIType *make_abstract_array_declarator(string_span size_str);
  FFIType *make_array_declarator(FFIType *declarator, string_span size_str);
  FFIType *make_function(FFIType *func_expr, FFIType *params);
  FFIType *make_pointer();
  FFIType *make_struct_def(FFIType *fields);
  FFIType *make_union_def(FFIType *fields);

  friend class YYParser;

private:
  FFITypedefs &typedefs;
  bool expr_mode;
  Lexer lexer;
  YYParser yy_parser;
  TypesAllocator alloc;
  std::vector<FFIType*> types;
  std::unordered_map<std::string, FFITypeKind> may_need_forward_decl;
  std::map<std::string, int> enum_constants;

  FFIType *combine_array_type(FFIType *dst, FFIType *elem_type, FFIType *current);
  FFIType *make_struct_or_union_def(FFIType *fields, bool is_struct);

  FFIType *function_to_var(FFIType *function);

  void raise_error(const std::string &message);

  void add_missing_decls();
};

} // namespace ffi
