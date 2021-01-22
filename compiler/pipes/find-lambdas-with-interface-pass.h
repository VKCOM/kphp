// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <map>

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/function-pass.h"
#include "compiler/name-gen.h"
#include "compiler/type-hint.h"

/**
 * finds lambdas in different places, if these places expect interface with __invoke method
 * than lambdas start to implement the interface; all such interfaces are saved to std::set
 * for generation their abstract __invoke method
 */
class FindLambdasWithInterfacePass final : public FunctionPassBase {
public:
  std::map<ClassPtr, std::vector<ClassPtr>> lambdas_interfaces;

  std::string get_description() override {
    return "Find lambdas with interface";
  }

  bool check_function(FunctionPtr function) const override {
    return !function->is_extern();
  }

  /**
   * Lambdas are something like this `$lhs = function() { ... }`
   * in AST they are represented as `LambdaConstructor(Lamba.alloc())` = op_func_call(op_alloc, op_func_param...)
   */
  static LambdaPtr get_lambda_class(VertexPtr arg) {
    auto func_call_arg = arg.try_as<op_func_call>();
    if (!func_call_arg || !is_constructor_call(func_call_arg)) {
      return {};
    }
    if (auto alloc_inside = func_call_arg->args()[0].try_as<op_alloc>()) {
      return alloc_inside->allocated_class.try_as<LambdaClassData>();
    }
    return {};
  }

  void check_assumption_and_inherit(const Assumption &assumption, LambdaPtr lambda) {
//    auto interface_assumption = assumption.type_hint->try_as<TypeHintCallable>();
//    if (!interface_assumption || !interface_assumption->is_typed_callable() || !interface_assumption->get_interface()->is_interface()) {
//      return;
//    }
    auto interface_assumption = assumption.try_as_class();
    if (!interface_assumption || !interface_assumption->is_interface()) {
      return;
    }
    if (!lambda->can_implement_interface(interface_assumption)) {
      kphp_error(false, fmt_format("lambda can't implement interface: {}", interface_assumption->name));
      return;
    }
    lambda->implement_interface(interface_assumption);
    lambdas_interfaces[interface_assumption].emplace_back(lambda);
  }

  /**
   * finds patterns like this
   * my_func(function() {...})
   * $this->my_method(10, function() {...})
   */
  void find_interface_in_func_call(VertexAdaptor<op_func_call> func_call) {
    if (std::none_of(func_call->args().begin(), func_call->args().end(), get_lambda_class)) {
      return;
    }

    const string &name = func_call->extra_type == op_ex_func_call_arrow
                         ? resolve_instance_func_name(current_function, func_call)
                         : func_call->get_string();

    FunctionPtr called_function = G->get_function(name);
    if (!called_function || called_function->is_extern()) {
      return;
    }

    auto call_args = func_call->args();
    auto func_args = called_function->get_params();
    auto call_args_n = static_cast<int>(call_args.size());
    auto func_args_n = static_cast<int>(func_args.size());

    for (int i = 0; i < std::min(call_args_n, func_args_n); i++) {
      if (auto lambda_class = get_lambda_class(call_args[i])) {
        auto func_param = func_args[i].as<op_func_param>();
        if (!func_param) {
          continue;
        }

        auto assumption = calc_assumption_for_argument(called_function, func_param->var()->str_val);
        check_assumption_and_inherit(assumption, lambda_class);
      }
    }
  }

  /**
   * finds patterns like this
   * $lhs = function() {...}
   * $lhs[] = function() {...}
   * $lhs = [[function() {...}]];
   */
  void find_interface_in_op_set(VertexPtr lhs, VertexPtr rhs) {
    if (auto rhs_array = rhs.try_as<op_array>()) {
      for (auto array_element : rhs_array->args()) {
        if (auto double_arrow = array_element.try_as<op_double_arrow>()) {
          find_interface_in_op_set(lhs, double_arrow->value());
        } else {
          find_interface_in_op_set(lhs, array_element);
        }
      }
    }
    auto lambda_class = get_lambda_class(rhs);
    if (!lambda_class) {
      return;
    }

    // replace $a[] with $a, to find out assumption of $a
    if (auto lhs_array = lhs.try_as<op_index>()) {
      if (!lhs_array->has_key()) {
        lhs = lhs_array->array();
      }
    }

    auto assum = infer_class_of_expr(current_function, lhs);
    while (Assumption inner = assum.get_inner_if_array()) {
      assum = inner;
    }

    check_assumption_and_inherit(assum, lambda_class);
  }

  /**
   * finds patterns like this
   * function my_func() {
   *   if (true) return [function() {...}];
   *   else return [function() {...}];
   * }
   */
  void find_interface_in_op_return(VertexPtr v) {
    if (auto return_op = v.try_as<op_return>()) {
      v = return_op->has_expr() ? return_op->expr() : v;
    }

    if (auto return_array = v.try_as<op_array>()) {
      for (auto array_element : return_array->args()) {
        find_interface_in_op_return(array_element);
      }
    } else if (auto lambda_class = get_lambda_class(v)) {
      auto assum = calc_assumption_for_return(current_function, {});
      while (Assumption inner = assum.get_inner_if_array()) {
        assum = inner;
      }

      check_assumption_and_inherit(assum, lambda_class);
    }
  }

  VertexPtr on_enter_vertex(VertexPtr root) final {
    if (auto func_call = root.try_as<op_func_call>()) {
      find_interface_in_func_call(func_call);
    } else if (auto set_operation = root.try_as<op_set>()) {
      find_interface_in_op_set(set_operation->lhs(), set_operation->rhs());
    } else if (auto return_op = root.try_as<op_return>()) {
      find_interface_in_op_return(return_op);
    }
    return root;
  }
};
