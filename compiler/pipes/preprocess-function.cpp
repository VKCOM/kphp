#include "compiler/pipes/preprocess-function.h"

#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"

class PreprocessFunctionPass : public FunctionPassBase {
private:
  AUTO_PROF (preprocess_function_c);
public:
  DataStream<FunctionPtr> &instance_of_function_template_stream;

  explicit PreprocessFunctionPass(DataStream<FunctionPtr> &instance_of_function_template_stream) :
    instance_of_function_template_stream(instance_of_function_template_stream) {}

  std::string get_description() override {
    return "Preprocess function";
  }

  bool check_function(FunctionPtr function) override {
    return default_check_function(function) && function->type() != FunctionData::func_extern && !function->is_template;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *) {
    if (root->type() == op_function_c) {
      auto new_root = VertexAdaptor<op_string>::create();
      if (stage::get_function_name() != stage::get_file()->main_func_name) {
        new_root->set_string(stage::get_function_name());
      }
      set_location(new_root, root->get_location());
      root = new_root;
    }

    if (root->type() == op_func_call || root->type() == op_func_ptr || root->type() == op_constructor_call) {
      root = try_set_func_id(root);
    }

    return root;
  }

private:
  void instantiate_lambda(VertexAdaptor<op_func_call> call, VertexPtr &call_arg) {
    if (ClassPtr lambda_class = FunctionData::is_lambda(call_arg)) {
      FunctionPtr instance_of_template_invoke;
      std::string invoke_name;

      if (auto template_of_invoke_method = lambda_class->get_template_of_invoke_function()) {
        std::map<int, std::pair<AssumType, ClassPtr>> template_type_id_to_ClassPtr;
        invoke_name = lambda_class->get_name_of_invoke_function_for_extern(call, current_function, &template_type_id_to_ClassPtr, &template_of_invoke_method);

        G->operate_on_function_locking(invoke_name, [&](FunctionPtr &f_inst) {
          if (!f_inst) {
            f_inst = FunctionData::generate_instance_of_template_function(template_type_id_to_ClassPtr, template_of_invoke_method, invoke_name);
            if (f_inst) {
              f_inst->is_required = true;
              f_inst->kphp_required = true;
              instance_of_function_template_stream << f_inst;
              AutoLocker<Lockable *> locker(&(*lambda_class));
              lambda_class->members.add_instance_method(f_inst, access_public);
            }
          }

          if (f_inst) {
            instance_of_template_invoke = f_inst;
            f_inst->is_callback = true;
          }
        });
      } else {
        kphp_assert(lambda_class->members.has_any_instance_method());
        instance_of_template_invoke = lambda_class->members.get_instance_method("__invoke")->function;
        invoke_name = instance_of_template_invoke->name;
      }

      if (instance_of_template_invoke) {
        VertexAdaptor<op_func_ptr> new_call_arg = VertexAdaptor<op_func_ptr>::create(call_arg);
        new_call_arg->set_func_id(instance_of_template_invoke);
        new_call_arg->set_string(invoke_name);
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
          VertexPtr call_arg = i < call->args().size() ? call->args()[i] : param->default_value();

          AssumType assum = infer_class_of_expr(stage::get_function(), call_arg, class_corresponding_to_parameter);

          kphp_assert(assum != assum_unknown);
          auto insertion_result = template_type_id_to_ClassPtr.emplace(param->template_type_id, std::make_pair(assum, class_corresponding_to_parameter));
          if (!insertion_result.second) {
            const std::pair <AssumType, ClassPtr> &previous_assum_and_class = insertion_result.first->second;
            auto wrap_if_array = [](const std::string &s, AssumType assum) {
              return assum == assum_instance_array ? s + "[]" : s;
            };

            std::string error_msg =
              "argument $" + param->var()->get_string() + " of " + func->name +
              " has a type: `" + wrap_if_array(class_corresponding_to_parameter->name, assum) +
              "` but expected type: `" + wrap_if_array(previous_assum_and_class.second->name, previous_assum_and_class.first) + "`";

            kphp_error_act(previous_assum_and_class.second->name == class_corresponding_to_parameter->name, error_msg.c_str(), return {});
            kphp_error_act(previous_assum_and_class.first == assum, error_msg.c_str(), return {});
          }

          if (assum == assum_instance_array) {
            name_of_function_instance += "$arr";
          }

          if (assum != assum_not_instance) {
            name_of_function_instance += "$" + replace_backslashes(class_corresponding_to_parameter->name);
          } else {
            name_of_function_instance += "$" + std::to_string(i) + "not_instance";
          }
        }
      }
    }

    call->set_string(name_of_function_instance);
    call->set_func_id({});

    G->operate_on_function_locking(name_of_function_instance, [&](FunctionPtr &f_inst) {
      if (!f_inst) {
        f_inst = FunctionData::generate_instance_of_template_function(template_type_id_to_ClassPtr, func, name_of_function_instance);
        if (f_inst) {
          f_inst->is_required = true;
          f_inst->kphp_required = true;
          ClassPtr klass = f_inst->class_id;
          if (klass) {
            AutoLocker<Lockable *> locker(&(*klass));
            klass->members.add_instance_method(f_inst, f_inst->access_type);
          }

          instance_of_function_template_stream << f_inst;
        }
      }

      if (f_inst) {
        set_func_id(call, f_inst);
      }
    });

