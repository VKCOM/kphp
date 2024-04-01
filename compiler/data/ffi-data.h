// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

#include "compiler/data/class-data.h"
#include "compiler/ffi/ffi_types.h"

// FFI symbol is an extern variable or function information carrier
struct FFISymbol {
  // env_index is bound after FFIRoot.bind_symbols() is completed;
  // it's an index inside FFIEnv.symbols array (runtime structure)
  int env_index = -1;

  const FFIType *type = nullptr;

  FFISymbol(const FFIType *type)
    : type{type} {}

  std::string name() const {
    return type->str;
  }

  bool operator<(const FFISymbol &other) const {
    return name() < other.name();
  }
};

struct FFIClassDataMixin {
  FFIClassDataMixin(const FFIType *ffi_type, std::string scope_name, ClassPtr non_ref = {})
    : non_ref{non_ref}
    , ffi_type{ffi_type}
    , scope_name{std::move(scope_name)} {}

  bool is_ref() const noexcept {
    return static_cast<bool>(non_ref);
  }

  ClassPtr non_ref; // for reference types this field will contain the original (non-referential) type
  const FFIType *ffi_type;
  std::string scope_name;
};

struct FFIScopeDataMixin {
  std::string scope_name;

  Location location;

  // -1 means that all symbols originate from a static library;
  // note that we can store the shared lib id only once as opposed to
  // a per-symbol alternative because we limit scopes to 1 source definition
  int shared_lib_id = -1;

  std::vector<FFISymbol> variables;
  std::vector<FFISymbol> functions;
  std::vector<const FFIType *> types; // not sorted!

  std::map<std::string, int> enum_constants;

  FFITypedefs typedefs;

  bool is_shared_lib() const noexcept {
    return shared_lib_id != -1;
  }

  int get_env_offset() const noexcept {
    // variables are placed before functions, so if there
    // are any variables, we use the first variable offset
    if (!variables.empty()) {
      return variables[0].env_index;
    }
    kphp_assert(!functions.empty());
    return functions[0].env_index;
  }

  const FFISymbol *find_variable(const vk::string_view name) {
    return find_symbol(variables, name);
  }
  const FFISymbol *find_function(const vk::string_view name) {
    return find_symbol(functions, name);
  }

private:
  static const FFISymbol *find_symbol(const std::vector<FFISymbol> &sorted_vector, vk::string_view name) {
    const auto &it =
      std::lower_bound(sorted_vector.begin(), sorted_vector.end(), name, [](const FFISymbol &lhs, vk::string_view name) { return lhs.name() < name; });
    if (it != sorted_vector.end() && it->name() == name) {
      return &sorted_vector[std::distance(sorted_vector.begin(), it)];
    }
    return nullptr;
  }
};

struct FFISharedLib {
  int id;
  std::string path;
};

class FFIRoot {
  std::mutex mutex;

  std::vector<FFISharedLib> shared_libs;
  std::unordered_map<std::string, FFIScopeDataMixin *> scopes;

  int num_dynamic_symbols = 0;

public:
  const TypeHint *c2php_field_type_hint(const TypeHint *c_hint);
  const TypeHint *c2php_return_type_hint(const TypeHint *c_hint);
  const TypeHint *c2php_scalar_type_hint(ClassPtr cdata_class);

  const TypeHint *create_type_hint(const FFIType *type, const std::string &scope_name);

  const std::vector<FFISharedLib> &get_shared_libs() const;
  int get_shared_lib_id(const std::string &path);

  std::vector<FFIScopeDataMixin *> get_scopes() const;
  FFIScopeDataMixin *find_scope(const std::string &scope_name);

  static const FFIType *get_ffi_type(ClassPtr klass);
  static const FFIType *get_ffi_type(const TypeHint *type_hint);
  static std::string get_ffi_scope(const TypeHint *type_hint);

  static bool is_ffi_scope_call(VertexAdaptor<op_func_call> call);

  static void register_builtin_classes(DataStream<FunctionPtr> &os);

  bool register_scope(const std::string &scope_name, FFIScopeDataMixin *data);

  void bind_symbols();

  int get_dynamic_symbols_num() const noexcept {
    return num_dynamic_symbols;
  }

  // produces a name suitable for a C code declaration;
  // uses a scope_name as a prefix part
  //
  // note that not every symbol needs to be mangled
  static std::string c_name_mangle(const std::string &scope_name, const std::string name) {
    return "ffi_" + scope_name + "_" + name;
  }

  static std::string scope_class_name(const std::string &scope_name) {
    return "scope$" + scope_name;
  }

  static std::string cdata_class_name(const std::string &scope_name, const std::string &name) {
    return "cdata$" + scope_name + "\\" + name;
  }

private:
  enum class Php2cMode {
    FuncReturn,
    FieldRead,
  };
  const TypeHint *c2php_type_hint(const TypeHint *c_type, Php2cMode mode);
  const TypeHint *c2php_scalar_type_hint(FFITypeKind kind);

  const TypeHint *unboxed_type_hint(const FFIType *type, const std::string &scope_name, bool is_return);
};

const FFIBuiltinType *ffi_builtin_type(FFITypeKind);
const FFIBuiltinType *ffi_builtin_type(const std::string &php_class_name);

// FFI printing routines:
//
// * type_string     -- printing for types "as a whole"
// * decltype_string -- printing types in a context of type expression (e.g. a type cast)
//
// Unmangled names are used for error messages (so the user sees the original names)
// and for static lib function and variables (so they link properly).
// Mangled names are used for all struct/union types so we can avoid naming collisions.
//
// The name mangling is performed by a FFIRoot::c_name_mangle function.

std::string ffi_type_string(const FFIType *type);
std::string ffi_mangled_type_string(const std::string &scope_name, const FFIType *type);
std::string ffi_decltype_string(const FFIType *type);
std::string ffi_mangled_decltype_string(const std::string &scope_name, const FFIType *type);
std::string ffi_mangled_decltype_string(const TypeHint *type_hint);
