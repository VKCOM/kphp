// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/ffi/c_parser/parsing_driver.h"

#include "compiler/kphp_assert.h"

#include "common/algorithms/find.h"
#include "common/containers/final_action.h"

#include <charconv>
#include <unordered_set>
#include <vector>

using namespace ffi;

inline long parse_string_span(string_span s) {
  const auto *start = s.data_;
  long result = 0;
  int base = 10;
  if (start[0] == '0' && start[1] == 'b') {
    base = 2;
    start += 2;
  } else if (start[0] == '0' && start[1] == 'x') {
    base = 16;
    start += 2;
  } else if (start[0] == '0') {
    base = 8;
    start += 1;
  }
  std::from_chars(start, start + s.len_, result, base);
  return result;
}

static FFITypeKind int_type_by_size(size_t size, bool is_signed = true) {
  switch (size) {
    case 1:
      return is_signed ? FFITypeKind::Int8 : FFITypeKind::Uint8;
    case 2:
      return is_signed ? FFITypeKind::Int16 : FFITypeKind::Uint16;
    case 4:
      return is_signed ? FFITypeKind::Int32 : FFITypeKind::Uint32;
    default:
      return is_signed ? FFITypeKind::Int64 : FFITypeKind::Uint64;
  }
}

static void finalize_types(FFIType *type) {
  const auto finalize = [](const FFIType *type) {
    // we make an assumption that unsigned types have the same bit size as their signed counterparts
    static_assert(sizeof(unsigned) == sizeof(signed), "odd-sized unsigned");
    static_assert(sizeof(unsigned long) == sizeof(signed long), "odd-sized unsigned long");
    static_assert(sizeof(unsigned long long) == sizeof(signed long long), "odd-sized unsigned long long");

    // interpret things like "unsigned" as "unsigned int", etc
    FFITypeKind kind = type->kind;
    if (kind == FFITypeKind::_typeFlags) {
      kind = FFITypeKind::_int;
    }

    // simplify any enum type to "int" for now
    if (kind == FFITypeKind::Enum) {
      enum EnumSizeTest { A, B };
      // do non-const assert to fail only when we need to emit a FFI enum
      kphp_assert(sizeof(EnumSizeTest) == sizeof(int));
      kind = FFITypeKind::_int;
    }

    if (kind == FFITypeKind::_size) {
      return int_type_by_size(sizeof(size_t), false);
    }
    if (kind == FFITypeKind::_intptr) {
      return int_type_by_size(sizeof(intptr_t), true);
    }
    if (kind == FFITypeKind::_uintptr) {
      return int_type_by_size(sizeof(uintptr_t), false);
    }

    if (kind == FFITypeKind::_int) {
      bool is_signed = !(type->flags & FFIType::Flag::Unsigned);
      if (type->flags & FFIType::Flag::LongLong) {
        return int_type_by_size(sizeof(long long int), is_signed);
      }
      if (type->flags & FFIType::Flag::Long) {
        return int_type_by_size(sizeof(long int), is_signed);
      }
      if (type->flags & FFIType::Flag::Short) {
        return int_type_by_size(sizeof(short int), is_signed);
      }
      return int_type_by_size(sizeof(int), is_signed);
    }

    if (kind == FFITypeKind::Char) {
      if (type->flags & FFIType::Flag::Unsigned) {
        return FFITypeKind::Uint8;
      }
      if (type->flags & FFIType::Flag::Signed) {
        return FFITypeKind::Int8;
      }
      return FFITypeKind::Char;
    }

    return kind;
  };

  type->kind = finalize(type);
  for (auto &member : type->members) {
    finalize_types(member);
  }
}

FFIType *ParsingDriver::function_to_var(FFIType *function) {
  function->kind = FFITypeKind::Var;
  FFIType *function_ptr_type = alloc.new_type(FFITypeKind::FunctionPointer);
  function_ptr_type->members = std::move(function->members);
  function->members = std::vector<FFIType*>{function_ptr_type};
  return function;
}

void ParsingDriver::add_missing_decls() {
  // when we see this statement in the program:
  //
  //   typedef struct Foo Foo;
  //
  // it could be a standalone forward declaration of Foo,
  // or it may be a normal typedef that is written *before*
  // the struct itself is defined.
  //
  // we want to auto-declare a forward declaration (StructDef)
  // only in the case when there is no StructDef anywhere
  // for that name already

  std::unordered_set<std::string> declared_types;
  declared_types.reserve(types.size());
  for (const FFIType *type : types) {
    declared_types.insert(type->str);
  }

  for (const auto &it : may_need_forward_decl) {
    if (vk::contains(declared_types, it.first)) {
      continue;
    }
    FFIType *fwd_decl = alloc.new_type(it.second);
    fwd_decl->str = it.first;
    add_type(fwd_decl);
  }
}

