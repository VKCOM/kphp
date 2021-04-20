// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

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
#include "compiler/type-hint.h"

namespace {

bool implicit_cast_allowed(bool strict_types, const TypeHint *type) {
  if (!strict_types) {
    return true;
  }
  // converting to regexp is OK even in strict_types mode
  if (const auto *primitive_type = type->try_as<TypeHintPrimitive>()) {
    return primitive_type->ptype == tp_regexp;
  }
  return false;
}

} // namespace

class PreprocessFunctionPass final : public FunctionPassBase {
public:
  DataStream<FunctionPtr> &instance_of_function_template_stream;

  explicit PreprocessFunctionPass(DataStream<FunctionPtr> &instance_of_function_template_stream) :
    instance_of_function_template_stream(instance_of_function_template_stream) {}

  std::string get_description() override {
    return "Preprocess function";
  }

  bool check_function(FunctionPtr function) const override {
    return !function->is_extern();
  }

  VertexPtr on_exit_vertex(VertexPtr root) final {
    if (auto clone_root = root.try_as<op_clone>()) {
      auto klass = infer_class_of_expr(stage::get_function(), clone_root).try_as_class();
      kphp_error_act(klass, "`clone` keyword could be used only with instances", return clone_root);
      kphp_error_act(!klass->is_builtin(), fmt_format("`{}` class is forbidden for clonning", klass->name), return clone_root);
      bool clone_is_inside_virt_clone = vk::any_of_equal(current_function->local_name(),
                                                         ClassData::NAME_OF_VIRT_CLONE,
                                                         FunctionData::get_name_of_self_method(ClassData::NAME_OF_VIRT_CLONE));
      if (!klass->derived_classes.empty() && !clone_is_inside_virt_clone) {
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
        auto tmp_var = VertexAdaptor<op_var>::create().set_location(clone_root);
        tmp_var->set_string(gen_unique_name("tmp_for_clone"));
        tmp_var->extra_type = op_ex_var_superlocal;

        assumption_add_for_var(stage::get_function(), tmp_var->get_string(), Assumption(klass));

        auto set_clone_to_tmp = VertexAdaptor<op_set>::create(tmp_var, clone_root).set_location(clone_root);

        auto call_of___clone = VertexAdaptor<op_func_call>::create(tmp_var).set_location(clone_root);
        call_of___clone->set_string(ClassData::NAME_OF_CLONE);
        call_of___clone->extra_type = op_ex_func_call_arrow;

        call_of___clone = try_set_func_id(call_of___clone).as<op_func_call>();

        root = VertexAdaptor<op_seq_rval>::create(set_clone_to_tmp, call_of___clone, tmp_var).set_location(clone_root);
      }
    }

    return root;
  }

