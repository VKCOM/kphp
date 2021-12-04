// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/instantiate-ffi-operations.h"

#include "compiler/const-manipulations.h"
#include "compiler/ffi/ffi_parser.h"
#include "compiler/type-hint.h"

/*
 * This pass creates op_ffi_* vertices.
 * * FFI::new() and $cdef->new() are replaced with op_ffi_new
 * * FFI::cast() and $cdef->cast() are replaced with op_ffi_cast
 * * FFI::addr() is replaced with op_ffi_addr
 * * $cdef->module_function($arg): wrap args with op_ffi_php2c_conv, wrap return value with op_ffi_c2php_conv
 * * $cdata->prop and $cdef->prop are wrapped with op_ffi_c2php_conv
 * * $ffi_primitive->cdata is replaced with op_ffi_cdata_value_ref
 *
 * Up to this point of pipeline, instance->props are resolved, call->func_id are bound,
 * args have already been checked, variadics have already been wrapped into an array in a call.
 * Hence, we are sure that if call->func_id points to FFI::is_null, this call has exactly one argument, as declared.
 *
 * We can't express the @return of FFI::new() and some others in _functions.txt,
 * that's why their inferring is hardcoded: see infer_from_ffi_static_new() and others returning TypeHint*.
 * These functions are also used in assumptions, therefore we perform arg checks particularly there.
 */

// since FFI ops preprocessing happens after phpdoc parsing, we won't get "undefined class"
// errors for invalid FFI type strings that refer to some class;
// this functions doesn't cover all cases like T* and other FFI type hints,
// but it catches some simple errors to avoid compiler crashes;
// TODO: revisit this later, we need to consolidate undefined classes checking
static void check_class_defined(const TypeHint *type_hint) {
  if (const auto *as_instance = type_hint->try_as<TypeHintInstance>()) {
    kphp_error(as_instance->resolve(), fmt_format("Could not find class {}", TermStringFormat::paint_red(as_instance->full_class_name)));
  }
}

static ClassPtr get_cdata_class(FunctionPtr context, VertexPtr expr) {
  ClassPtr klass = assume_class_of_expr(context, expr, expr).try_as_class();
  if (klass && klass->is_ffi_cdata()) {
    return klass;
  }
  return {};
}

static bool is_ptr_field(ClassPtr klass, const std::string &field_name) {
  const auto *field = klass->get_instance_field(field_name);
  if (!field) {
    return false;
  }
  const auto *as_ffi = field->type_hint->try_as<TypeHintFFIType>();
  if (!as_ffi) {
    return false;
  }
  return as_ffi->type->kind == FFITypeKind::Pointer;
}

static bool contains_non_scalar_types(const FFIType *type) {
  if (vk::any_of_equal(type->kind, FFITypeKind::Struct, FFITypeKind::Union)) {
    return true;
  }
  return std::any_of(type->members.begin(), type->members.end(), [](const FFIType *member) {
    return contains_non_scalar_types(member);
  });
}

static inline VertexPtr unwrap_if_c2php_conv(VertexPtr v) {
  return v->type() == op_ffi_c2php_conv         // unwrapping might be useful for outermost ast roots,
         ? v.as<op_ffi_c2php_conv>()->expr()    // since op_ffi_c2php_conv is created in on_exit_vertex()
         : v;
}

// internal handling FFI::cast('int', $cdata) and $cdef->cast('struct A', $cdata): calc the @return of this operation
static const TypeHint *ffi_cast_impl(const FFIType *type, ClassPtr cdata_class, ClassPtr scope_class) {
  // FFI::cast returns a reference-like non-owning data, so a special care
  // should be taken here
  if (const auto *to_builtin = ffi_builtin_type(type->kind)) {
    const auto *from_builtin = ffi_builtin_type(cdata_class->ffi_class_mixin->ffi_type->kind);
    // it could potentially be safe to perform some struct->scalar conversions,
    // but we're forbidding them for now
    kphp_error_act(from_builtin,
                   fmt_format("ffi casting a non-scalar type {} to a scalar type {}", cdata_class->name, to_builtin->c_name), return nullptr);
    return TypeHintInstance::create("&" + to_builtin->php_class_name);
  }

  if (type->kind == FFITypeKind::Struct && scope_class) {
    return TypeHintInstance::create("&" + FFIRoot::cdata_class_name(scope_class->ffi_scope_mixin->scope_name, type->str));
  }

  const TypeHint *php_type = G->get_ffi_root().create_type_hint(type, !scope_class ? "C" : scope_class->ffi_scope_mixin->scope_name);
  check_class_defined(php_type);
  return php_type;
}

