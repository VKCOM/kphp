#include "compiler/pipes/preprocess-function.h"

#include "common/termformat/termformat.h"
#include "common/wrappers/likely.h"

#include "compiler/data/class-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/lambda-generator.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/name-gen.h"

class PreprocessFunctionPass : public FunctionPassBase {
public:
  DataStream<FunctionPtr> &instance_of_function_template_stream;

  explicit PreprocessFunctionPass(DataStream<FunctionPtr> &instance_of_function_template_stream) :
    instance_of_function_template_stream(instance_of_function_template_stream) {}

  std::string get_description() override {
    return "Preprocess function";
  }

  bool check_function(FunctionPtr function) override {
    return default_check_function(function) && !function->is_extern() && !function->is_template;
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *) {
    if (auto clone_root = root.try_as<op_clone>()) {
      ClassPtr klass;
      auto assum = infer_class_of_expr(stage::get_function(), clone_root, klass);
      kphp_error_act(assum == assum_instance, "`clone` keyword could be used only with instances", return clone_root);
      kphp_error_act(!klass->is_builtin(), format("`%s` class is forbidden for clonning", klass->name.c_str()), return clone_root);
      bool has_derived = !klass->derived_classes.empty();
      bool clone_is_inside_virt_clone = current_function->local_name() == ClassData::NAME_OF_VIRT_CLONE;
      if (klass->is_interface() || (has_derived && !clone_is_inside_virt_clone)) {
        /**
         * clone of interfaces are replaced with call of virtual method
         * which automatic calls clone of derived class
         */
        auto virt_clone = klass->members.get_instance_method(ClassData::NAME_OF_VIRT_CLONE);
        kphp_assert(virt_clone);
        auto virt_clone_func = virt_clone->function;
        kphp_assert(virt_clone_func);

        auto call_function = VertexAdaptor<op_func_call>::create(clone_root->expr()).set_location(clone_root);
        call_function->extra_type = op_ex_func_call_arrow;
        call_function->set_string(ClassData::NAME_OF_VIRT_CLONE);
        call_function->func_id = virt_clone_func;

        return call_function;
      }

      if (klass->members.has_instance_method(ClassData::NAME_OF_CLONE)) {
        auto location = clone_root->get_location();

        auto tmp_var = VertexAdaptor<op_var>::create();
        tmp_var->set_string(gen_unique_name("tmp_for_clone"));
        tmp_var->extra_type = op_ex_var_superlocal_inplace;

        assumption_add_for_var(stage::get_function(), assum_instance, tmp_var->get_string(), klass);

        auto set_clone_to_tmp = VertexAdaptor<op_set>::create(tmp_var, clone_root);

        auto call_of___clone = VertexAdaptor<op_func_call>::create(tmp_var);
        call_of___clone->set_string(ClassData::NAME_OF_CLONE);
        call_of___clone->extra_type = op_ex_func_call_arrow;

        call_of___clone = try_set_func_id(call_of___clone).as<op_func_call>();

        root =  VertexAdaptor<op_seq_rval>::create(set_clone_to_tmp, call_of___clone, tmp_var);
        set_location(location, tmp_var, set_clone_to_tmp, call_of___clone, root);
      }
    }

    return root;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *) {
    if (auto instanceof = root.try_as<op_instanceof>()) {
      auto str_repr_of_class = GenTree::get_actual_value(instanceof->rhs())->get_string();
      instanceof->derived_class = G->get_class(str_repr_of_class);
      kphp_error(instanceof->derived_class, format("Can't find class: %s", str_repr_of_class.c_str()));
    }

    if (vk::any_of_equal(root->type(), op_func_call, op_func_ptr, op_constructor_call)) {
      root = try_set_func_id(root);
    }

    if (auto return_op = root.try_as<op_return>()) {
      if (current_function->type == FunctionData::func_global ||
          current_function->type == FunctionData::func_switch ||
          current_function->disabled_warnings.count("return")) {
        if (return_op->has_expr()) {
          return VertexAdaptor<op_return>::create(VertexAdaptor<op_conv_var>::create(return_op->expr()));
        } else {
          return VertexAdaptor<op_return>::create(VertexAdaptor<op_null>::create());
        }
      }
    }

    return root;
  }

private:
  FunctionPtr generate_instance_template_function_by_name(const std::map<int, std::pair<AssumType, ClassPtr>> &template_type_id_to_ClassPtr,
                                                          FunctionPtr func,
                                                          const std::string &name_of_function_instance) {
    FunctionPtr instance;
    G->operate_on_function_locking(name_of_function_instance, [&](FunctionPtr &f_inst) {
      if (!f_inst || f_inst->is_template) {
        f_inst = FunctionData::generate_instance_of_template_function(template_type_id_to_ClassPtr, func, name_of_function_instance);
        if (f_inst) {
          ClassPtr klass = f_inst->class_id;
          if (klass) {
            AutoLocker<Lockable *> locker(&(*klass));
            if (klass->members.get_instance_method(f_inst->local_name())) {
              klass->members.for_each([&](ClassMemberInstanceMethod &m) {
                if (m.global_name() == f_inst->name) {
                  m.function = f_inst;
                }
              });
            } else {
              klass->members.add_instance_method(f_inst, f_inst->access_type);
            }
          }

          kphp_assert(!f_inst->is_required);
          G->require_function(f_inst, instance_of_function_template_stream);
        }
      }

      if (f_inst) {
        kphp_assert_msg(!f_inst->is_template, "instance of template function must be non template");
        instance = f_inst;
      }
    });

    return instance;
  }

