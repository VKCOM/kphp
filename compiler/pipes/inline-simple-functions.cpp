// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/inline-simple-functions.h"

#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/public.h"

void InlineSimpleFunctions::on_simple_operation() noexcept {
  if (++n_simple_operations_ > 6) {
    inline_is_possible_ = false;
  }
}

VertexPtr InlineSimpleFunctions::on_enter_vertex(VertexPtr root) {
  if (root.try_as<meta_op_num>()) {
    return root;
  }

  if (root.try_as<meta_op_binary>() || root.try_as<meta_op_unary>()) {
    on_simple_operation();
    return root;
  }

  if (auto var_vertex = root.try_as<op_var>()) {
    if (in_param_list_ && var_vertex->var_id &&
        !var_vertex->var_id->marked_as_const &&
        !var_vertex->var_id->is_read_only &&
        !var_vertex->var_id->is_reference &&
        !tinf::get_type(var_vertex->var_id)->is_primitive_type()) {
      inline_is_possible_ = false;
    }
    return root;
  }

  switch (root->type()) {
    case op_empty:
    case op_func_param:
    case op_false:
    case op_true:
    case op_null:
    case op_function:
    case op_instance_prop:
    case op_ffi_cdata_value_ref:
    case op_ffi_cast:
    case op_ffi_load_call:
    case op_ffi_php2c_conv:
    case op_ffi_c2php_conv:
      break;
    case op_func_name:
    case op_func_call:
    case op_index:
    case op_push_back:
    case op_return:
    case op_ternary:
    case op_if:
    case op_alloc:
    case op_ffi_new:
    case op_ffi_addr:
    case op_ffi_array_get:
    case op_ffi_array_set:
      on_simple_operation();
      break;
    case op_func_param_list:
      in_param_list_ = true;
      // fallthrough
    case op_seq:
      if (root->size() > 5) {
        inline_is_possible_ = false;
      }
      break;
    case op_string_build:
    case op_array:
    case op_tuple:
    case op_shape:
      if (root->size() > 2) {
        inline_is_possible_ = false;
      }
      break;
    default:
      inline_is_possible_ = false;
  }
  return root;
}

VertexPtr InlineSimpleFunctions::on_exit_vertex(VertexPtr root) {
  if (root.try_as<op_func_param_list>()) {
    in_param_list_ = false;
  }
  return root;
}

bool InlineSimpleFunctions::user_recursion(VertexPtr) {
  return !inline_is_possible_;
}

bool InlineSimpleFunctions::check_function(FunctionPtr function) const {
  return !function->is_resumable &&
         !function->is_inline &&
         !function->can_throw() &&
         !function->has_variadic_param &&
         !function->is_main_function() &&
         function->type != FunctionData::func_class_holder &&
         (!function->modifiers.is_instance() || function->local_name() != "__wakeup") &&
         !function->kphp_lib_export;
}

void InlineSimpleFunctions::on_start() {
  if (auto klass = current_function->class_id) {
    if (klass->internal_interface) {
      inline_is_possible_ = false;
    }
  }
  return FunctionPassBase::on_start();
}

void InlineSimpleFunctions::on_finish() {
  if (inline_is_possible_) {
    current_function->is_inline = true;
  }
  return FunctionPassBase::on_finish();
}