// todo some time later, drop the --enable-ffi flag, enable it always
bool InstantiateFFIOperationsPass::check_function(FunctionPtr f __attribute__((unused))) const {
  return G->settings().ffi_enabled.get();
}

// on FFI::new(), replace it with op_ffi_new
VertexPtr InstantiateFFIOperationsPass::on_ffi_static_new(VertexAdaptor<op_func_call> call) {
  auto new_op = VertexAdaptor<op_ffi_new>::create().set_location(call);
  new_op->php_type = infer_from_ffi_static_new(call);
  return new_op;
}

// on $cdef->new(), replace it with op_ffi_new
VertexPtr InstantiateFFIOperationsPass::on_ffi_scope_new(VertexAdaptor<op_func_call> call, ClassPtr scope_class) {
  auto new_op = VertexAdaptor<op_ffi_new>::create().set_location(call);
  new_op->php_type = infer_from_ffi_scope_new(call, scope_class);
  return new_op;
}

// on FFI::addr(), replace it with op_ffi_addr
VertexPtr InstantiateFFIOperationsPass::on_ffi_static_addr(VertexAdaptor<op_func_call> call) {
  // FFI::addr(c2php($x)) -> FFI::addr($x)
  auto arg = unwrap_if_c2php_conv(call->args()[0]);
  return VertexAdaptor<op_ffi_addr>::create(arg).set_location(call);
}

// on FFI::is_null(), unwrap a call argument
VertexPtr InstantiateFFIOperationsPass::on_ffi_static_is_null(VertexAdaptor<op_func_call> call) {
  // FFI::is_null(c2php($x)) -> FFI::is_null($x)
  call->args()[0] = unwrap_if_c2php_conv(call->args()[0]);
  return call;
}

// on FFI::cast(), replace it with op_ffi_cast
VertexPtr InstantiateFFIOperationsPass::on_ffi_static_cast(VertexAdaptor<op_func_call> call) {
  const TypeHint *php_type = infer_from_ffi_static_cast(current_function, call);
  if (!php_type) {
    return call;
  }

  // FFI::cast(c2php($x)) -> FFI::cast($x)
  auto arg = unwrap_if_c2php_conv(call->args()[1]);
  auto cast_op = VertexAdaptor<op_ffi_cast>::create(arg).set_location(call);
  cast_op->php_type = php_type;
  return cast_op;
}

// on $cdef->cast(), replace it with op_ffi_cast
// real-world example: $cdef2->cast('struct Foo*', FFI::addr($single_int))
VertexPtr InstantiateFFIOperationsPass::on_ffi_scope_cast(VertexAdaptor<op_func_call> call, ClassPtr scope_class) {
  const TypeHint *php_type = infer_from_ffi_scope_cast(current_function, call, scope_class);
  if (!php_type) {
    return call;
  }

  // $cdef->cast(c2php($x)) -> FFI::cast($x)
  auto arg = unwrap_if_c2php_conv(call->args()[2]);
  auto cast_op = VertexAdaptor<op_ffi_cast>::create(arg).set_location(call);
  cast_op->php_type = php_type;
  return cast_op;
}