    return call;
  }

  VertexPtr set_func_id(VertexPtr call, FunctionPtr func) {
    kphp_assert (call->type() == op_func_ptr || call->type() == op_func_call || call->type() == op_constructor_call);
    kphp_assert (func);
    kphp_assert (!call->get_func_id() || call->get_func_id() == func);
    if (call->get_func_id() == func) {
      return call;
    }
    //fprintf (stderr, "%s\n", func->name.c_str());

    call->set_func_id(func);
    if (call->type() == op_func_ptr) {
      func->is_callback = true;
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

    VertexAdaptor<meta_op_function> func_root = func->root;
    VertexAdaptor<op_func_param_list> param_list = func_root->params();
    VertexRange call_args = call.as<op_func_call>()->args();
    VertexRange func_args = param_list->params();
    int call_args_n = (int)call_args.size();
    int func_args_n = (int)func_args.size();

    // TODO: why it is here???
    if (func->varg_flag) {
      for (int i = 0; i < call_args_n; i++) {
        kphp_error_act (
          call_args[i]->type() != op_func_name,
          "Unexpected function pointer",
          return VertexPtr()
        );
      }
      vector<VertexPtr> new_call_args;
      if (call_args_n == 1 && call_args[0]->type() == op_varg) {    // для call_user_func_array
        new_call_args.emplace_back(GenTree::conv_to<tp_array>(call_args[0].as<op_varg>()->array()));
      } else {
        // вызов f(1,2,3) для vararg-функций превращаем в f([1,2,3]), т.к. f($VA_LIST)
        // если f инстанс-фукнция, то вызов f(v$this,1,2,3) делаем f(v$this,[1,2,3])
        const vector<VertexPtr> &cur_call_args = call->get_next();
        int rest_start_pos = func->has_implicit_this_arg() ? 1 : 0;
        new_call_args.insert(new_call_args.begin(), cur_call_args.begin(), cur_call_args.begin() + rest_start_pos);
        vector<VertexPtr> remaining_args(cur_call_args.begin() + rest_start_pos, cur_call_args.end());

        auto rest_args_v = VertexAdaptor<op_array>::create(remaining_args);
        rest_args_v->location = call->get_location();
        new_call_args.emplace_back(GenTree::conv_to<tp_array>(rest_args_v));
      }
      auto new_call = VertexAdaptor<op_func_call>::create(new_call_args);
      new_call->copy_location_and_flags(*call);
      new_call->set_func_id(func);
      new_call->str_val = call.as<op_func_call>()->str_val;
      return new_call;
    }

    for (int i = 0; i < std::min(call_args_n, func_args_n); i++) {
      switch (func_args[i]->type()) {
        case op_func_param: {
          if (call_args[i]->type() == op_func_name) {
            string msg = "Unexpected function pointer: " + call_args[i]->get_string();
            kphp_error(false, msg.c_str());
            continue;
          } else if (call_args[i]->type() == op_varg) {
            string msg = "function: `" + func->name + "` must takes variable-length argument list";
            kphp_error_act(false, msg.c_str(), break);
          }

          VertexAdaptor<op_func_param> param = func_args[i];
          if (param->type_rule) {
            call_args[i]->type_rule = param->type_rule;
          } else if (param->type_help != tp_Unknown) {
            call_args[i] = GenTree::conv_to(call_args[i], param->type_help, param->var()->ref_flag);
          }

          break;
        }

        case op_func_param_callback: {
          call_args[i] = conv_to_func_ptr(call_args[i], stage::get_function());
          instantiate_lambda(call.as<op_func_call>(), call_args[i]);
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
    if (call->get_func_id()) {
      return call;
    }

    const string &name =
      call->type() == op_constructor_call
      ? resolve_constructor_func_name(current_function, call)
      : call->type() == op_func_call && call->extra_type == op_ex_func_member
        ? resolve_instance_func_name(current_function, call)
        : call->get_string();

    FunctionPtr f = G->get_function(name);

    if (likely(!!f)) {
      call = set_func_id(call, f);
    } else {
      print_why_cant_set_func_id_error(call, name);
    }

    return call;
  }

  void print_why_cant_set_func_id_error(VertexPtr call, std::string unexisting_func_name) {
    if (call->type() == op_constructor_call) {
      kphp_error(0, format("Calling 'new %s()', but this class is fully static", call->get_string().c_str()));
    } else if (call->type() == op_func_call && call->extra_type == op_ex_func_member) {
      ClassPtr klass;
      infer_class_of_expr(current_function, call.as<op_func_call>()->args()[0], klass);
      kphp_error(0, format("Unknown function ->%s() of %s\n", call->get_c_string(), klass ? klass->name.c_str() : "Unknown class"));
    } else {
      kphp_error(0, format("Unknown function %s()\n", unexisting_func_name.c_str()));
    }
  }
};

void PreprocessFunctionF::execute(FunctionPtr function, OStreamT &os) {
  PreprocessFunctionPass pass(*os.project_to_nth_data_stream<1>());

  run_function_pass(function, &pass);

  if (!stage::has_error() && !function->is_template) {
    (*os.project_to_nth_data_stream<0>()) << function;
  }
}
