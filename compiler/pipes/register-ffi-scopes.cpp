// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/register-ffi-scopes.h"

#include "compiler/const-manipulations.h"
#include "compiler/data/ffi-data.h"
#include "compiler/data/src-file.h"
#include "compiler/ffi/ffi_parser.h"
#include "compiler/name-gen.h"
#include "compiler/type-hint.h"
#include "compiler/vertex-util.h"

#include <fstream>

class RegisterFFIScopes final : public FunctionPassBase {
private:
  DataStream<FunctionPtr>& os;

  static bool register_scope(FFIScopeDataMixin* scope, const FFIParseResult& result) {
    for (const FFIType* type : result) {
      if (type->kind == FFITypeKind::Var) {
        scope->variables.emplace_back(type);
      } else if (type->kind == FFITypeKind::Function) {
        scope->functions.emplace_back(type);
      } else if (vk::any_of_equal(type->kind, FFITypeKind::StructDef, FFITypeKind::UnionDef)) {
        scope->types.emplace_back(type);
      }
    }

    std::sort(scope->variables.begin(), scope->variables.end());
    std::sort(scope->functions.begin(), scope->functions.end());
    // scope->types should not be sorted as their definitions
    // may depend on each other; funcs and vars are always extern,
    // so they can't really refer to anything except types

    return G->get_ffi_root().register_scope(result.scope, scope);
  }

  void register_class(ClassPtr klass) {
    G->register_class(klass);

    auto class_holder = klass->gen_holder_function(klass->name);
    G->register_and_require_function(class_holder, os);
    os << class_holder;
  }

  static void add_extern_var(VertexAdaptor<op_func_call> call, ClassPtr scope_class, const FFIType* type, const FFIParseResult& result) {
    const TypeHint* type_hint = G->get_ffi_root().create_type_hint(type->members[0], result.scope);
    kphp_error_return(type_hint, fmt_format("unsupported C variable type: {}", ffi_decltype_string(type->members[0])));
    auto var = VertexAdaptor<op_var>::create().set_location(call);
    var->str_val = type->str;
    scope_class->members.add_instance_field(var, {}, FieldModifiers{}.set_public(), nullptr, type_hint);
  }

  void add_function(VertexAdaptor<op_func_call> call, ClassPtr scope_class, const FFIType* type, const FFIParseResult& result) {
    int num_params = type->members.size() - 1; // [0] stores return type, it's not a param
    std::vector<VertexAdaptor<op_func_param>> params;
    params.reserve(1 + num_params); // +1 due to $this param
    params.emplace_back(ClassData::gen_param_this(call->location));
    for (int i = 1; i < type->members.size(); i++) {
      const FFIType* ffi_type = type->members[i];
      auto var = VertexAdaptor<op_var>::create().set_location(call->location);
      auto param = VertexAdaptor<op_func_param>::create(var).set_location(call->location);
      kphp_assert(ffi_type->kind == FFITypeKind::Var);
      var->str_val = ffi_type->str;
      if (var->str_val.empty()) {
        // we need to generate some name for unnamed parameter to avoid
        // broken type assumptions (see issue #424)
        var->str_val = "_unnamed_arg" + std::to_string(i);
      }
      if (ffi_type->members[0]->kind == FFITypeKind::FunctionPointer) {
        kphp_error_return(!type->is_signal_safe(), fmt_format("{}: @kphp-ffi-signalsafe should not be used with FFI callbacks", type->str));
      }
      param->type_hint = G->get_ffi_root().create_type_hint(ffi_type->members[0], result.scope);
      kphp_error_return(param->type_hint, fmt_format("{}: unsupported C param type: {}", type->str, ffi_decltype_string(ffi_type)));
      params.emplace_back(param);
    }
    if (type->is_variadic()) {
      auto var = VertexAdaptor<op_var>::create().set_location(call->location);
      auto param = VertexAdaptor<op_func_param>::create(var).set_location(call->location);
      var->str_val = "_variadic_arg";
      param->type_hint = TypeHintArray::create(TypeHintInstance::create("FFI\\CData"));
      params.emplace_back(param);
    }

    auto param_list = VertexAdaptor<op_func_param_list>::create(params).set_location(call);
    auto func_root = VertexAdaptor<op_function>::create(param_list, VertexAdaptor<op_seq>::create().set_location(call));
    auto ffi_func = FunctionData::create_function(replace_backslashes(scope_class->name) + "$$" + type->str, func_root, FunctionData::func_extern);
    ffi_func->has_variadic_param = type->is_variadic();
    ffi_func->modifiers.set_instance();
    ffi_func->modifiers.set_public();
    ffi_func->return_typehint = G->get_ffi_root().create_type_hint(type->members[0], result.scope);
    kphp_error_return(ffi_func->return_typehint, fmt_format("{}: unsupported C return type: {}", type->str, ffi_type_string(type->members[0])));

    scope_class->members.add_instance_method(ffi_func);
    G->register_and_require_function(ffi_func, os);
    os << ffi_func;
  }