// on $cdef->custom_module_function(), insert php2c conversions for arguments and c2php for a return value
VertexPtr InstantiateFFIOperationsPass::on_ffi_scope_call_custom(VertexAdaptor<op_func_call> call) {
  FunctionPtr called_func = call->func_id;

  for (int i = 1; i < call->args().size(); i++) {   // 0 is implicit $this
    auto arg = call->args()[i];
    auto func_param = called_func->get_params()[i].as<op_func_param>();

    // last param of a variadic function: wrap each item of arg (it's op_array) into php2c conv
    if (called_func->has_variadic_param && i == called_func->get_params().size() - 1) {
      kphp_assert(arg->type() == op_array);
      for (auto &array_item: *arg) {
        if (array_item->type() == op_ffi_c2php_conv) {
          array_item = unwrap_if_c2php_conv(array_item);
        } else {
          auto php2c_conv = VertexAdaptor<op_ffi_php2c_conv>::create(array_item).set_location(arg);
          php2c_conv->c_type = func_param->type_hint->try_as<TypeHintArray>()->inner;
          array_item = php2c_conv;
        }
      }
      continue;
    }

    // if it's c2php() already, we don't want to insert php2c() for this
    // argument as it would be a (potentially expensive) no-op php2c(c2php(arg))
    if (arg->type() == op_ffi_c2php_conv) {
      call->args()[i] = unwrap_if_c2php_conv(arg);
      continue;
    }

    auto php2c_conv = VertexAdaptor<op_ffi_php2c_conv>::create(arg).set_location(arg);
    php2c_conv->c_type = func_param->type_hint;
    call->args()[i] = php2c_conv;
  }

  const auto *php_type = G->get_ffi_root().c2php_return_type_hint(called_func->return_typehint);
  kphp_error_act(php_type, fmt_format("Unsupported c2php type: {}", called_func->return_typehint->as_human_readable()), return call);

  auto conv = VertexAdaptor<op_ffi_c2php_conv>::create(call).set_location(call);
  conv->php_type = php_type;
  return conv;
}

// on $cdata->struct_prop, wrap it with c2php conv (struct_prop is still a valid class field and has VarPtr)
// on $cdata->cdata, replace it with op_ffi_cdata_value_ref
VertexPtr InstantiateFFIOperationsPass::on_cdata_instance_prop(ClassPtr root_class, VertexAdaptor<op_instance_prop> root) {
  // traverse the instance() to mark all nested instance_op with appropriate access_type
  // so that we don't try to insert conversions there as well
  VertexAdaptor<op_instance_prop> current = root;
  while (true) {
    auto next = current->instance().try_as<op_instance_prop>();
    if (!next) {
      // end of the chain
      current->access_type = InstancePropAccessType::CData;
      break;
    }
    ClassPtr cdata_class = get_cdata_class(current_function, next->instance());
    if (!cdata_class) {
      // left side instance is not FFI value, can't access directly
      current->access_type = InstancePropAccessType::CData;
      break;
    }
    current->access_type = is_ptr_field(cdata_class, next->get_string())
                           ? InstancePropAccessType::CDataDirectPtr
                           : InstancePropAccessType::CDataDirect;
    current = next;
  }

  // for now, we only support $var->cdata; PHP allows nested cdata write, but not read
  //   $struct->field->cdata = $value;  // OK in PHP
  //   var_dump($struct->field->cdata); // Error in PHP
  // both ->cdata forms give compile-time error in KPHP as this style of access is redundant
  //   $struct->field = $value;  // works in both PHP and KPHP
  //   var_dump($struct->field); // works in both PHP and KPHP
  if (root->str_val == "cdata" && root->instance()->type() == op_var) {
    const auto *php_type = G->get_ffi_root().c2php_scalar_type_hint(root_class);
    if (php_type) {
      auto getref = VertexAdaptor<op_ffi_cdata_value_ref>::create(root->instance()).set_location(root);
      auto conv = VertexAdaptor<op_ffi_c2php_conv>::create(getref).set_location(getref);
      conv->php_type = php_type;
      return conv;
    }
  }
  kphp_error_act(root->str_val != "cdata",  // non-var lhs
                 fmt_format("Field $cdata not found in class {}", root_class->as_human_readable()), return root);

  const auto *field = root_class->get_instance_field(root->get_string());
  kphp_assert(field && field->type_hint);
  const auto *php_type = G->get_ffi_root().c2php_field_type_hint(field->type_hint);
  kphp_error_act(php_type, fmt_format("Unsupported c2php type: {}", field->type_hint->as_human_readable()), return root);

  auto conv = VertexAdaptor<op_ffi_c2php_conv>::create(root).set_location(root);
  conv->php_type = php_type;
  return conv;
}

