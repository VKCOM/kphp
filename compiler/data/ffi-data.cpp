// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/ffi-data.h"

#include "common/algorithms/contains.h"
#include "compiler/data/function-data.h"
#include "compiler/type-hint.h"

static std::vector<FFIBuiltinType> make_builtin_types() {
  auto make_builtin_type = [](FFITypeKind kind, std::string php_class_name, std::string c_name) {
    std::string src_name = "C$FFI$CData<" + c_name + ">";
    return FFIBuiltinType{
      FFIType{kind},
      std::move(php_class_name),
      std::move(c_name),
      std::move(src_name),
    };
  };

  std::vector<FFIBuiltinType> result = {
    make_builtin_type(FFITypeKind::Void, "FFI\\CData_void", "void"),         make_builtin_type(FFITypeKind::Bool, "FFI\\CData_bool", "bool"),
    make_builtin_type(FFITypeKind::Char, "FFI\\CData_char", "char"),         make_builtin_type(FFITypeKind::Float, "FFI\\CData_float", "float"),
    make_builtin_type(FFITypeKind::Double, "FFI\\CData_double", "double"),   make_builtin_type(FFITypeKind::Int8, "FFI\\CData_int8", "int8_t"),
    make_builtin_type(FFITypeKind::Int16, "FFI\\CData_int16", "int16_t"),    make_builtin_type(FFITypeKind::Int32, "FFI\\CData_int32", "int32_t"),
    make_builtin_type(FFITypeKind::Int64, "FFI\\CData_int64", "int64_t"),    make_builtin_type(FFITypeKind::Uint8, "FFI\\CData_uint8", "uint8_t"),
    make_builtin_type(FFITypeKind::Uint16, "FFI\\CData_uint16", "uint16_t"), make_builtin_type(FFITypeKind::Uint32, "FFI\\CData_uint32", "uint32_t"),
    make_builtin_type(FFITypeKind::Uint64, "FFI\\CData_uint64", "uint64_t"),

    make_builtin_type(FFITypeKind::Unknown, "FFI\\CData", "cdata"),          make_builtin_type(FFITypeKind::Unknown, "", "unknown"),
  };

  return result;
}

static const std::vector<FFIBuiltinType> builtin_types = make_builtin_types();

const FFIBuiltinType *ffi_builtin_type(FFITypeKind kind) {
  for (const auto &builtin_type : builtin_types) {
    if (builtin_type.type.kind == kind) {
      return &builtin_type;
    }
  }
  return nullptr;
}

const FFIBuiltinType *ffi_builtin_type(const std::string &php_class_name) {
  for (const auto &builtin_type : builtin_types) {
    if (builtin_type.php_class_name == php_class_name) {
      return &builtin_type;
    }
  }
  return nullptr;
}

static ClassPtr register_builtin_class(const FFIBuiltinType *builtin, DataStream<FunctionPtr> &os, ClassPtr non_ref = {}) {
  bool is_ref = static_cast<bool>(non_ref);
  auto cdata_class = ClassPtr{new ClassData{ClassType::ffi_cdata}};
  cdata_class->ffi_class_mixin = new FFIClassDataMixin{&builtin->type, "C", non_ref};
  cdata_class->set_name_and_src_name(is_ref ? "&" + builtin->php_class_name : builtin->php_class_name);
  cdata_class->src_name = is_ref ? "CDataRef<" + builtin->c_name + ">" : builtin->src_name;

  G->register_class(cdata_class);

  auto class_holder = cdata_class->gen_holder_function(cdata_class->name);
  G->register_and_require_function(class_holder, os);
  os << class_holder;

  return cdata_class;
}

void FFIRoot::register_builtin_classes(DataStream<FunctionPtr> &os) {
  register_builtin_class(ffi_builtin_type("FFI\\CData"), os);

  for (const auto &builtin_type : builtin_types) {
    if (builtin_type.type.kind == FFITypeKind::Unknown) {
      continue;
    }

    auto cdata_class = register_builtin_class(&builtin_type, os);
    // passing {} as current_function is OK here, the class name is FQN,
    // resolve_context argument will not be used
    cdata_class->add_str_dependent({}, ClassType::klass, "\\FFI\\CData");

    // we'll need ref-wrappers for builtin types too as some operations
    // yield reference types (casting non-pointer scalar types, for instance)
    auto cdata_class_ref = register_builtin_class(&builtin_type, os, cdata_class);
    cdata_class_ref->add_str_dependent({}, ClassType::klass, "\\FFI\\CData");
  }
}