ParsingDriver::Result ParsingDriver::parse() {
  // C parser throws an exception in case of errors,
  // so parse() is expected to return with 0 value (no errors)
  kphp_assert(yy_parser.parse() == 0);

  add_missing_decls();

  for (FFIType *type : types) {
    alloc.mark_used(type);
  }
  auto num_deleted = alloc.collect_garbage();

  Result result;
  result.types = types;
  result.enum_constants = enum_constants;
  result.num_deleted = num_deleted;
  result.num_allocated = alloc.num_allocated();

  return result;
}

void ParsingDriver::add_type(FFIType *type) {
  vk::finally([&] () {
      lexer.reset_comment();
  });

  if (type->kind == FFITypeKind::Unknown) {
    return;
  }
  if (vk::any_of_equal(type->kind, FFITypeKind::Struct, FFITypeKind::Union)) {
    FFIType *fwd_decl = alloc.new_type(type->kind == FFITypeKind::Struct ? FFITypeKind::StructDef : FFITypeKind::UnionDef);
    fwd_decl->str = type->str;
    add_type(fwd_decl);
    return;
  }
  if (type->kind == FFITypeKind::_typesList) {
    for (const auto &x : type->members) {
      add_type(x);
    }
    return;
  }
  finalize_types(type);
  types.emplace_back(type);
}

void ParsingDriver::add_typedef(FFIType *type, FFIType *declarator) {
  FFIType *combined_type = combine(DeclarationSpecifiers{type}, Declarator{declarator});
  finalize_types(combined_type);

  if (combined_type->kind == FFITypeKind::Function) {
    combined_type = function_to_var(combined_type);
  }
  if (combined_type->kind != FFITypeKind::Var) {
    return;
  }

  std::string alias_name = combined_type->str;
  FFIType *aliased_type = combined_type->members[0];

  if (vk::any_of_equal(aliased_type->kind, FFITypeKind::Struct, FFITypeKind::Union)) {
    may_need_forward_decl.emplace(aliased_type->str, aliased_type->kind == FFITypeKind::Struct ? FFITypeKind::StructDef : FFITypeKind::UnionDef);
  }

  // typedef on inline struct (or union) type definition,
  // e.g. `typedef struct Name { ... } alias_name`
  if (vk::any_of_equal(aliased_type->kind, FFITypeKind::StructDef, FFITypeKind::UnionDef)) {
    if (aliased_type->str.empty()) {
      aliased_type->str = alias_name;
    }
    add_type(aliased_type); // save struct (or union) definition
    bool is_struct = (aliased_type->kind == FFITypeKind::StructDef);
    FFIType *new_aliased_type = alloc.new_type(is_struct ? FFITypeKind::Struct : FFITypeKind::Union);
    new_aliased_type->str = aliased_type->str;
    aliased_type = new_aliased_type;
  }

  // typedef on inline enum definition,
  // e.g. `typedef enum Name { ... } alias_name`
  if (aliased_type->kind == FFITypeKind::EnumDef) {
    if (aliased_type->str.empty()) {
      aliased_type->str = alias_name;
    }
    FFIType *new_aliased_type = alloc.new_type(FFITypeKind::Enum);
    new_aliased_type->str = aliased_type->str;
    aliased_type = new_aliased_type;
  }

  if (!alias_name.empty()) {
    alloc.mark_used(aliased_type, false);
    typedefs[alias_name] = aliased_type;
  }
}

void ParsingDriver::add_enumerator(FFIType *enum_list, FFIType *enumerator) {
  int enum_value = enumerator->num;
  if (enumerator->num == 0 || enumerator->num == enum_list->num) {
    enum_value = enum_list->num++;
  } else {
    enum_list->num = enum_value + 1;
  }

  enum_constants.emplace(enumerator->str, enum_value);
}

int ParsingDriver::stoi(string_span s) {
  return parse_string_span(s);
}

FFIType *ParsingDriver::combine(const TypeQualifier &type_qualifier, const DeclarationSpecifiers &decl_specs) {
  return combine(TypeSpecifier{type_qualifier.type}, decl_specs);
}

FFIType *ParsingDriver::combine(const TypeSpecifier &type_spec, const SpecifierQualifierList &list) {
  return combine(type_spec, DeclarationSpecifiers{list.type});
}

FFIType *ParsingDriver::combine(const TypeQualifier &type_qualifier, const SpecifierQualifierList &list) {
  return combine(type_qualifier, DeclarationSpecifiers{list.type});
}