// on $cdef->some_prop, wrap it with c2php conversion
VertexPtr InstantiateFFIOperationsPass::on_scope_instance_prop(ClassPtr scope_class, VertexAdaptor<op_instance_prop> root) {
  auto enum_constant = scope_class->ffi_scope_mixin->enum_constants.find(root->get_string());
  if (enum_constant != scope_class->ffi_scope_mixin->enum_constants.end()) {
    return GenTree::create_int_const(enum_constant->second);
  }

  root->access_type = InstancePropAccessType::ExternVar;

  const auto *field = scope_class->get_instance_field(root->get_string());
  kphp_assert(field && field->type_hint);
  const auto *php_type = G->get_ffi_root().c2php_field_type_hint(field->type_hint);
  kphp_error_act(php_type, fmt_format("Unsupported c2php type: {}", field->type_hint->as_human_readable()), return root);

  auto conv = VertexAdaptor<op_ffi_c2php_conv>::create(root).set_location(root);
  conv->php_type = php_type;
  return conv;
}

// infer @return for FFI::new()
const TypeHint *InstantiateFFIOperationsPass::infer_from_ffi_static_new(VertexAdaptor<op_func_call> call) {
  kphp_error_act(call->args().size() == 1,
                 "FFI::new() expects one non-empty const string argument", return nullptr);

  const std::string &type_expr = collect_string_concatenation(call->args()[0]);
  kphp_error_act(!type_expr.empty(),
                 "FFI::new() expects one non-empty const string argument", return nullptr);

  FFITypedefs typedefs;
  auto[type, err] = ffi_parse_type(type_expr, typedefs);
  kphp_error_act(err.message.empty(),
                 fmt_format("FFI::new(): line {}: {}", err.line, err.message), return nullptr);
  kphp_error_act(!contains_non_scalar_types(type),
                 "static FFI::new() can only create scalar types", return nullptr);

  return G->get_ffi_root().create_type_hint(type, "C");
}

// infer @return for $cdef->new()
const TypeHint *InstantiateFFIOperationsPass::infer_from_ffi_scope_new(VertexAdaptor<op_func_call> call, ClassPtr scope_class) {
  kphp_error_act(call->args().size() == 2,  // first if implicit $this
                 "ffi->new() expects a non-empty const string argument", return nullptr);

  const std::string &type_expr = collect_string_concatenation(call->args()[1]);
  kphp_error_act(!type_expr.empty(),
                 "ffi->new() expects a non-empty const string argument", return nullptr);

  auto[type, err] = ffi_parse_type(type_expr, scope_class->ffi_scope_mixin->typedefs);
  kphp_error_act(err.message.empty(),
                 fmt_format("ffi->new(): line {}: {}", err.line, err.message), return nullptr);

  const TypeHint *php_type = G->get_ffi_root().create_type_hint(type, scope_class->ffi_scope_mixin->scope_name);
  kphp_error(php_type, fmt_format("ffi->new(): failed to infer a PHP type hint for {} type", type->str));
  return php_type;
}

// infer @return for FFI::cast()
const TypeHint *InstantiateFFIOperationsPass::infer_from_ffi_static_cast(FunctionPtr f, VertexAdaptor<op_func_call> call) {
  kphp_error_act(call->args().size() == 2,
                 "FFI::cast() expects two arguments", return nullptr);

  const std::string &type_expr = collect_string_concatenation(call->args()[0]);
  kphp_error_act(!type_expr.empty(),
                 "FFI::cast() expects a non-empty const string first argument", return nullptr);

  FFITypedefs typedefs;
  auto[type, err] = ffi_parse_type(type_expr, typedefs);
  kphp_error_act(err.message.empty(),
                 fmt_format("FFI::cast(): line {}: {}", err.line, err.message), return nullptr);

  ClassPtr cdata_class = get_cdata_class(f, call->args()[1]);
  if (!cdata_class) {
    return nullptr;
  }

  return ffi_cast_impl(type, cdata_class, ClassPtr{});
}

// infer @return for $cdef->cast()
const TypeHint *InstantiateFFIOperationsPass::infer_from_ffi_scope_cast(FunctionPtr f, VertexAdaptor<op_func_call> call, ClassPtr scope_class) {
  kphp_error_act(call->args().size() == 3,  // first is implicit $this
                 "ffi->cast() expects two arguments", return nullptr);

  const std::string &type_expr = collect_string_concatenation(call->args()[1]);
  kphp_error_act(!type_expr.empty(),
                 "ffi->cast() expects a non-empty const string first argument", return nullptr);

  auto[type, err] = ffi_parse_type(type_expr, scope_class->ffi_scope_mixin->typedefs);
  kphp_error_act(err.message.empty(),
                 fmt_format("ffi->cast(): line {}: {}", err.line, err.message), return nullptr);

  ClassPtr cdata_class = get_cdata_class(f, call->args()[2]);
  if (!cdata_class) {
    return nullptr;
  }

  return ffi_cast_impl(type, cdata_class, scope_class);
}