std::string FFIRoot::get_ffi_scope(const TypeHint *type_hint) {
  if (const auto *as_instance = type_hint->try_as<TypeHintInstance>()) {
    auto klass = as_instance->to_type_data()->class_type();
    return klass->ffi_class_mixin ? klass->ffi_class_mixin->scope_name : "";
  }
  if (const auto *as_ffi = type_hint->try_as<TypeHintFFIType>()) {
    return as_ffi->scope_name;
  }
  return "";
}

const FFIType *FFIRoot::get_ffi_type(const TypeHint *type_hint) {
  if (const auto *as_instance = type_hint->try_as<TypeHintInstance>()) {
    return get_ffi_type(as_instance->to_type_data()->class_type());
  }
  if (const auto *as_ffi = type_hint->try_as<TypeHintFFIType>()) {
    return as_ffi->type;
  }
  return nullptr;
}

const FFIType *FFIRoot::get_ffi_type(ClassPtr klass) {
  if (klass->ffi_class_mixin) {
    return klass->ffi_class_mixin->ffi_type;
  }
  return nullptr;
}

const TypeHint *FFIRoot::c2php_scalar_type_hint(ClassPtr cdata_class) {
  if (const auto *ffi_type = get_ffi_type(cdata_class)) {
    return c2php_scalar_type_hint(ffi_type->kind);
  }
  return nullptr;
}

const TypeHint *FFIRoot::c2php_return_type_hint(const TypeHint *c_hint) {
  return c2php_type_hint(c_hint, Php2cMode::FuncReturn);
}

const TypeHint *FFIRoot::c2php_field_type_hint(const TypeHint *c_hint) {
  return c2php_type_hint(c_hint, Php2cMode::FieldRead);
}

const TypeHint *FFIRoot::c2php_scalar_type_hint(FFITypeKind kind) {
  switch (kind) {
    case FFITypeKind::Int8:
    case FFITypeKind::Int16:
    case FFITypeKind::Int32:
    case FFITypeKind::Int64:
    case FFITypeKind::Uint8:
    case FFITypeKind::Uint16:
    case FFITypeKind::Uint32:
    case FFITypeKind::Uint64:
      return TypeHintPrimitive::create(tp_int);

    case FFITypeKind::Void:
      return TypeHintPrimitive::create(tp_void);

    case FFITypeKind::Bool:
      return TypeHintPrimitive::create(tp_bool);

    case FFITypeKind::Float:
    case FFITypeKind::Double:
      return TypeHintPrimitive::create(tp_float);

    default:
      return nullptr;
  }
}

const TypeHint *FFIRoot::c2php_type_hint(const TypeHint *c_hint, Php2cMode mode) {
  const FFIType *type = get_ffi_type(c_hint);
  if (!type) {
    return nullptr;
  }

  if (type->is_cstring()) {
    return TypeHintOptional::create(TypeHintPrimitive::create(tp_string), true, false);
  }

  if (const auto *as_scalar = c2php_scalar_type_hint(type->kind)) {
    return as_scalar;
  }

  switch (type->kind) {
    case FFITypeKind::Char:
      return TypeHintPrimitive::create(tp_string);

    case FFITypeKind::Pointer:
    case FFITypeKind::Array:
      return c_hint;

    case FFITypeKind::Struct:
    case FFITypeKind::StructDef:
      if (mode == Php2cMode::FieldRead) {
        if (const auto *as_instance = c_hint->try_as<TypeHintInstance>()) {
          return TypeHintInstance::create("&" + as_instance->full_class_name);
        }
        if (const auto *as_ffi = c_hint->try_as<TypeHintFFIType>()) {
          return TypeHintInstance::create("&" + cdata_class_name(as_ffi->scope_name, as_ffi->type->str));
        }
      }
      return c_hint;

    default:
      return nullptr;
  }
}