FFIType *ParsingDriver::combine(const TypeSpecifier &type_spec, const DeclarationSpecifiers &decl_specs) {
  // TODO: errors for things like combine(char, char).

  if (decl_specs.type->kind == FFITypeKind::_typeFlags && type_spec.type->kind != FFITypeKind::_typeFlags) {
    return combine(TypeSpecifier{decl_specs.type}, DeclarationSpecifiers{type_spec.type});
  }
  uint32_t flags = type_spec.type->flags | decl_specs.type->flags;
  if ((type_spec.type->flags & FFIType::Flag::Long) && (decl_specs.type->flags & FFIType::Flag::Long)) {
    flags |= FFIType::Flag::LongLong;
  }
  FFIType *result = decl_specs.type;
  result->flags = static_cast<FFIType::Flag>(flags);
  return result;
}

FFIType *ParsingDriver::combine(const DeclarationSpecifiers &decl_specs, const InitDeclaratorList &init_declarator_list) {
  FFIType *result = alloc.new_type(FFITypeKind::_typesList);
  for (auto &declarator : init_declarator_list.type->members) {
    // init_declarator is basically a simple declarator right now
    result->members.emplace_back(combine(decl_specs, Declarator{declarator}));
  }
  return result;
}

FFIType *ParsingDriver::combine(const DeclarationSpecifiers &decl_specs, const Declarator &declarator) {
  if (declarator.type->kind == FFITypeKind::Name) {
    FFIType *result = alloc.new_type(FFITypeKind::Var);
    result->str = declarator.type->str;
    result->members.emplace_back(decl_specs.type);
    return result;
  }

  if (declarator.type->kind == FFITypeKind::Function) {
    FFIType *result = alloc.new_type();
    *result = *declarator.type;
    result->members[0] = decl_specs.type;
    return result;
  }

  if (declarator.type->kind == FFITypeKind::_pointerDeclarator) {
    FFIType *ptr_type = alloc.new_type(FFITypeKind::Pointer);
    ptr_type->num = declarator.type->num;
    // when typedef over a pointer is combined with a pointer type, we want to
    // merge it into a single pointer type with total indirection;
    // in other words, we want pointer{Elem: T, Num: 2} instead of pointer{Elem: pointer{Elem: T, Num: 1}, Num: 1}
    if (decl_specs.type->kind == FFITypeKind::Pointer) {
      ptr_type->num += decl_specs.type->num;
      ptr_type->members.emplace_back(decl_specs.type->members[0]);
    } else {
      ptr_type->members.emplace_back(decl_specs.type);
    }
    return combine(DeclarationSpecifiers{ptr_type}, Declarator{declarator.type->members[0]});
  }

  if (declarator.type->kind == FFITypeKind::_arrayDeclarator) {
    FFIType *array_type = alloc.new_type(FFITypeKind::Array);
    FFIType *nested_declarator = combine_array_type(array_type, decl_specs.type, declarator.type);
    return combine(DeclarationSpecifiers{array_type}, Declarator{nested_declarator});
  }

  return nullptr;
}

FFIType *ParsingDriver::combine(const Pointer &pointer, const DirectDeclarator &declarator) {
  FFIType *result = alloc.new_type(FFITypeKind::_pointerDeclarator);
  result->num = pointer.type->num;
  result->members.emplace_back(declarator.type);
  return result;
}

FFIType *ParsingDriver::combine(const Pointer &pointer, const DirectAbstractDeclarator &declarator) {
  return combine(pointer, DirectDeclarator{declarator.type});
}

FFIType *ParsingDriver::combine(const DeclarationSpecifiers &decl_specs, const AbstractDeclarator &declarator) {
  if (declarator.type->kind == FFITypeKind::Pointer) {
    FFIType *ptr_declarator = alloc.new_type(FFITypeKind::_pointerDeclarator);
    ptr_declarator->num = declarator.type->num;
    FFIType *name = alloc.new_type(FFITypeKind::Name);
    ptr_declarator->members.emplace_back(name);
    return combine(decl_specs, Declarator{ptr_declarator});
  }

  return combine(decl_specs, Declarator{declarator.type});;
}

FFIType *ParsingDriver::combine_array_type(FFIType *dst, FFIType *elem_type, FFIType *current) {
  dst->num = current->num;
  if (current->members[0]->kind != FFITypeKind::_arrayDeclarator) {
    dst->members.emplace_back(elem_type);
    return current->members[0];
  }
  FFIType *nested_array_type = alloc.new_type(FFITypeKind::Array);
  FFIType *declarator = combine_array_type(nested_array_type, elem_type, current->members[0]);
  dst->members.emplace_back(nested_array_type);
  return declarator;
}