VertexPtr InstantiateFFIOperationsPass::on_enter_vertex(VertexPtr root) {
  if (auto instance_prop = root.try_as<op_instance_prop>()) {
    if (instance_prop->access_type != InstancePropAccessType::Default) {
      // not a first time we touch this instance_prop, skip it
      // as it was processed through its parent
      return root;
    }

    if (instance_prop->class_id->is_ffi_scope()) {
      return on_scope_instance_prop(instance_prop->class_id, instance_prop);
    }
    if (instance_prop->class_id->is_ffi_cdata()) {
      return on_cdata_instance_prop(instance_prop->class_id, instance_prop);
    }
  }

  return root;
}

VertexPtr InstantiateFFIOperationsPass::on_exit_vertex(VertexPtr root) {
  if (auto call = root.try_as<op_func_call>()) {
    FunctionPtr f_called = call->func_id;

    if (call->extra_type == op_ex_func_call_arrow && f_called->class_id->name == "FFI\\Scope") {
      ClassPtr klass = assume_class_of_expr(current_function, call->args()[0], call).try_as_class();
      if (f_called->name == "FFI$Scope$$new") {
        return on_ffi_scope_new(call, klass);
      }
      if (f_called->name == "FFI$Scope$$cast") {
        return on_ffi_scope_cast(call, klass);
      }

    } else if (call->extra_type == op_ex_func_call_arrow && f_called->class_id->is_ffi_scope()) {
      return on_ffi_scope_call_custom(call);

    } else if (call->extra_type != op_ex_func_call_arrow && f_called->is_extern()) {
      if (f_called->name == "FFI$$new") {
        return on_ffi_static_new(call);
      }
      if (f_called->name == "FFI$$addr") {
        return on_ffi_static_addr(call);
      }
      if (f_called->name == "FFI$$cast") {
        return on_ffi_static_cast(call);
      }
      if (f_called->name == "FFI$$isNull") {
        return on_ffi_static_is_null(call);
      }

      // count(c2php($x)) -> count($x)
      if (f_called->name == "count" && call->args()[0]->type() == op_ffi_c2php_conv) {
        call->args()[0] = unwrap_if_c2php_conv(call->args()[0]);
      }

      // some CData-like types are not boxed, but we want get_class()
      // to work in the same way as in PHP; so we fold all
      // get_class() over cdata types to the result we expect
      if (f_called->name == "get_class" && call->args().size() == 1) {
        if (get_cdata_class(current_function, call->args()[0])) {
          auto class_name = VertexAdaptor<op_string>::create().set_location(call);
          class_name->str_val = "FFI\\CData";
          return class_name;
        }
      }
    }
  }

  // we don't really support list assignments into cdata,
  // but we remove excessive c2php conv vertices to give better error messages
  if (auto list = root.try_as<op_list>()) {
    for (auto x : list->list()) {
      auto kv = x.as<op_list_keyval>();
      if (auto c2php_conv = kv->var().try_as<op_ffi_c2php_conv>()) {
        kv->var() = c2php_conv->expr();
      }
    }
  }

  if (auto assign = root.try_as<op_set_modify>()) {
    if (auto c2php_conv = assign->lhs().try_as<op_ffi_c2php_conv>()) {
      auto lhs = c2php_conv->expr();

      if (auto rhs_c2php_conv = assign->rhs().try_as<op_ffi_c2php_conv>()) {
        assign->rhs() = rhs_c2php_conv->expr();
      } else {
        auto php2c_conv = VertexAdaptor<op_ffi_php2c_conv>::create(assign->rhs()).set_location(assign->rhs());
        const TypeHint *c_type = assume_class_of_expr(current_function, lhs, assign).assum_hint;
        php2c_conv->c_type = c_type;
        php2c_conv->simple_dst = lhs->type() == op_var;
        assign->rhs() = php2c_conv;
      }
      assign->lhs() = lhs;
      return assign;
    }
  }

  return root;
}