const TypeHint *FFIRoot::create_type_hint(const FFIType *type, const std::string &scope_name) {
  const auto *builtin_type = ffi_builtin_type(type->kind);
  if (builtin_type) {
    return TypeHintInstance::create(builtin_type->php_class_name);
  }
  switch (type->kind) {
    case FFITypeKind::Struct:
    case FFITypeKind::StructDef:
    case FFITypeKind::Union:
    case FFITypeKind::UnionDef:
      return TypeHintInstance::create(cdata_class_name(scope_name, type->str));

    case FFITypeKind::Pointer:
    case FFITypeKind::Array:
      return TypeHintFFIType::create(scope_name, type);

    case FFITypeKind::FunctionPointer: {
      const auto *return_type = unboxed_type_hint(type->members[0], scope_name, true);
      if (!return_type) {
        return nullptr;
      }
      std::vector<const TypeHint *> param_types;
      param_types.reserve(type->members.size() - 1);
      for (int i = 1; i < type->members.size(); i++) {
        kphp_assert(type->members[i]->kind == FFITypeKind::Var);
        const auto *param_type = unboxed_type_hint(type->members[i]->members[0], scope_name, false);
        if (!param_type) {
          return nullptr;
        }
        param_types.push_back(param_type);
      }
      return TypeHintCallable::create(std::move(param_types), return_type);
    }

    default:
      return nullptr;
  }
}

const TypeHint *FFIRoot::unboxed_type_hint(const FFIType *type, const std::string &scope_name, bool is_return) {
  // auto-convertible types, like int and float are kept unboxed
  if (!is_return && type->is_cstring()) {
    return TypeHintOptional::create(TypeHintPrimitive::create(tp_string), true, false);
  }
  if (const TypeHint *as_scalar = c2php_scalar_type_hint(type->kind)) {
    return as_scalar;
  }
  return create_type_hint(type, scope_name);
}

std::vector<FFIScopeDataMixin *> FFIRoot::get_scopes() const {
  std::vector<FFIScopeDataMixin *> result;
  result.reserve(scopes.size());
  for (const auto &it : scopes) {
    result.emplace_back(it.second);
  }
  std::sort(result.begin(), result.end(), [](const auto &x, const auto &y) { return x->scope_name < y->scope_name; });
  return result;
}

FFIScopeDataMixin *FFIRoot::find_scope(const std::string &scope_name) {
  return scopes.at(scope_name);
}

bool FFIRoot::register_scope(const std::string &scope_name, FFIScopeDataMixin *data) {
  std::lock_guard<std::mutex> locker{mutex};

  if (vk::contains(scopes, scope_name)) {
    return false;
  }
  scopes.emplace(scope_name, data);
  return true;
}

int FFIRoot::get_shared_lib_id(const std::string &path) {
  std::lock_guard<std::mutex> locker{mutex};

  const auto &it = std::find_if(shared_libs.begin(), shared_libs.end(), [&](const FFISharedLib &x) { return x.path == path; });
  if (it != shared_libs.end()) {
    return std::distance(shared_libs.begin(), it);
  }
  int id = shared_libs.size();
  shared_libs.emplace_back(FFISharedLib{id, path});
  return id;
}

const std::vector<FFISharedLib> &FFIRoot::get_shared_libs() const {
  return shared_libs;
}

void FFIRoot::bind_symbols() {
  int index = 0;
  for (auto &it : scopes) {
    auto &scope = it.second;
    for (auto &sym : scope->variables) {
      sym.env_index = index;
      index++;
    }
    for (auto &sym : scope->functions) {
      sym.env_index = index;
      index++;
    }
  }
  num_dynamic_symbols = index;
}

bool FFIRoot::is_ffi_scope_call(VertexAdaptor<op_func_call> call) {
  return call->extra_type == op_ex_func_call_arrow && call->func_id->class_id && call->func_id->class_id->is_ffi_scope();
}

struct TypePrinter {
  bool is_decltype = false;
  bool name_mangling = false;
  std::string scope_name;

  std::string format_list(std::vector<FFIType *>::const_iterator begin, std::vector<FFIType *>::const_iterator end, char sep = ',') const {
    std::string result;
    for (auto it = begin; it != end; ++it) {
      result += format_type(*it);
      if (std::next(it) != end) {
        result.push_back(sep);
        result.push_back(' ');
      }
    }
    return result;
  }

