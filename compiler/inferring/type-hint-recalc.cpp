// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/type-hint.h"

#include "compiler/data/function-data.h"
#include "compiler/inferring/public.h"
#include "compiler/vertex-util.h"

// recalc_type_data_in_context_of_call() from type-hint.h for all ancestors
// they are implemented in a separate .cpp file to leave type-hint.cpp more clear, as recalculation is a separate inferring part

void TypeHintArgRef::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  if (auto vertex = VertexUtil::get_call_arg_ref(arg_num, call)) {
    dst->set_lca(vertex->tinf_node.get_type());
  }
}

void TypeHintArgRefCallbackCall::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  static auto prevent_recursion_thread_safe = [](const std::function<void(std::vector<std::string>&)>& callback) {
    static std::mutex mutex;
    static std::vector<std::string> recursively_recalculated;
    std::lock_guard<std::mutex> guard{mutex};
    callback(recursively_recalculated);
  };

  auto called_function = call.as<op_func_call>()->func_id;
  auto callback_param = called_function->get_params()[arg_num - 1].as<op_func_param>();
  VertexRange call_args = call.as<op_func_call>()->args();
  FunctionPtr provided_callback = call_args[arg_num - 1].as<op_callback_of_builtin>()->func_id;

  // here we deal with an uncommon, but hard situation: array_map("array_filter", $arr)
  // we have call (array_map) with this ^1() type hint, and provided_callback (array_filter) has ^1 in return type
  // so, return type of callback isn't constexpr, and we need to emulate that callback invocation:
  // create a fake inner call: array_map(function(...$args) { return array_filter(...$args); }, $arr)
  if (provided_callback->return_typehint && provided_callback->return_typehint->has_argref_inside()) {
    bool in_recursion = false;
    prevent_recursion_thread_safe([provided_callback, &in_recursion](std::vector<std::string>& recursion) {
      in_recursion = std::find(recursion.begin(), recursion.end(), provided_callback->name) != recursion.end();
    });
    if (in_recursion) { // see array_reduce(): when we calc ^2() as a return, stop calculating ^2() as a callback parameter
      return;
    }

    std::vector<VertexPtr> fake_call_params; // e.g., only one here for fake array_filter call: ^2[*]
    for (const auto* callback_param_hint : callback_param->type_hint->try_as<TypeHintCallable>()->arg_types) {
      prevent_recursion_thread_safe([provided_callback](std::vector<std::string>& recursion) { recursion.emplace_back(provided_callback->name); });

      auto fake_call_param = VertexAdaptor<op_none>::create();        // it has no contents, only tinf_node
      TypeData* type_of_passed = TypeData::get_type(tp_any)->clone(); // type of passed ^2[*]
      callback_param_hint->recalc_type_data_in_context_of_call(type_of_passed, call);
      fake_call_param->tinf_node.set_type(type_of_passed);
      fake_call_params.emplace_back(fake_call_param);

      prevent_recursion_thread_safe([provided_callback](std::vector<std::string>& recursion) {
        recursion.erase(std::find(recursion.begin(), recursion.end(), provided_callback->name));
      });
    }
    auto fake_func_call = VertexAdaptor<op_func_call>::create(fake_call_params);
    fake_func_call->func_id = provided_callback;
    provided_callback->return_typehint->recalc_type_data_in_context_of_call(dst, fake_func_call);

  } else {
    // just a regular array_map(function(){...}, $arr) or array_map('trim', $arr)
    // then a type of this ^1() is just what provided_callback returns, it's a constexpr type
    dst->set_lca(tinf::get_type(provided_callback, -1));
  }
}

void TypeHintArgRefInstance::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  if (auto v = VertexUtil::get_call_arg_ref(arg_num, call)) {
    if (const auto* class_name = VertexUtil::get_constexpr_string(v)) {
      if (auto klass = G->get_class(*class_name)) {
        dst->set_lca(klass->type_data);
        return;
      }
      kphp_error(0, fmt_format("Can't find class {}", *class_name));
    }
  }

  kphp_error(0, fmt_format("Can't find class: argument #{} is not a const string", arg_num));
  dst->set_lca(TypeData::get_type(tp_Error));
}

void TypeHintArgSubkeyGet::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  TypeData nested(*TypeData::get_type(tp_any));
  inner->recalc_type_data_in_context_of_call(&nested, call);
  dst->set_lca(nested.const_read_at(MultiKey::any_key(1)));
}

void TypeHintArray::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  dst->set_lca(TypeData::get_type(tp_array));
  TypeData nested(*TypeData::get_type(tp_any));
  inner->recalc_type_data_in_context_of_call(&nested, call);
  dst->set_lca_at(MultiKey::any_key(1), &nested);
}

void TypeHintCallable::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  if (is_typed_callable() && (!call || !call.as<op_func_call>()->func_id->is_extern())) {
    dst->set_lca(get_interface()->type_data);
  } else if (is_untyped_callable() && f_bound_to) {
    dst->set_lca(get_lambda_class()->type_data);
  } else {
    // the 'callable' keyword used in phpdoc/type declaration doesn't affect the type inference
    // if used in param, like function f(callable $cb), f was auto-converted to generics, and 'callable' was dropped away
    // this branch is reachable when 'callable' is used in @return or in @var, then it's treated like 'any' and replaced by an inferred lambda
  }
}

