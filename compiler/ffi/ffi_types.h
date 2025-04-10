// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class FFITypeKind : uint16_t {
  Unknown,

  // Function is a function definition.
  // this->str: function name
  // this->members[0]: return type
  // this->members[1:...]: param types
  Function,

  // FunctionPointer is a pointer that describes a function type
  // members[0]: return type
  // members[1:...]: param types
  //
  // Example: void (*) (int, int) | [void, int, int]
  //
  // We only support simple function pointer types;
  // it's impossible to express higher order functions or function pointers that return a function pointer
  // this makes the parser grammar and the type tree easier while it still covers a big
  // portion of what we want to support
  FunctionPointer,

  // Var is used for both struct members and named function params
  // this->str: name
  // this->members[0]: type
  Var,

  Struct,    // str: struct type name
  StructDef, // str: struct type name; members: StructField entries

  Enum,    // str: enum type name
  EnumDef, // str: enum type name

  Union,    // str: union type name
  UnionDef, // str: union type name; members: StructField entries

  Name, // str: associated name

  // members[0]: element type
  // num: indirection level
  //
  // *T  -> members[0]=T, num=1
  // **T -> members[0]=T, num=2
  Pointer,

  // members[0]: element type
  // num: array size
  //
  // T[0]  -> members[0]=T, num=0
  // T[10] -> members[0]=T, num=10
  // T[]   -> members[0]=T, num=-1
  Array,

  // Fundamental C types.
  //
  // We don't resolve char into int8_t or uint8_t to follow the
  // PHP FFI logic as it treats chars differently.

  Void,
  Bool,
  Char,
  Float,
  Double,
  Int8,
  Int16,
  Int32,
  Int64,
  Uint8,
  Uint16,
  Uint32,
  Uint64,

  // Values below are used only during the final type construction.
  // They're like intermediate AST representations.

  _abstractArrayDeclarator, // num: array size
  _arrayDeclarator,         // num: array size
  _pointerDeclarator,       // num: indirection level
  _typesList,
  _structFieldList,
  _initDeclaratorList,
  _enumList,
  _enumMember, // str: constant name; num: constant int value (or 0 if unset)
  _int,
  _size,
  _intptr,
  _uintptr,
  _typeFlags,
  _ellipsis,
};

struct FFIType {
  enum Flag : uint16_t {
    EmptySet = 0,

    Const = 1 << 0,
    Volatile = 1 << 1,
    Variadic = 1 << 2,

    Unsigned = 1 << 3,
    Signed = 1 << 4,
    Short = 1 << 5,
    Long = 1 << 6,
    LongLong = 1 << 7, // Second "long" specifier bit

    Used = 1 << 8,

    SignalSafe = 1 << 9,
  };

  FFITypeKind kind = FFITypeKind::Unknown;
  Flag flags = Flag::EmptySet;
  int32_t num = 0;
  std::string str;
  std::vector<FFIType*> members;

  FFIType() = default;

  explicit FFIType(FFITypeKind kind)
      : kind{kind} {}
  explicit FFIType(std::string str)
      : str{std::move(str)} {}
  explicit FFIType(Flag flags)
      : flags{flags} {}

  FFIType(FFITypeKind kind, Flag flags)
      : kind{kind},
        flags{flags} {}

  bool is_const() const noexcept {
    return flags & Flag::Const;
  }
  bool is_variadic() const noexcept {
    return flags & Flag::Variadic;
  }
  bool is_signal_safe() const noexcept {
    return flags & Flag::SignalSafe;
  }

  // whether a type is `void*`
  bool is_void_ptr() const noexcept {
    return kind == FFITypeKind::Pointer && members[0]->kind == FFITypeKind::Void && num == 1;
  }

  // whether a type is `const char*`
  bool is_cstring() const noexcept {
    return kind == FFITypeKind::Pointer && members[0]->kind == FFITypeKind::Char && members[0]->is_const() && num == 1;
  }
};

struct FFIBuiltinType {
  FFIType type;
  std::string php_class_name;
  std::string c_name;
  std::string src_name;
};

void ffi_type_delete(const FFIType* type);

using FFITypedefs = std::unordered_map<std::string, FFIType*>;