  void instantiate_lambda(VertexAdaptor<op_func_call> call, VertexPtr &call_arg) {
    if (auto lambda_class = LambdaClassData::get_from(call_arg)) {
      FunctionPtr instance_of_template_invoke;
      std::string invoke_name;

      if (auto template_of_invoke_method = lambda_class->get_template_of_invoke_function()) {
        std::map<int, std::pair<AssumType, ClassPtr>> template_type_id_to_ClassPtr;
        invoke_name = lambda_class->get_name_of_invoke_function_for_extern(call, current_function, &template_type_id_to_ClassPtr, &template_of_invoke_method);

        instance_of_template_invoke = generate_instance_template_function_by_name(template_type_id_to_ClassPtr, template_of_invoke_method, invoke_name);
        instance_of_template_invoke->instantiation_of_template_function_location = call_arg->get_location();
      } else {
        kphp_assert(lambda_class->members.has_any_instance_method());
        instance_of_template_invoke = lambda_class->members.get_instance_method(ClassData::NAME_OF_INVOKE_METHOD)->function;
        invoke_name = instance_of_template_invoke->name;
      }

      if (instance_of_template_invoke) {
        VertexAdaptor<op_func_ptr> new_call_arg = VertexAdaptor<op_func_ptr>::create(call_arg);
        new_call_arg->func_id = instance_of_template_invoke;
        new_call_arg->set_string(invoke_name);
        ::set_location(new_call_arg, call_arg->location);
        call_arg = new_call_arg;
      }
    }
  }

  VertexAdaptor<op_func_call> set_func_id_for_template(FunctionPtr func, VertexAdaptor<op_func_call> call) {
    kphp_assert(func->is_template);

    std::map<int, std::pair<AssumType, ClassPtr>> template_type_id_to_ClassPtr;
    std::string name_of_function_instance = func->name;

    VertexRange func_args = func->get_params();

    for (int i = 0; i < func_args.size(); ++i) {
      if (auto param = func_args[i].as<op_func_param>()) {
        if (param->template_type_id >= 0) {
          ClassPtr class_corresponding_to_parameter{nullptr};
          VertexPtr call_arg;
          if (i < call->args().size()) {
            call_arg = call->args()[i];
          } else {
            kphp_error_act(param->has_default_value(), format("missed %dth argument in function call: %s", i, call->get_string().c_str()), return call);
            call_arg = param->default_value();
          }

          AssumType assum = infer_class_of_expr(stage::get_function(), call_arg, class_corresponding_to_parameter);
          if (assum == assum_unknown) {
            assum = assum_not_instance;
          }

          auto insertion_result = template_type_id_to_ClassPtr.emplace(param->template_type_id, std::make_pair(assum, class_corresponding_to_parameter));
          if (!insertion_result.second) {
            const std::pair<AssumType, ClassPtr> &previous_assum_and_class = insertion_result.first->second;
            auto wrap_if_array = [](ClassPtr c, AssumType assum) {
              if (assum == assum_instance_array) return c->name + "[]";

              return assum == assum_instance ? c->name : "not instance";
            };

            auto prev_name = wrap_if_array(previous_assum_and_class.second, previous_assum_and_class.first);
            auto new_name = wrap_if_array(class_corresponding_to_parameter, assum);
            if (previous_assum_and_class.first != assum || prev_name != new_name) {
              std::string error_msg =
                "argument $" + param->var()->get_string() + " of " + func->name +
                " has a type: `" + prev_name + "` but expected type: `" + new_name + "`";

              kphp_error_act(false, error_msg.c_str(), return call);
            }
          }

          name_of_function_instance += FunctionData::encode_template_arg_name(assum, i, class_corresponding_to_parameter);
        }
      }
    }

    call->set_string(name_of_function_instance);
    call->func_id = {};
    if (auto instance = generate_instance_template_function_by_name(template_type_id_to_ClassPtr, func, name_of_function_instance)) {
      instance->instantiation_of_template_function_location = call->get_location();
      set_func_id(call, instance);
    }

    return call;
  }