static void recalc_ffi_type(TypeData* dst, const std::string& scope_name, const FFIType* type) {
  if (type->kind == FFITypeKind::Pointer) {
    TypeData nested{*TypeData::get_type(tp_any)};
    recalc_ffi_type(&nested, scope_name, type->members[0]);
    nested.set_indirection(type->num); // turn it into a pointer
    if (type->members[0]->is_const()) {
      nested.set_ffi_const_flag();
    }
    dst->set_lca(&nested);
    return;
  }

  if (type->kind == FFITypeKind::Array) {
    dst->set_lca(G->get_class("FFI\\CData")->type_data);
    TypeData nested{*TypeData::get_type(tp_any)};
    recalc_ffi_type(&nested, scope_name, type->members[0]);
    dst->set_lca_at(MultiKey::any_key(1), &nested);
    return;
  }

  if (vk::any_of_equal(type->kind, FFITypeKind::Struct, FFITypeKind::StructDef, FFITypeKind::Union, FFITypeKind::UnionDef)) {
    std::string class_name = FFIRoot::cdata_class_name(scope_name, type->str);
    ClassPtr cdata_class = G->get_class(class_name);
    if (cdata_class) {
      dst->set_lca(cdata_class->type_data);
    }
    return;
  }

  if (const auto* builtin = ffi_builtin_type(type->kind)) {
    dst->set_lca(G->get_class(builtin->php_class_name)->type_data);
    return;
  }
}

void TypeHintFFIType::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call __attribute__((unused))) const {
  recalc_ffi_type(dst, scope_name, type);
}

void TypeHintFFIScopeArgRef::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  if (auto v = VertexUtil::get_call_arg_ref(arg_num, call)) {
    if (const auto* arg_scope_name = VertexUtil::get_constexpr_string(v)) {
      dst->set_lca(G->get_class(FFIRoot::scope_class_name(*arg_scope_name))->type_data);
    }
  }
}

void TypeHintFFIScope::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call __attribute__((unused))) const {
  dst->set_lca(G->get_class(FFIRoot::scope_class_name(scope_name))->type_data);
}

void TypeHintFuture::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  kphp_assert(vk::any_of_equal(ptype, tp_future, tp_future_queue));
  dst->set_lca(ptype);
  TypeData nested(*TypeData::get_type(tp_any));
  inner->recalc_type_data_in_context_of_call(&nested, call);
  dst->set_lca_at(MultiKey::any_key(1), &nested);
}

void TypeHintNotNull::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  TypeData nested(*TypeData::get_type(tp_any));
  inner->recalc_type_data_in_context_of_call(&nested, call);
  dst->set_lca(&nested, !drop_not_false, !drop_not_null);
}

void TypeHintInstance::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call __attribute__((unused))) const {
  dst->set_lca(resolve()->type_data);
}

void TypeHintRefToField::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call __attribute__((unused))) const {
  kphp_error(0, "Syntax ClassName::field is available only in a combination with generics");
  dst->set_lca(tp_any);
}

void TypeHintRefToMethod::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call __attribute__((unused))) const {
  kphp_error(0, "Syntax ClassName::method() is available only in a combination with generics");
  dst->set_lca(tp_any);
}

void TypeHintOptional::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  inner->recalc_type_data_in_context_of_call(dst, call);
  if (or_null) {
    dst->set_lca(TypeData::get_type(tp_Null));
  }
  if (or_false) {
    dst->set_lca(TypeData::get_type(tp_False));
  }
}

void TypeHintPipe::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  for (const TypeHint* item : items) {
    item->recalc_type_data_in_context_of_call(dst, call);
  }
}

void TypeHintPrimitive::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call __attribute__((unused))) const {
  dst->set_lca(TypeData::get_type(ptype));
}

void TypeHintObject::recalc_type_data_in_context_of_call(TypeData* dst __attribute__((unused)), VertexPtr call __attribute__((unused))) const {
  // 'object' keyword is allowed only in params of functions
  // (it remains for extern functions, but for PHP functions it converts a function into generic and drops away)
  dst->set_lca(tp_object);
}

void TypeHintShape::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  dst->set_lca(TypeData::get_type(tp_shape));
  for (const auto& item : items) {
    TypeData nested(*TypeData::get_type(tp_any));
    item.second->recalc_type_data_in_context_of_call(&nested, call);
    MultiKey key({Key::string_key(item.first)});
    dst->set_lca_at(key, &nested);
  }
  if (is_vararg) {
    dst->set_shape_has_varg_flag();
  }
}

void TypeHintTuple::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call) const {
  dst->set_lca(TypeData::get_type(tp_tuple));
  for (int int_index = 0; int_index < items.size(); ++int_index) {
    TypeData nested(*TypeData::get_type(tp_any));
    items[int_index]->recalc_type_data_in_context_of_call(&nested, call);
    MultiKey key({Key::int_key(int_index)});
    dst->set_lca_at(key, &nested);
  }
}

void TypeHintGenericT::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call __attribute__((unused))) const {
  dst->set_lca(tp_Error);
  kphp_assert(0 && "generic T not instantiated before type inferring");
}

void TypeHintClassString::recalc_type_data_in_context_of_call(TypeData* dst, VertexPtr call __attribute__((unused))) const {
  dst->set_ptype(tp_string);
}