FFIType *ParsingDriver::make_enum_member(string_span name, int value) {
  FFIType *result = alloc.new_type(FFITypeKind::_enumMember);
  result->str = name.to_string();
  result->num = value;
  return result;
}

FFIType *ParsingDriver::make_abstract_array_declarator(string_span size_str) {
  FFIType *result = alloc.new_type(FFITypeKind::_arrayDeclarator);
  result->num = parse_string_span(size_str);
  FFIType *name = alloc.new_type(FFITypeKind::Name);
  result->members.emplace_back(name);
  return result;
}

FFIType *ParsingDriver::make_array_declarator(FFIType *declarator, string_span size_str) {
  FFIType *result = alloc.new_type(FFITypeKind::_arrayDeclarator);
  result->num = parse_string_span(size_str);
  result->members.emplace_back(declarator);
  return result;
}

FFIType *ParsingDriver::make_function(FFIType *func_expr, FFIType *params) {
  FFIType *function_type = alloc.new_type(FFITypeKind::Function);
  kphp_assert(vk::any_of_equal(func_expr->kind, FFITypeKind::Name, FFITypeKind::_pointerDeclarator));
  bool is_func_ptr = func_expr->kind == FFITypeKind::_pointerDeclarator;
  if (is_func_ptr) {
    // array of function pointers is not supported directly, but it can
    // be achieved via typedef if necessary
    if (func_expr->members[0]->kind != FFITypeKind::Name) {
      raise_error("parsing function pointer: only simple identifiers are supported (use typedef for arrays)");
    }
    function_type->str = func_expr->members[0]->str;
  } else {
    function_type->str = func_expr->str;
    vk::string_view doc_comment = lexer.get_comment().to_string_view();
    if (vk::contains(doc_comment, "@kphp-ffi-signalsafe")) {
      function_type->flags = static_cast<FFIType::Flag>(function_type->flags | FFIType::Flag::SignalSafe);
    }
  }
  FFIType *result_type_placeholder = alloc.new_type(); // return type will be inserted here later
  function_type->members.emplace_back(result_type_placeholder);
  if (params) { // nullptr for a func with an empty param list
    if (!expr_mode && params->members.size() == 1 && params->members[0]->kind == FFITypeKind::Void) {
      // let f(void) be f()
      return function_type;
    }

    for (FFIType *p : params->members) {
      if (p->kind == FFITypeKind::_ellipsis) {
        function_type->flags = static_cast<FFIType::Flag>(function_type->flags | FFIType::Flag::Variadic);
        continue;
      }

      // for the convenience, turn unnamed params into a Field with empty str
      if (p->kind == FFITypeKind::Function) {
        function_type->members.emplace_back(function_to_var(p));
      } else if (p->kind == FFITypeKind::Var) {
        function_type->members.emplace_back(p);
      } else {
        FFIType *var = alloc.new_type(FFITypeKind::Var);
        var->members.emplace_back(p);
        function_type->members.emplace_back(var);
      }
    }
  }
  return function_type;
}

FFIType *ParsingDriver::make_pointer() {
  FFIType *ptr = alloc.new_type(FFITypeKind::Pointer);
  ptr->num = 1;
  return ptr;
}

FFIType *ParsingDriver::make_union_def(FFIType *fields) {
  return make_struct_or_union_def(fields, false);
}

FFIType *ParsingDriver::make_struct_def(FFIType *fields) {
  return make_struct_or_union_def(fields, true);
}

FFIType *ParsingDriver::make_struct_or_union_def(FFIType *fields, bool is_struct) {
  FFIType *struct_type = alloc.new_type(is_struct ? FFITypeKind::StructDef : FFITypeKind::UnionDef);
  for (FFIType *list : fields->members) {
    kphp_assert(list->kind == FFITypeKind::_structFieldList);
    int num_names = list->members.size() - 1;
    for (int i = 0; i < num_names; i++) {
      FFIType *declarator = list->members[i];
      FFIType *field_type = list->members.back();
      FFIType *field = combine(DeclarationSpecifiers{field_type}, Declarator{declarator});
      if (field->kind == FFITypeKind::Function) {
        field = function_to_var(field);
      }
      struct_type->members.emplace_back(field);
    }
  }
  return struct_type;
}

void ParsingDriver::raise_error(const std::string &message) {
  // TODO: better error locations, more context information
  ffi::Location loc{0, 0};
  throw ParsingDriver::ParseError{loc, message};
}