  void add_struct_or_union(VertexAdaptor<op_func_call> call, const FFIType* type, const FFIParseResult& result) {
    bool is_struct = type->kind == FFITypeKind::StructDef;

    auto cdata_class = ClassPtr{new ClassData{ClassType::ffi_cdata}};
    cdata_class->ffi_class_mixin = new FFIClassDataMixin{type, result.scope};
    cdata_class->set_name_and_src_name(FFIRoot::cdata_class_name(result.scope, type->str));
    cdata_class->src_name = "C$FFI$CData<" + ffi_mangled_decltype_string(result.scope, type) + ">";
    cdata_class->add_str_dependent(current_function, ClassType::klass, "\\FFI\\CData");

    auto cdata_class_ref = ClassPtr{new ClassData{ClassType::ffi_cdata}};
    cdata_class_ref->ffi_class_mixin = new FFIClassDataMixin{type, result.scope, cdata_class};
    cdata_class_ref->set_name_and_src_name("&" + cdata_class->name);
    cdata_class_ref->src_name = "CDataRef<" + ffi_mangled_decltype_string(result.scope, type) + ">";
    cdata_class_ref->add_str_dependent(current_function, ClassType::klass, "\\FFI\\CData");

    for (const FFIType* field : type->members) {
      auto var = VertexAdaptor<op_var>::create().set_location(call);
      var->set_string(field->str);
      VertexPtr default_value;
      const TypeHint* type_hint = G->get_ffi_root().create_type_hint(field->members[0], result.scope);
      kphp_error_return(type_hint, fmt_format("unsupported C {} field type: {}", is_struct ? "struct" : "union", ffi_decltype_string(field->members[0])));
      cdata_class->members.add_instance_field(var, default_value, FieldModifiers{}.set_public(), nullptr, type_hint);
      cdata_class_ref->members.add_instance_field(var, default_value, FieldModifiers{}.set_public(), nullptr, type_hint);
    }

    register_class(cdata_class);
    register_class(cdata_class_ref);
  }

  void add_enum_constant(const std::string& const_name, int const_value, ClassPtr scope_class) {
    auto fake_op_var = VertexAdaptor<op_var>::create();
    fake_op_var->str_val = const_name;
    auto fake_def_val = VertexUtil::create_int_const(const_value);

    // in PHP/KPHP, we access enum constants via an arrow: $cdef->CONST
    // so, make it be an instance field, not a static const; it'll pass all checks of fields existence
    // later on, when ffi operations are processed, they will be inlined: see InstantiateFFIOperationsPass
    scope_class->members.add_instance_field(fake_op_var, fake_def_val, FieldModifiers{}.set_public(), nullptr, nullptr);
  }

  VertexPtr make_ffi_load_call(VertexAdaptor<op_func_call> call, FFIScopeDataMixin* scope, const FFIParseResult& result) {
    kphp_error_act(register_scope(scope, result), fmt_format("duplicate definition of `{}` scope", result.scope), return call);

    auto scope_class = ClassPtr{new ClassData{ClassType::ffi_scope}};
    scope_class->ffi_scope_mixin = scope;
    scope_class->set_name_and_src_name(FFIRoot::scope_class_name(result.scope));
    scope_class->src_name = "C$FFI$Scope";
    scope_class->file_id = current_function->file_id;
    scope_class->add_str_dependent(current_function, ClassType::klass, "\\FFI\\Scope");

    scope->location = call->location;

    for (const FFIType* type : result) {
      if (type->kind == FFITypeKind::Var) {
        add_extern_var(call, scope_class, type, result);
      } else if (type->kind == FFITypeKind::Function) {
        add_function(call, scope_class, type, result);
      } else if (vk::any_of_equal(type->kind, FFITypeKind::StructDef, FFITypeKind::UnionDef)) {
        add_struct_or_union(call, type, result);
      }
    }
    for (const auto& enum_item : result.enum_constants) {
      add_enum_constant(enum_item.first, enum_item.second, scope_class);
    }

    register_class(scope_class);

    auto ffi_load_call = VertexAdaptor<op_ffi_load_call>::create(call).set_location(call);
    ffi_load_call->scope_name = result.scope;
    return ffi_load_call;
  }