  VertexAdaptor<op_func_call> process_varargs(VertexAdaptor<op_func_call> call, FunctionPtr func) {
    if (!func->has_variadic_param) {
      return call;
    }

    int call_args_n = (int)call->args().size();
    int func_args_n = (int)func->get_params().size();
    for (int i = 0; i < call_args_n; i++) {
      kphp_error_act(call->args()[i]->type() != op_func_name, "Unexpected function pointer", return {});
    }

    // calling function with variadic arguments involve some transformations:
    // imagine that we have:
    //   function fun($x, ...$args)
    //
    // transformations will be:
    //   fun(1, 2, 3) -> fun(1, [2, 3])
    //   fun(1, ...$arr) -> fun(1, $arr)
    //   fun(1, 2, 3, ...$arr1, ...$arr2) -> fun(1, array_merge([2, 3], $arr1, $arr2))
    //
    // If this function is method of some class, than we need skip implicit $this argument
    // just as ordinary positional argument:
    //   $this->fun(1, 2, 3) -> fun($this, 1, [2, 3])

    const std::vector<VertexPtr> &cur_call_args = call->get_next();
    auto positional_args_start = cur_call_args.begin();

    auto variadic_args_start = positional_args_start;
    kphp_assert(func_args_n > 0);

    kphp_error_act(func_args_n - 1 <= call_args_n,
                   format("function takes: %d arguments; passed only %d", func_args_n, call_args_n),
                   return {});
    std::advance(variadic_args_start, func_args_n - 1);

    auto unpacking_args_start = std::find_if(cur_call_args.begin(), cur_call_args.end(), [](const VertexPtr &ptr) { return ptr->type() == op_varg; });
    kphp_error_act(std::distance(variadic_args_start, unpacking_args_start) >= 0,
                   "It's prohibited to unpack arrays in places where positional arguments expected",
                   return {});
    std::vector<VertexPtr> new_call_args(positional_args_start, variadic_args_start);

    // case when variadic params just have been forwarded
    // e.g. fun(...$args) will be transformed to f($args) without any array_merge
    if (variadic_args_start == unpacking_args_start && std::distance(unpacking_args_start, cur_call_args.end()) == 1) {
      new_call_args.emplace_back(GenTree::conv_to<tp_array>(unpacking_args_start->as<op_varg>()->array()));
    } else {
      std::vector<VertexPtr> variadic_args_passed_as_positional(variadic_args_start, unpacking_args_start);
      auto array_from_varargs_passed_as_positional = VertexAdaptor<op_array>::create(variadic_args_passed_as_positional);
      array_from_varargs_passed_as_positional.set_location(call);

      if (unpacking_args_start != cur_call_args.end()) {
        std::vector<VertexPtr> unpacking_args_converted_to_array;
        unpacking_args_converted_to_array.reserve(static_cast<size_t>(std::distance(unpacking_args_start, cur_call_args.end())));
        for (auto unpack_arg_it = unpacking_args_start; unpack_arg_it != cur_call_args.end(); ++unpack_arg_it) {
          if (auto unpack_as_varg = unpack_arg_it->try_as<op_varg>()) {
            unpacking_args_converted_to_array.emplace_back(GenTree::conv_to<tp_array>(unpack_as_varg->array()));
          } else {
            const std::string &s = (*unpack_arg_it)->has_get_string() ? (*unpack_arg_it)->get_string() : "";
            kphp_error_act(false, format("It's prohibited using something after argument unpacking: `%s`", s.c_str()), return {});
          }
        }
        auto merge_arrays = VertexAdaptor<op_func_call>::create(array_from_varargs_passed_as_positional, unpacking_args_converted_to_array);
        merge_arrays->set_string("array_merge");
        merge_arrays.set_location(call);
        merge_arrays->func_id = G->get_function("array_merge");

        new_call_args.emplace_back(merge_arrays);
      } else {
        new_call_args.emplace_back(array_from_varargs_passed_as_positional);
      }
    }

    auto new_call = VertexAdaptor<op_func_call>::create(new_call_args);
    new_call->copy_location_and_flags(*call);
    new_call->extra_type = call->extra_type;
    new_call->func_id = func;
    new_call->str_val = call->str_val;

    return new_call;
  }