  std::string format_var(const FFIType *var) const {
    if (is_decltype) {
      return format_type(var->members[0]);
    }

    const FFIType *type = var->members[0];
    std::string name = var->str;
    std::string array_suffix;

    while (type->kind == FFITypeKind::Array) {
      if (type->num == -1) {
        array_suffix = "[]" + array_suffix;
      } else {
        array_suffix = "[" + std::to_string(type->num) + "]" + array_suffix;
      }
      type = type->members[0];
    }

    if (type->kind == FFITypeKind::FunctionPointer) {
      return format_function(type, name + array_suffix, true);
    }
    // a special case to avoid excessive empty space;
    // also: array types are not allowed when parameter name is omitted
    if (name.empty()) {
      return format_type(type);
    }
    return format_type(type) + " " + name + array_suffix;
  }

  std::string format_function(const FFIType *f, const std::string &func_name, bool is_func_ptr) const {
    std::string variadic_arg = f->is_variadic() ? ", ..." : "";
    const std::string &params = "(" + format_list(f->members.begin() + 1, f->members.end()) + variadic_arg + ")";
    if (is_func_ptr) {
      return format_type(f->members[0]) + " (*" + func_name + ")" + params;
    }
    const std::string &name = func_name.empty() ? "(*)" : func_name;
    return format_type(f->members[0]) + " " + name + params;
  }

  std::string format_struct_or_union_def(const FFIType *s, const std::string &tag) const {
    if (is_decltype) {
      return tag + " " + format_type_name(s->str);
    }
    return tag + " " + format_type_name(s->str) + "{ " + format_list(s->members.begin(), s->members.end(), ';') + "; }";
  }

  std::string format_type_name(const std::string &name) const {
    if (name_mangling) {
      return FFIRoot::c_name_mangle(scope_name, name);
    }
    return name;
  }

  std::string format_type(const FFIType *type) const {
    auto result = format_type_no_cv_qualifiers(type);
    if (type->flags & FFIType::Flag::Volatile) {
      result = "volatile " + result;
    }
    if (type->flags & FFIType::Flag::Const) {
      result = "const " + result;
    }
    return result;
  }

  std::string format_type_no_cv_qualifiers(const FFIType *type) const {
    if (const auto *builtin = ffi_builtin_type(type->kind)) {
      return builtin->c_name;
    }

    switch (type->kind) {
      case FFITypeKind::StructDef:
        return format_struct_or_union_def(type, "struct");
      case FFITypeKind::UnionDef:
        return format_struct_or_union_def(type, "union");

      case FFITypeKind::Union:
        return "union " + format_type_name(type->str);
      case FFITypeKind::Struct:
        return "struct " + format_type_name(type->str);
      case FFITypeKind::Enum:
        return "enum " + format_type_name(type->str);

      case FFITypeKind::Var:
        return format_var(type);
      case FFITypeKind::Array:
        if (type->num == -1) {
          return format_type(type->members[0]) + "[]";
        }
        return format_type(type->members[0]) + "[" + std::to_string(type->num) + "]";

      case FFITypeKind::Pointer:
        return type->members.empty() ? "*" : format_type(type->members[0]) + std::string(type->num, '*');

      case FFITypeKind::FunctionPointer:
        return format_function(type, "", true);

      case FFITypeKind::Function:
        return format_function(type, is_decltype ? "" : type->str, false);

      default:
        return "?";
    }
  }
};

std::string ffi_decltype_string(const FFIType *type) {
  TypePrinter p;
  p.is_decltype = true;
  return p.format_type(type);
}

std::string ffi_mangled_decltype_string(const std::string &scope_name, const FFIType *type) {
  TypePrinter p;
  p.is_decltype = true;
  p.name_mangling = true;
  p.scope_name = scope_name;
  return p.format_type(type);
}

std::string ffi_mangled_decltype_string(const TypeHint *type_hint) {
  return ffi_mangled_decltype_string(FFIRoot::get_ffi_scope(type_hint), FFIRoot::get_ffi_type(type_hint));
}

std::string ffi_type_string(const FFIType *type) {
  TypePrinter p;
  return p.format_type(type);
}

std::string ffi_mangled_type_string(const std::string &scope_name, const FFIType *type) {
  TypePrinter p;
  p.scope_name = scope_name;
  p.name_mangling = true;
  return p.format_type(type);
}