  VertexPtr on_ffi_cdef(VertexAdaptor<op_func_call> call) {
    if (call->args().empty()) {
      return call;
    }
    const std::string& code = collect_string_concatenation(call->args()[0]);
    kphp_error_act(!code.empty(), "FFI::cdef() $code param expects a non-empty const string argument", return call);

    // we rewrite FFI::cdef('...') to FFI::load('') call and re-use the same code path as with FFI::load()
    auto fake_filename_vertex = VertexAdaptor<op_string>::create().set_location(call);
    auto fake_load_call = VertexAdaptor<op_func_call>::create(fake_filename_vertex).set_location(call);
    fake_load_call->set_string("FFI$$load");

    FFIRoot& ffi = G->get_ffi_root();

    int shared_lib_id = -1;
    if (call->args().size() > 1 && call->args()[1]->type() != op_null) {
      const std::string& lib = collect_string_concatenation(call->args()[1]);
      kphp_error_act(!lib.empty(), "FFI::cdef() $lib param expects a non-empty const string argument or null", return call);
      shared_lib_id = ffi.get_shared_lib_id(lib);
    }

    auto* scope = new FFIScopeDataMixin{};
    FFIParseResult result = ffi_parse_file(code, scope->typedefs);
    kphp_error_act(result.err.message.empty(), fmt_format("FFI::cdef(): line {}: {}", result.err.line, result.err.message), return call);
    if (result.scope.empty()) {
      result.scope = gen_anonymous_scope_name(current_function);
    }
    scope->enum_constants = result.enum_constants;
    scope->scope_name = result.scope;
    scope->shared_lib_id = shared_lib_id;
    auto ffi_load_call = make_ffi_load_call(fake_load_call, scope, result);

    auto scope_name_vertex = VertexAdaptor<op_string>::create();
    scope_name_vertex->set_string(result.scope);
    auto ffi_scope = VertexAdaptor<op_func_call>::create(scope_name_vertex).set_location(call);
    ffi_scope->set_string("FFI$$scope");

    return VertexAdaptor<op_seq_rval>::create(ffi_load_call, ffi_scope).set_location(call);
  }

  static std::string read_file(const std::string& filename) {
    std::ifstream file{filename};
    if (!file.is_open()) {
      return "";
    }

    std::string file_contents;
    file.seekg(0, std::ios::end);
    file_contents.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    file_contents.assign(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});

    return file_contents;
  }

  VertexPtr on_ffi_load(VertexAdaptor<op_func_call> call) {
    if (call->args().size() != 1) {
      return call;
    }
    const std::string& filename = collect_string_concatenation(call->args()[0]);
    kphp_error_act(!filename.empty(), "FFI::load() $filename param expects a non-empty const string argument", return call);

    std::string file_contents = read_file(filename);
    kphp_error_act(!file_contents.empty(), fmt_format("FFI::load() can't read {}", filename), return call);

    auto* scope = new FFIScopeDataMixin{};
    FFIParseResult result = ffi_parse_file(file_contents, scope->typedefs);
    kphp_error_act(result.err.message.empty(), fmt_format("FFI::load(): line {}: {}", result.err.line, result.err.message), return call);
    scope->scope_name = result.scope;
    scope->enum_constants = result.enum_constants;

    kphp_error_act(result.static_lib.empty() || result.lib.empty(), "can't use both FFI_LIB and FFI_STATIC_LIB at the same time", return call);

    FFIRoot& ffi = G->get_ffi_root();

    int shared_lib_id = -1;
    if (!result.lib.empty()) {
      shared_lib_id = ffi.get_shared_lib_id(result.lib);
    }

    scope->shared_lib_id = shared_lib_id;
    return make_ffi_load_call(call, scope, result);
  }

public:
  explicit RegisterFFIScopes(DataStream<FunctionPtr>& os)
      : os{os} {};

  std::string get_description() override {
    return "Register FFI scopes";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override;
};

VertexPtr RegisterFFIScopes::on_exit_vertex(VertexPtr root) {
  if (root->type() != op_func_call || root->extra_type == op_ex_func_call_arrow) {
    return root;
  }

  // at this step of pipeline, we don't have :: calls resolved, so do a brute search for FFI calls declaring scopes
  // we don't support "use FFI as a" and calling a::cdef
  auto call = root.as<op_func_call>();
  if (call->str_val.size() < 11 && call->str_val[4] == ':') {
    if (call->str_val == "FFI::load" || call->str_val == "\\FFI::load") {
      return on_ffi_load(call);
    }
    if (call->str_val == "FFI::cdef" || call->str_val == "\\FFI::cdef") {
      return on_ffi_cdef(call);
    }
  }

  return root;
}

void RegisterFFIScopesF::execute(FunctionPtr f, DataStream<FunctionPtr>& os) {
  RegisterFFIScopes pass{os};
  pass.setup(f);
  run_function_pass(f->root, &pass);
  os << f;
}