  bool convert_array_with_instance_to_lambda_class(VertexPtr &arr, Location anon_location) {
    auto array_arg = arr.try_as<op_array>();
    if (!array_arg || array_arg->size() != 2) {
      return false;
    }

    const auto *name_of_class_method = GenTree::get_constexpr_string(array_arg->args()[1]);
    if (!name_of_class_method) {
      return false;
    }

    kphp_error_act(!vk::string_view{*name_of_class_method}.starts_with("__"),
      format("Call methods with prefix `__` are prohibited: `%s`", name_of_class_method->c_str()),
      return false);

    ClassPtr captured_class;
    AssumType assum = infer_class_of_expr(current_function, array_arg->args()[0], captured_class);
    if (assum != AssumType::assum_instance) {
      return false;
    }

    auto called_method = captured_class->members.get_instance_method(*name_of_class_method);
    if (!called_method) {
      auto err_msg = "Can't find instance method: " +
                     TermStringFormat::paint(FunctionData::get_human_readable_name(captured_class->name + "$$" + *name_of_class_method), TermStringFormat::green);
      kphp_error(false, err_msg.c_str());
      return false;
    }

    arr = LambdaGenerator{current_function, anon_location}
      .add_invoke_method_which_call_method(called_method->function)
      .add_constructor_from_uses()
      .generate_and_require(current_function, instance_of_function_template_stream)
      ->gen_constructor_call_with_args({array_arg->args()[0]});

    return true;
  }

  VertexPtr set_func_id(VertexPtr call, FunctionPtr func) {
    kphp_assert (call->type() == op_func_ptr || call->type() == op_func_call || call->type() == op_constructor_call);
    FunctionPtr &func_id = call->type() == op_func_ptr ? call.as<op_func_ptr>()->func_id : call.as<op_func_call>()->func_id;
    kphp_assert (func);
    kphp_assert (!func_id || func_id == func);
    if (func_id == func) {
      return call;
    }
    //fprintf (stderr, "%s\n", func->name.c_str());

    func_id = func;
    if (call->type() == op_func_ptr) {
      if (!func->is_required) {
        std::string err = "function: `" + func->name + "` need tag @kphp-required, because used only as string passed to internal functions";
        kphp_error_act(false, err.c_str(), return call);
      }
      return call;
    }

    if (!func->root) {
      kphp_fail();
      return call;
    }

    if (auto new_call = process_varargs(call.as<op_func_call>(), func)) {
      call = new_call;
    } else {
      return call;
    }

    VertexRange call_args = call.as<op_func_call>()->args();
    VertexRange func_args = func->get_params();
    int call_args_n = (int)call_args.size();
    int func_args_n = (int)func_args.size();

    for (int i = 0; i < std::min(call_args_n, func_args_n); i++) {
      auto &call_arg = call_args[i];
      switch (func_args[i]->type()) {
        case op_func_param: {
          if (call_arg->type() == op_func_name) {
            string msg = "Unexpected function pointer: " + call_arg->get_string();
            kphp_error(false, msg.c_str());
            continue;
          } else if (call_arg->type() == op_varg) {
            string msg = "function: `" + func->name + "` must takes variable-length argument list";
            kphp_error_act(false, msg.c_str(), break);
          }

          auto param = func_args[i].as<op_func_param>();
          if (param->type_rule) {
            call_arg->type_rule = param->type_rule;
          } else if (param->type_help != tp_Unknown) {
            call_arg = GenTree::conv_to(call_arg, param->type_help, param->var()->ref_flag);
          }

          bool is_callable_ok = true;
          std::string name_of_fun_for_wrapping_into_lambda;
          if (param->is_callable) {
            is_callable_ok = vk::none_of_equal(call_arg->type(), op_string, op_array);
            name_of_fun_for_wrapping_into_lambda = conv_to_func_ptr_name(call_arg);
            if (!name_of_fun_for_wrapping_into_lambda.empty()) {
              FunctionPtr fun_for_wrapping = G->get_function(name_of_fun_for_wrapping_into_lambda);
              if (!fun_for_wrapping || !fun_for_wrapping->is_required) {
                auto err_msg = "Maybe you have never used class or miss @kphp-required tag: " +
                               TermStringFormat::paint(FunctionData::get_human_readable_name(name_of_fun_for_wrapping_into_lambda), TermStringFormat::green);
                kphp_error_act(false, err_msg.c_str(), return call);
              }

              auto anon_location = call->location;
              call_arg = LambdaGenerator{current_function, anon_location}
                .add_invoke_method_which_call_function(fun_for_wrapping)
                .add_constructor_from_uses()
                .generate_and_require(current_function, instance_of_function_template_stream)
                ->gen_constructor_call_pass_fields_as_args();
              is_callable_ok = true;
            } else {
              if (convert_array_with_instance_to_lambda_class(call_arg, get_location(call))) {
                is_callable_ok = true;
              } else if (stage::has_error()) {
                return call;
              }
            }
          }

          if (!is_callable_ok) {
            std::string fun_name = name_of_fun_for_wrapping_into_lambda;
            if (fun_name.empty() && call_arg->type() == op_array) {
              auto arr = call_arg.as<op_array>();
              if (arr->args().size() == 2 && arr->args()[1]->type() == op_string) {
                fun_name = arr->args()[1]->get_string();
              }
            }

            auto err_msg = "can't find function:" + TermStringFormat::paint(fun_name, TermStringFormat::green) + ", maybe you miss @kphp-required tag";
            kphp_error_act(false, err_msg.c_str(), return call);
          }

          break;
        }

        case op_func_param_callback: {
          bool converted = convert_array_with_instance_to_lambda_class(call_arg, get_location(call));
          if (!converted) {
            call_arg = conv_to_func_ptr(call_arg);
          }

          instantiate_lambda(call.as<op_func_call>(), call_arg);
          break;
        }

        default: {
          kphp_fail();
        }
      }
    }

    if (func->is_template) {
      call = set_func_id_for_template(func, call.as<op_func_call>());
    }

    return call;
  }