  VertexPtr on_enter_vertex(VertexPtr root) final {
    if (auto instanceof = root.try_as<op_instanceof>()) {
      auto str_repr_of_class = GenTree::get_actual_value(instanceof->rhs())->get_string();
      instanceof->derived_class = G->get_class(str_repr_of_class);
      kphp_error(instanceof->derived_class, fmt_format("Can't find class: {}", str_repr_of_class));
    }

    if (vk::any_of_equal(root->type(), op_func_call, op_func_ptr)) {
      root = try_set_func_id(root);
    }

    if (auto return_op = root.try_as<op_return>()) {
      if (current_function->is_main_function() ||
          current_function->type == FunctionData::func_switch ||
          current_function->disabled_warnings.count("return")) {
        if (return_op->has_expr()) {
          return VertexAdaptor<op_return>::create(VertexAdaptor<op_force_mixed>::create(return_op->expr()));
        } else {
          return VertexAdaptor<op_return>::create(VertexAdaptor<op_null>::create());
        }
      }
    }

    return root;
  }

private:
  FunctionPtr generate_instance_template_function_by_name(const std::map<int, Assumption> &template_type_id_to_ClassPtr,
                                                          FunctionPtr func,
                                                          const std::string &name_of_function_instance) {
    FunctionPtr instance;
    G->operate_on_function_locking(name_of_function_instance, [&](FunctionPtr &f_inst) {
      if (!f_inst || f_inst->is_template) {
        f_inst = FunctionData::generate_instance_of_template_function(template_type_id_to_ClassPtr, func, name_of_function_instance);
        if (f_inst) {
          if (auto klass = f_inst->class_id) {
            AutoLocker<Lockable *> locker(&(*klass));
            if (f_inst->modifiers.is_instance()) {
              if (auto instance_method = klass->members.get_instance_method(f_inst->local_name())) {
                instance_method->function = f_inst;
              } else {
                klass->members.add_instance_method(f_inst);
              }
            } else {
              if (auto static_method = klass->members.get_static_method(f_inst->local_name())) {
                static_method->function = f_inst;
              } else {
                klass->members.add_static_method(f_inst);
              }
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
        std::map<int, Assumption> template_type_id_to_ClassPtr;
        invoke_name = lambda_class->get_name_of_invoke_function_for_extern(call, current_function, &template_type_id_to_ClassPtr, &template_of_invoke_method);

        instance_of_template_invoke = generate_instance_template_function_by_name(template_type_id_to_ClassPtr, template_of_invoke_method, invoke_name);
        instance_of_template_invoke->instantiation_of_template_function_location = call_arg->get_location();
      } else {
        kphp_assert(lambda_class->members.has_any_instance_method());
        instance_of_template_invoke = lambda_class->members.get_instance_method(ClassData::NAME_OF_INVOKE_METHOD)->function;
        invoke_name = instance_of_template_invoke->name;
      }

      if (instance_of_template_invoke) {
        VertexAdaptor<op_func_ptr> new_call_arg = VertexAdaptor<op_func_ptr>::create(call_arg).set_location(call_arg);
        new_call_arg->func_id = instance_of_template_invoke;
        new_call_arg->set_string(invoke_name);
        call_arg = new_call_arg;
      }
    }
  }

  VertexAdaptor<op_func_call> set_func_id_for_template(FunctionPtr func, VertexAdaptor<op_func_call> call) {
    kphp_assert(func->is_template);

    std::map<int, Assumption> template_type_id_to_ClassPtr;
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
            kphp_error_act(param->has_default_value(), fmt_format("missed {}th argument in function call: {}", i, call->get_string()), return call);
            call_arg = param->default_value();
          }

          Assumption assumption = infer_class_of_expr(current_function, call_arg);

          auto insertion_result = template_type_id_to_ClassPtr.emplace(param->template_type_id, assumption);
          if (!insertion_result.second) {
            const Assumption &previous_assumption = insertion_result.first->second;
            auto prev_name = previous_assumption.as_human_readable();
            auto new_name = assumption.as_human_readable();
            if (prev_name != new_name) {
              std::string error_msg =
                "argument $" + param->var()->get_string() + " of " + func->name +
                " has a type: `" + prev_name + "` but expected type: `" + new_name + "`";

              kphp_error_act(false, error_msg.c_str(), return call);
            }
          }

          name_of_function_instance += FunctionData::encode_template_arg_name(assumption, i);
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

    int min_args = func_args_n - 1; // variadic param may accept 0 args, so subtract 1
    int explicit_args = call_args_n;
    if (func->modifiers.is_instance()) {
      // subtract the implicit $this argument
      --min_args;
      --explicit_args;
    }

    auto variadic_args_start = positional_args_start;
    kphp_assert(func_args_n > 0);

    kphp_error_act(func_args_n - 1 <= call_args_n,
                   fmt_format("Not enough arguments in function call [{} : {}] [found {}] [expected at least {}]",
                              func->file_id->file_name, func->get_human_readable_name(), explicit_args, min_args),
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
            kphp_error_act(false, fmt_format("It's prohibited using something after argument unpacking: `{}`", s), return {});
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
      fmt_format("Call methods with prefix `__` are prohibited: `{}`", *name_of_class_method),
      return false);

    auto klass = infer_class_of_expr(current_function, array_arg->args()[0]).try_as_class();
    if (!klass) {
      return false;
    }

    auto called_method = klass->members.get_instance_method(*name_of_class_method);
    if (!called_method) {
      auto err_msg = "Can't find instance method: " +
                     TermStringFormat::paint(FunctionData::get_human_readable_name(klass->name + "$$" + *name_of_class_method), TermStringFormat::green);
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
    kphp_assert (call->type() == op_func_ptr || call->type() == op_func_call);
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
    bool func_is_not_empty = !func->root->cmd()->empty() || func->local_name() == ClassData::NAME_OF_VIRT_CLONE;
    kphp_error(func->is_extern() || func_is_not_empty, fmt_format("Usage of function with empty body: {}", func->get_human_readable_name()));

    if (auto new_call = process_varargs(call.as<op_func_call>(), func)) {
      call = new_call;
    } else {
      return call;
    }

    const VertexRange call_args = call.as<op_func_call>()->args();
    const VertexRange func_args = func->get_params();
    auto call_args_n = static_cast<int>(call_args.size());
    auto func_args_n = static_cast<int>(func_args.size());

    for (int i = 0; i < std::min(call_args_n, func_args_n); i++) {
      auto param = func_args[i].as<op_func_param>();
      bool is_callback_passed_to_extern = func->is_extern() && param->type_hint && param->type_hint->try_as<TypeHintCallable>();
      auto &call_arg = call_args[i];

      if (!is_callback_passed_to_extern) {
        kphp_error_act(call_arg->type() != op_func_name,
                       fmt_format("Unexpected function pointer: {}", call_arg->get_string()), continue);
        kphp_error_act(call_arg->type() != op_varg,
                       fmt_format("Passed unpacked arguments to a non-vararg function"), continue);

        // for cast params (::: in functions.txt or '@kphp-infer cast') we add
        // conversions automatically (implicit casts), unless the file from where
        // we're calling this function is annotated with strict_types=1
        if (param->is_cast_param && implicit_cast_allowed(current_function->file_id->is_strict_types, param->type_hint)) {
          call_arg = GenTree::conv_to_cast_param(call_arg, param->type_hint, param->var()->ref_flag);
        }

        if (!param->is_callable) {
          continue;
        }

        // handle typed callables, which are interfaces in face
        if (!func->is_template && !func->is_instantiation_of_template_function()) {
          if (auto arg_assum = infer_class_of_expr(current_function, call_arg).try_as_class()) {
            ClassPtr lambda_interface_klass = infer_class_of_expr(func, param->var()).try_as_class();
            auto is_correct_lambda = arg_assum->is_lambda() && lambda_interface_klass && lambda_interface_klass->is_parent_of(arg_assum);
            kphp_error_act(is_correct_lambda, fmt_format("Can't infer lambda from expression passed as argument: {}", param->var()->str_val), return call);
          } else {
            kphp_error_act(false, fmt_format("You must passed lambda as argument: {}", param->var()->str_val), return call);
          }
          continue;
        }

        // handle untyped callables
        auto name_of_fun_for_wrapping_into_lambda = conv_to_func_ptr_name(call_arg);
        if (!name_of_fun_for_wrapping_into_lambda.empty()) {
          FunctionPtr fun_for_wrapping = G->get_function(name_of_fun_for_wrapping_into_lambda);
          kphp_error_act(fun_for_wrapping && fun_for_wrapping->is_required,
                         fmt_format("Can't find function {}, maybe you miss @kphp-required tag", name_of_fun_for_wrapping_into_lambda), continue);

          auto anon_location = call->location;
          call_arg = LambdaGenerator{current_function, anon_location}
            .add_invoke_method_which_call_function(fun_for_wrapping)
            .add_constructor_from_uses()
            .generate_and_require(current_function, instance_of_function_template_stream)
            ->gen_constructor_call_pass_fields_as_args();
        } else if (convert_array_with_instance_to_lambda_class(call_arg, call->get_location())) {
          continue;
        }

        if (!infer_class_of_expr(current_function, call_arg).try_as_class()) {
          std::string fun_name;
          if (auto arr = call_arg.try_as<op_array>()) {
            if (arr->args().size() == 2 && arr->args()[1]->type() == op_string) {
              fun_name = arr->args()[1]->get_string();
            }
          } else if (auto str = call_arg.try_as<op_string>()) {
            fun_name = str->get_string();
          }
          kphp_error(0, fmt_format("Can't find function {}, maybe you miss @kphp-required tag", fun_name));
        }

      } else {    // is_callback_passed_to_extern
        if (!convert_array_with_instance_to_lambda_class(call_arg, call->get_location())) {
          call_arg = conv_to_func_ptr(call_arg);
        }

        instantiate_lambda(call.as<op_func_call>(), call_arg);
      }

    }

    if (func->is_template) {
      call = set_func_id_for_template(func, call.as<op_func_call>());
    }

    if (func->name == "instance_serialize" && call_args.size() == 1) {
      auto klass = infer_class_of_expr(current_function, call_args[0]).try_as_class();
      kphp_error_act(klass, "You may not use instance_serialize with primitives", return call);
      kphp_error(klass->is_serializable, fmt_format("You may not serialize class without @kphp-serializable tag {}", klass->name));
    } else if (func->name == "instance_deserialize" && call_args.size() == 2) {
      auto *class_name = GenTree::get_constexpr_string(call_args[1]);
      kphp_error_act(class_name && !class_name->empty(), "bad second parameter: expected constant nonempty string with class name", return call);

      auto klass = G->get_class(*class_name);
      kphp_error_act(klass, fmt_format("bad second parameter: can't find the class {}", *class_name), return call);

      kphp_error(klass->is_serializable, fmt_format("You may not deserialize class without @kphp-serializable tag {}", klass->name));
    }

    return call;
  }

  // For the vertices like 'fn(...)' and 'new A(...)' associate a real FunctionPtr
  // which will be available via vertex->get_func_id() afterwards.
  // Instance method calls like '$a->fn(...)' were converted to op_func_call 'fn($a, ...)' during the gentree
  // with a special extra_type, so we can deduce the FunctionPtr using the first call argument.
  VertexPtr try_set_func_id(VertexPtr call) {
    FunctionPtr func_id = call->type() == op_func_ptr ? call.as<op_func_ptr>()->func_id : call.as<op_func_call>()->func_id;
    if (func_id) {
      if (func_id->is_lambda_with_uses()) {
        LambdaPtr lambda_class = func_id->class_id.as<LambdaClassData>();
        if (!lambda_class->construct_function->is_template) {
          return call;
        }

        kphp_assert(!lambda_class->construct_function->is_required);

        lambda_class->construct_function->function_in_which_lambda_was_created = current_function;
        call->location.function = current_function;
        lambda_class->construct_function->is_template = false;
        G->require_function(lambda_class->construct_function, instance_of_function_template_stream);
      }

      return call;
    }

    const string &name =
        call->type() == op_func_call && call->extra_type == op_ex_func_call_arrow
        ? resolve_instance_func_name(current_function, call.as<op_func_call>())
        : call->get_string();

    if (auto f = G->get_function(name)) {
      kphp_error(!(f->modifiers.is_static() && f->modifiers.is_abstract()),
                 fmt_format("you may not call abstract static method: {}", f->get_human_readable_name()));
      kphp_error(!f->modifiers.is_instance() || !f->class_id->is_trait(),
                 fmt_format("you may not call a method of trait {}", f->get_human_readable_name()));

      kphp_error(!f->modifiers.is_static() || call->extra_type != op_ex_func_call_arrow, "It's not allowed to call static method through instance var");
      call = set_func_id(call, f);
      if (!f->is_required && f->is_virtual_method && !f->is_template && f->local_name() == ClassData::NAME_OF_INVOKE_METHOD) {
        /**
         * Typed lambda's interfaces should be generated before. If it doesn't happen, they will be created implicitly in resolving the assumptions above.
         * So we have registered invoke methods that are not passed through pipes. Some kphp_asserts will fail due to this.
         * As the result, such functions are passed next and will be thrown away later.
         */
        instance_of_function_template_stream << f;
      }
    } else {
      print_why_cant_set_func_id_error(call, name);
    }

    return call;
  }

  void print_why_cant_set_func_id_error(VertexPtr call, const std::string &unexisting_func_name) {
    if (auto func_call = call.try_as<op_func_call>()) {
      if (call->extra_type == op_ex_func_call_arrow) {
        if (call->get_string() == ClassData::NAME_OF_CONSTRUCT && !call.as<op_func_call>()->args().empty()) {
          if (auto alloc = call.as<op_func_call>()->args()[0].as<op_alloc>()) {
            ClassPtr klass = G->get_class(alloc->allocated_class_name);
            if (klass) {
              kphp_assert(klass->modifiers.is_abstract() || klass->is_trait());
              const char *type_of_incorrect_class = klass->is_trait() ? "trait" : "abstract/interface";
              kphp_error(0, fmt_format("Calling 'new {}()', but this class is {}", klass->name, type_of_incorrect_class));
            } else {
              kphp_error(0, fmt_format("Class {} does not exist", call->get_string()));
            }
            return;
          }
        }
        auto a = infer_class_of_expr(current_function, call.as<op_func_call>()->args()[0]);
        kphp_error(0, fmt_format("Unknown function ->{}() of {}\n", call->get_string(), a.as_human_readable()));
        return;
      }
    }
    kphp_error(0, fmt_format("Unknown function {}()\n", unexisting_func_name));
  }
};

void PreprocessFunctionF::execute(FunctionPtr function, OStreamT &os) {
  if (function->is_template) {
    return;
  }
  DataStream<FunctionPtr> tmp_stream{true};
  PreprocessFunctionPass pass(tmp_stream);

  run_function_pass(function, &pass);

  for (auto fun: tmp_stream.flush()) {
    *os.project_to_nth_data_stream<1>() << fun;
  }

  if (!stage::has_error()) {
    (*os.project_to_nth_data_stream<0>()) << function;
  }
}