  /**
   * Имея vertex вида 'fn(...)' или 'new A(...)', сопоставить этому vertex реальную FunctionPtr
   *  (он будет доступен через vertex->get_func_id()).
   * Вызовы instance-методов вида $a->fn(...) были на уровне gentree преобразованы в op_func_call fn($a, ...),
   * со спец. extra_type, поэтому для таких можно определить FunctionPtr по первому аргументу.
   */
  VertexPtr try_set_func_id(VertexPtr call) {
    FunctionPtr func_id = call->type() == op_func_ptr ? call.as<op_func_ptr>()->func_id : call.as<op_func_call>()->func_id;
    if (func_id) {
      if (func_id->is_lambda_with_uses()) {
        LambdaPtr lambda_class = func_id->class_id.as<LambdaClassData>();
        if (!lambda_class->construct_function->is_template) {
          return call;
        }

        kphp_assert(!lambda_class->construct_function->is_required);

        lambda_class->infer_uses_assumptions(current_function);
        call->location.function = current_function;
        lambda_class->construct_function->is_template = false;
        G->require_function(lambda_class->construct_function, instance_of_function_template_stream);
      }

      return call;
    }

    const string &name =
      call->type() == op_constructor_call
      ? resolve_constructor_func_name(current_function, call.as<op_constructor_call>())
      : call->type() == op_func_call && call->extra_type == op_ex_func_call_arrow
        ? resolve_instance_func_name(current_function, call.as<op_func_call>())
        : call->get_string();

    FunctionPtr f = G->get_function(name);

    if (likely(!!f)) {
      kphp_error(!f->is_static_function() || call->extra_type != op_ex_func_call_arrow, "It's not allowed to call static method through instance var");
      call = set_func_id(call, f);
    } else {
      print_why_cant_set_func_id_error(call, name);
    }

    return call;
  }

  void print_why_cant_set_func_id_error(VertexPtr call, const std::string &unexisting_func_name) {
    if (call->type() == op_constructor_call) {
      ClassPtr klass = G->get_class(call->get_string());
      if (klass) {
        kphp_error(0, format("Calling 'new %s()', but this class is %s", call->get_c_string(), klass->is_interface() ? "interface" : "fully static"));
      } else {
        kphp_error(0, format("Class %s does not exist", call->get_c_string()));
      }
    } else if (call->type() == op_func_call && call->extra_type == op_ex_func_call_arrow) {
      ClassPtr klass;
      infer_class_of_expr(current_function, call.as<op_func_call>()->args()[0], klass);
      kphp_error(0, format("Unknown function ->%s() of %s\n", call->get_c_string(), klass ? klass->name.c_str() : "Unknown class"));
    } else {
      kphp_error(0, format("Unknown function %s()\n", unexisting_func_name.c_str()));
    }
  }
};

void PreprocessFunctionF::execute(FunctionPtr function, OStreamT &os) {
  DataStream<FunctionPtr> tmp_stream{true};
  PreprocessFunctionPass pass(tmp_stream);

  run_function_pass(function, &pass);

  for (auto fun: tmp_stream.flush()) {
    *os.project_to_nth_data_stream<1>() << fun;
  }

  if (!stage::has_error() && !function->is_template) {
    (*os.project_to_nth_data_stream<0>()) << function;
  }
}
