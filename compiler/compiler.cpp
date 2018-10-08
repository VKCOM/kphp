#include "compiler/compiler.h"

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <functional>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

#include "common/crc32.h"
#include "common/type_traits/function_traits.h"
#include "common/version-string.h"

#include "compiler/bicycle.h"
#include "compiler/cfg.h"
#include "compiler/code-gen.h"
#include "compiler/compiler-core.h"
#include "compiler/const-manipulations.h"
#include "compiler/data_ptr.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/io.h"
#include "compiler/lexer.h"
#include "compiler/name-gen.h"
#include "compiler/phpdoc.h"
#include "compiler/pipes/analyzer.h"
#include "compiler/pipes/calc-actual-edges.h"
#include "compiler/pipes/calc-bad-vars.h"
#include "compiler/pipes/calc-const-types.h"
#include "compiler/pipes/calc-locations.h"
#include "compiler/pipes/calc-rl.h"
#include "compiler/pipes/calc-val-ref.h"
#include "compiler/pipes/check-access-modifiers.h"
#include "compiler/pipes/check-infered-instances.h"
#include "compiler/pipes/check-instance-props.h"
#include "compiler/pipes/check-nested-foreach.h"
#include "compiler/pipes/check-returns.h"
#include "compiler/pipes/check-ub.h"
#include "compiler/pipes/collect-classes.h"
#include "compiler/pipes/collect-const-vars.h"
#include "compiler/pipes/collect-defines.h"
#include "compiler/pipes/convert-list-assignments.h"
#include "compiler/pipes/create-switch-foreach-vars.h"
#include "compiler/pipes/extract-async.h"
#include "compiler/pipes/extract-resumable-calls.h"
#include "compiler/pipes/file-and-token.h"
#include "compiler/pipes/file-to-tokens.h"
#include "compiler/pipes/filter-only-actually-used.h"
#include "compiler/pipes/final-check.h"
#include "compiler/pipes/load-files.h"
#include "compiler/pipes/optimization.h"
#include "compiler/pipes/parse.h"
#include "compiler/pipes/preprocess-break.h"
#include "compiler/pipes/preprocess-defines.h"
#include "compiler/pipes/preprocess-eq3.h"
#include "compiler/pipes/preprocess-vararg.h"
#include "compiler/pipes/register-defines.h"
#include "compiler/pipes/register-variables.h"
#include "compiler/pipes/split-switch.h"
#include "compiler/pipes/write-files.h"
#include "compiler/scheduler/constructor.h"
#include "compiler/scheduler/one-thread-scheduler.h"
#include "compiler/scheduler/pipe.h"
#include "compiler/scheduler/scheduler.h"
#include "compiler/stage.h"
#include "compiler/type-inferer.h"
#include "compiler/utils.h"

class CollectRequiredCallback {
private:
  DataStream<SrcFilePtr> &file_stream;
  DataStream<FunctionPtr> &function_stream;

public:

  CollectRequiredCallback(DataStream<SrcFilePtr> &file_stream, DataStream<FunctionPtr> &function_stream) :
    file_stream(file_stream),
    function_stream(function_stream) {}

  pair<SrcFilePtr, bool> require_file(const string &file_name, const string &class_context) {
    return G->require_file(file_name, class_context, file_stream);
  }

  void require_function(const string &name) {
    G->require_function(name, function_stream);
  }
};

class CollectRequiredPass : public FunctionPassBase {
private:
  AUTO_PROF (collect_required);
  bool force_func_ptr;
  CollectRequiredCallback callback;

public:
  CollectRequiredPass(DataStream<SrcFilePtr> &file_stream, DataStream<FunctionPtr> &function_stream) :
    force_func_ptr(false),
    callback(file_stream, function_stream) {
  }

  struct LocalT : public FunctionPassBase::LocalT {
    bool saved_force_func_ptr;
  };

  string get_description() {
    return "Collect required";
  }

  void require_class(const string &class_name, const string &context_name) {
    pair<SrcFilePtr, bool> res = callback.require_file(class_name + ".php", context_name);
    kphp_error(res.first, dl_pstr("Class %s not found", class_name.c_str()));
    if (res.second) {
      res.first->req_id = current_function;
    }
  }

  template<class VisitT>
  bool user_recursion(VertexPtr v, LocalT *local __attribute__((unused)), VisitT &visit __attribute__((unused))) {
    if (v->type() == op_function && v.as<op_function>()->name().as<op_func_name>()->get_string() == current_function->name) {
      if (current_function->type() == FunctionData::func_global && !current_function->class_name.empty()) {
        if (!current_function->class_extends.empty()) {
          require_class(resolve_uses(current_function, current_function->class_extends, '/'), current_function->class_context_name);
        }
        if ((current_function->namespace_name + "\\" + current_function->class_name) != current_function->class_context_name) {
          return true;
        }
        if (!current_function->class_extends.empty()) {
          require_class(resolve_uses(current_function, current_function->class_extends, '/'), "");
        }
      }
    }
    return false;
  }

  string get_class_name_for(const string &name, char delim = '$') {
    size_t pos$$ = name.find("::");
    if (pos$$ == string::npos) {
      return "";
    }

    string class_name = name.substr(0, pos$$);
    kphp_assert(!class_name.empty());
    return resolve_uses(current_function, class_name, delim);
  }


  VertexPtr on_enter_vertex(VertexPtr root, LocalT *local) {
    bool new_force_func_ptr = false;
    if (root->type() == op_func_call || root->type() == op_func_name) {
      if (root->extra_type != op_ex_func_member) {
        string name = get_full_static_member_name(current_function, root->get_string(), root->type() == op_func_call);
        callback.require_function(name);
      }
    }

    if (root->type() == op_func_call || root->type() == op_var || root->type() == op_func_name) {
      const string &name = root->get_string();
      const string &class_name = get_class_name_for(name, '/');
      if (!class_name.empty()) {
        require_class(class_name, "");
        string member_name = get_full_static_member_name(current_function, name, root->type() == op_func_call);
        root->set_string(member_name);
      }
    }
    if (root->type() == op_constructor_call) {
      if (likely(!root->type_help)) {     // type_help <=> Memcache | Exception
        const string &class_name = resolve_uses(current_function, root->get_string(), '/');
        require_class(class_name, "");
      }
    }

    if (root->type() == op_func_call) {
      new_force_func_ptr = true;
      const string &name = root->get_string();
      if (name == "func_get_args" || name == "func_get_arg" || name == "func_num_args") {
        current_function->varg_flag = true;
      }
    }

    local->saved_force_func_ptr = force_func_ptr;
    force_func_ptr = new_force_func_ptr;

    return root;
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *local) {
    force_func_ptr = local->saved_force_func_ptr;

    if (root->type() == op_require || root->type() == op_require_once) {
      VertexAdaptor<meta_op_require> require = root;
      for (auto &cur : require->args()) {
        kphp_error_act (cur->type() == op_string, "Not a string in 'require' arguments", continue);
        pair<SrcFilePtr, bool> tmp = callback.require_file(cur->get_string(), "");
        SrcFilePtr file = tmp.first;
        bool required = tmp.second;
        if (required) {
          file->req_id = current_function;
        }

        auto call = VertexAdaptor<op_func_call>::create();
        if (file) {
          call->str_val = file->main_func_name;
          cur = call;
        } else {
          kphp_error (0, dl_pstr("Cannot require [%s]\n", cur->get_string().c_str()));
        }
      }
    }

    return root;
  }
};

class CollectRequiredF {
public:
  using ReadyFunctionPtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<ReadyFunctionPtr, SrcFilePtr, FunctionPtr>;

  void execute(FunctionPtr function, OStreamT &os) {
    auto &ready_function_stream = *os.template project_to_nth_data_stream<0>();
    auto &file_stream = *os.template project_to_nth_data_stream<1>();
    auto &function_stream = *os.template project_to_nth_data_stream<2>();

    CollectRequiredPass pass(file_stream, function_stream);

    run_function_pass(function, &pass);

    if (stage::has_error()) {
      return;
    }

    if (function->type() == FunctionData::func_global && !function->class_name.empty() &&
        (function->namespace_name + "\\" + function->class_name) != function->class_context_name) {
      return;
    }
    ready_function_stream << function;
  }
};

/*** Apply function header ***/
void function_apply_header(FunctionPtr func, VertexAdaptor<meta_op_function> header) {
  VertexAdaptor<meta_op_function> root = func->root;
  func->used_in_source = true;

  kphp_assert (root && header);
  kphp_error_return (
    !func->header,
    dl_pstr("Function [%s]: multiple headers", func->name.c_str())
  );
  func->header = header;

  kphp_error_return (
    !root->type_rule,
    dl_pstr("Function [%s]: type_rule is overided by header", func->name.c_str())
  );
  root->type_rule = header->type_rule;

  kphp_error_return (
    !(!header->varg_flag && func->varg_flag),
    dl_pstr("Function [%s]: varg_flag mismatch with header", func->name.c_str())
  );
  func->varg_flag = header->varg_flag;

  if (!func->varg_flag) {
    VertexAdaptor<op_func_param_list> root_params_vertex = root->params();
    VertexAdaptor<op_func_param_list> header_params_vertex = header->params();
    VertexRange root_params = root_params_vertex->params();
    VertexRange header_params = header_params_vertex->params();

    kphp_error (
      root_params.size() == header_params.size(),
      dl_pstr("Bad header for function [%s]", func->name.c_str())
    );
    int params_n = (int)root_params.size();
    for (int i = 0; i < params_n; i++) {
      kphp_error (
        root_params[i]->size() == header_params[i]->size(),
        dl_pstr(
          "Function [%s]: %dth param has problem with default value",
          func->name.c_str(), i + 1
        )
      );
      kphp_error (
        root_params[i]->type_help == tp_Unknown,
        dl_pstr("Function [%s]: type_help is overrided by header", func->name.c_str())
      );
      root_params[i]->type_help = header_params[i]->type_help;
    }
  }
}

void prepare_function_misc(FunctionPtr func) {
  VertexAdaptor<meta_op_function> func_root = func->root;
  kphp_assert (func_root);
  VertexAdaptor<op_func_param_list> param_list = func_root->params();
  VertexRange params = param_list->args();
  int param_n = (int)params.size();
  bool was_default = false;
  func->min_argn = param_n;
  for (int i = 0; i < param_n; i++) {
    if (func->varg_flag) {
      kphp_error (params[i].as<meta_op_func_param>()->var()->ref_flag == false,
                  "Reference arguments are not supported in varg functions");
    }
    if (params[i].as<meta_op_func_param>()->has_default_value()) {
      if (!was_default) {
        was_default = true;
        func->min_argn = i;
      }
      if (func->type() == FunctionData::func_local) {
        kphp_error (params[i].as<meta_op_func_param>()->var()->ref_flag == false,
                    dl_pstr("Default value in reference function argument [function = %s]", func->name.c_str()));
      }
    } else {
      kphp_error (!was_default,
                  dl_pstr("Default value expected [function = %s] [param_i = %d]",
                          func->name.c_str(), i));
    }
  }
}

/*
 * Анализ @kphp-infer, @kphp-inline и других @kphp-* внутри phpdoc над функцией f
 * Сейчас пока что есть костыль: @kphp-required анализируется всё равно в gentree
 */
void parse_and_apply_function_kphp_phpdoc(FunctionPtr f) {
  if (f->phpdoc_token == nullptr || f->phpdoc_token->type() != tok_phpdoc_kphp) {
    return;   // обычный phpdoc, без @kphp нотаций, тут не парсим; если там инстансы, распарсится по требованию
  }

  int infer_type = 0;
  vector<VertexPtr> prepend_cmd;
  VertexPtr func_params = f->root.as<op_function>()->params();

  std::unordered_map<std::string, VertexAdaptor<op_func_param>> name_to_function_param;
  for (auto param_ptr : *func_params) {
    VertexAdaptor<op_func_param> param = param_ptr.as<op_func_param>();
    name_to_function_param.emplace("$" + param->var()->get_string(), param);
  }

  std::size_t id_of_kphp_template = 0;
  stage::set_location(f->root->get_location());
  for (auto &tag : parse_php_doc(f->phpdoc_token->str_val)) {
    stage::set_line(tag.line_num);
    switch (tag.type) {
      case php_doc_tag::kphp_inline: {
        f->root->inline_flag = true;
        continue;
      }

      case php_doc_tag::kphp_sync: {
        f->should_be_sync = true;
        continue;
      }

      case php_doc_tag::kphp_infer: {
        kphp_error(infer_type == 0, "Double kphp-infer tag found");
        std::istringstream is(tag.value);
        string token;
        while (is >> token) {
          if (token == "check") {
            infer_type |= 1;
          } else if (token == "hint") {
            infer_type |= 2;
          } else if (token == "cast") {
            infer_type |= 4;
          } else {
            kphp_error(0, dl_pstr("Unknown kphp-infer tag type '%s'", token.c_str()));
          }
        }
        break;
      }

      case php_doc_tag::kphp_disable_warnings: {
        std::istringstream is(tag.value);
        string token;
        while (is >> token) {
          if (!f->disabled_warnings.insert(token).second) {
            kphp_warning(dl_pstr("Warning '%s' has been disabled twice", token.c_str()));
          }
        }
        break;
      }

      case php_doc_tag::kphp_required: {
        f->kphp_required = true;
        break;
      }

      case php_doc_tag::param: {
        if (infer_type) {
          kphp_error_act(!name_to_function_param.empty(), "Too many @param tags", return);
          std::istringstream is(tag.value);
          string type_help, var_name;
          kphp_error(is >> type_help, "Failed to parse @param tag");
          kphp_error(is >> var_name, "Failed to parse @param tag");

          auto func_param_it = name_to_function_param.find(var_name);

          kphp_error_act(func_param_it != name_to_function_param.end(), dl_pstr("@param tag var name mismatch. found %s.", var_name.c_str()), return);

          VertexAdaptor<op_func_param> cur_func_param = func_param_it->second;
          VertexAdaptor<op_var> var = cur_func_param->var().as<op_var>();
          name_to_function_param.erase(func_param_it);

          kphp_error(var, "Something strange happened during @param parsing");

          VertexPtr doc_type = phpdoc_parse_type(type_help, f);
          kphp_error(doc_type,
                     dl_pstr("Failed to parse type '%s'", type_help.c_str()));
          if (infer_type & 1) {
            auto doc_type_check = VertexAdaptor<op_lt_type_rule>::create(doc_type);
            auto doc_rule_var = VertexAdaptor<op_var>::create();
            doc_rule_var->str_val = var->str_val;
            doc_rule_var->type_rule = doc_type_check;
            set_location(doc_rule_var, f->root->location);
            prepend_cmd.push_back(doc_rule_var);
          }
          if (infer_type & 2) {
            auto doc_type_check = VertexAdaptor<op_common_type_rule>::create(doc_type);
            auto doc_rule_var = VertexAdaptor<op_var>::create();
            doc_rule_var->str_val = var->str_val;
            doc_rule_var->type_rule = doc_type_check;
            set_location(doc_rule_var, f->root->location);
            prepend_cmd.push_back(doc_rule_var);
          }
          if (infer_type & 4) {
            kphp_error(doc_type->type() == op_type_rule,
                       dl_pstr("Too hard rule '%s' for cast", type_help.c_str()));
            kphp_error(doc_type.as<op_type_rule>()->args().empty(),
                       dl_pstr("Too hard rule '%s' for cast", type_help.c_str()));
            kphp_error(cur_func_param->type_help == tp_Unknown,
                       dl_pstr("Duplicate type rule for argument '%s'", var_name.c_str()));
            cur_func_param->type_help = doc_type.as<op_type_rule>()->type_help;
          }
        }
        break;
      }

      case php_doc_tag::kphp_template: {
        f->is_template = true;
        for (const auto &var_name : split_skipping_delimeters(tag.value, ", ")) {
          if (var_name[0] != '$') {
            break;
          }

          auto func_param_it = name_to_function_param.find(var_name);
          kphp_error_return(func_param_it != name_to_function_param.end(), dl_pstr("@kphp-template tag var name mismatch. found %s.", var_name.c_str()));

          VertexAdaptor<op_func_param> cur_func_param = func_param_it->second;
          name_to_function_param.erase(func_param_it);

          cur_func_param->template_type_id = id_of_kphp_template;
        }
        id_of_kphp_template++;

        break;
      }

      default:
        break;
    }
  }
  size_t cnt_left_params_without_tag = f->is_instance_function() && !f->is_constructor() ? 1 : 0;  // пропускаем неявный this
  if (infer_type && name_to_function_param.size() != cnt_left_params_without_tag) {
    std::string err_msg = "Not enough @param tags. Need tags for function arguments:\n";
    for (auto name_and_function_param : name_to_function_param) {
      err_msg += name_and_function_param.first;
      err_msg += "\n";
    }
    stage::set_location(f->root->get_location());
    kphp_error(false, err_msg.c_str());
  }

  // из { cmd } делаем { prepend_cmd; cmd }
  if (!prepend_cmd.empty()) {
    for (auto i : *f->root.as<op_function>()->cmd()) {
      prepend_cmd.push_back(i);
    }
    auto new_cmd = VertexAdaptor<op_seq>::create(prepend_cmd);
    ::set_location(new_cmd, f->root->location);
    f->root.as<op_function>()->cmd() = new_cmd;
  }
}

void prepare_function(FunctionPtr function) {
  parse_and_apply_function_kphp_phpdoc(function);
  prepare_function_misc(function);

  VertexPtr header = G->get_extern_func_header(function->name);
  if (header) {
    function_apply_header(function, header);
  }
  if (function->root && function->root->varg_flag) {
    function->varg_flag = true;
  }
}

class PrepareFunctionF {
public:

  void execute(FunctionPtr function, DataStream<FunctionPtr> &os) {
    stage::set_name("Prepare function");
    stage::set_function(function);
    kphp_assert (function);

    prepare_function(function);

    if (stage::has_error()) {
      return;
    }

    os << function;
  }
};

/*** Replace __FUNCTION__ ***/
/*** Set function_id for all function calls ***/
class PreprocessFunctionCPass : public FunctionPassBase {
private:
  AUTO_PROF (preprocess_function_c);
public:
  using InstanceOfFunctionTemplatePtr = FunctionPtr;
  using OStreamT = MultipleDataStreams<FunctionPtr, InstanceOfFunctionTemplatePtr>;
  OStreamT &os;

  explicit PreprocessFunctionCPass(OStreamT &os) :
    os(os) {}

  std::string get_description() override {
    return "Preprocess function C";
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
      VertexPtr args;
      if (call_args_n == 1 && call_args[0]->type() == op_varg) {
        args = call_args[0].as<op_varg>()->expr();
      } else {
        auto new_args = VertexAdaptor<op_array>::create(call->get_next());
        new_args->location = call->get_location();
        args = new_args;
      }
      vector<VertexPtr> tmp(1, GenTree::conv_to<tp_array>(args));
      auto new_call = VertexAdaptor<op_func_call>::create(tmp);
      new_call->copy_location_and_flags(*call);
      new_call->set_func_id(func);
      new_call->str_val = call.as<op_func_call>()->str_val;
      return new_call;
    }

    std::map<int, std::pair<AssumType, ClassPtr>> template_type_id_to_ClassPtr;
    std::string name_of_function_instance = func->name + "$instance";
    for (int i = 0; i < call_args_n; i++) {
      if (i < func_args_n) {
        if (func_args[i]->type() == op_func_param) {
          if (call_args[i]->type() == op_func_name) {
            string msg = "Unexpected function pointer: " + call_args[i]->get_string();
            kphp_error(false, msg.c_str());
            continue;
          } else if (call_args[i]->type() == op_varg) {
            string msg = "function: `" + func->name + "` must takes variable-length argument list";
            kphp_error_act(false, msg.c_str(), break);
          }
          VertexAdaptor<op_func_param> param = func_args[i];
          if (param->type_help != tp_Unknown) {
            call_args[i] = GenTree::conv_to(call_args[i], param->type_help, param->var()->ref_flag);
          }

          if (param->template_type_id >= 0) {
            kphp_assert(func->is_template);
            ClassPtr class_corresponding_to_parameter;
            AssumType assum = infer_class_of_expr(stage::get_function(), call_args[i], class_corresponding_to_parameter);

            {
              std::string error_msg = "function templates support only instances as argument value; " + func->name + "; " + param->var()
                                                                                                                                 ->get_string() + " argument";
              kphp_error_act(vk::any_of_equal(assum, assum_instance, assum_instance_array), error_msg.c_str(), return {});
            }

            auto insertion_result = template_type_id_to_ClassPtr.emplace(param->template_type_id, std::make_pair(assum, class_corresponding_to_parameter));
            if (!insertion_result.second) {
              const std::pair<AssumType, ClassPtr> &previous_assum_and_class = insertion_result.first->second;
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

            name_of_function_instance += "$" + replace_backslashes(class_corresponding_to_parameter->name);
          }
        } else if (func_args[i]->type() == op_func_param_callback) {
          call_args[i] = conv_to_func_ptr(call_args[i], stage::get_function());
          kphp_error (call_args[i]->type() == op_func_ptr, "Function pointer expected");
        } else {
          kphp_fail();
        }
      }
    }

    if (func->is_template) {
      call->set_string(name_of_function_instance);
      call->set_func_id({});

      G->operate_on_function_locking(name_of_function_instance, [&](FunctionPtr &f_inst) {
        if (!f_inst) {
          f_inst = generate_instance_of_template_function(template_type_id_to_ClassPtr, func, name_of_function_instance);
          f_inst->is_required = true;
          (*os.project_to_nth_data_stream<1>()) << f_inst;
        }

        set_func_id(call, f_inst);
      });
    }

    return call;
  }

  FunctionPtr generate_instance_of_template_function(const std::map<int, std::pair<AssumType, ClassPtr>> &template_type_id_to_ClassPtr,
                                                     FunctionPtr func,
                                                     const std::string &name_of_function_instance) {
    VertexAdaptor<op_func_param_list> param_list = func->root.as<meta_op_function>()->params();
    VertexRange func_args = param_list->params();
    size_t func_args_n = func_args.size();

    FunctionPtr new_function(new FunctionData());
    auto new_func_root =  func->root.as<op_function>().clone();

    for (auto id_classPtr_it : template_type_id_to_ClassPtr) {
      const std::pair<AssumType, ClassPtr> &assum_and_class = id_classPtr_it.second;
      for (size_t i = 0; i < func_args_n; ++i) {
        VertexAdaptor<op_func_param> param = func_args[i];
        if (param->template_type_id == id_classPtr_it.first) {
          new_function->assumptions.emplace_back(assum_and_class.first, param->var()->get_string(), assum_and_class.second);
          new_function->assumptions_inited_args = 2;

          new_func_root->params()->ith(i).as<op_func_param>()->template_type_id = -1;
        }
      }
    }

    new_func_root->name()->set_string(name_of_function_instance);

    new_function->root = new_func_root;
    new_function->root->set_func_id(new_function);
    new_function->is_required = true;
    new_function->type() = func->type();
    new_function->file_id = func->file_id;
    new_function->class_id = func->class_id;
    new_function->varg_flag = func->varg_flag;
    new_function->tinf_state = func->tinf_state;
    new_function->const_data = func->const_data;
    new_function->phpdoc_token = func->phpdoc_token;
    new_function->min_argn = func->min_argn;
    new_function->is_extern = func->is_extern;
    new_function->used_in_source = func->used_in_source;
    new_function->kphp_required = true;
    new_function->namespace_name = func->namespace_name;
    new_function->class_name = func->class_name;
    new_function->class_context_name = func->class_context_name;
    new_function->class_extends = func->class_extends;
    new_function->access_type = func->access_type;
    new_function->namespace_uses = func->namespace_uses;
    new_function->is_template = false;
    new_function->name = name_of_function_instance;

    std::function<void(VertexPtr, FunctionPtr)> set_location_for_all = [&set_location_for_all](VertexPtr root, FunctionPtr function_location) {
      root->location.function = function_location;
      for (VertexPtr &v : *root) {
        set_location_for_all(v, function_location);
      }
    };
    set_location_for_all(new_func_root, new_function);

    return new_function;
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
      f->is_required = true;
      call = set_func_id(call, f);
    } else {
      print_why_cant_set_func_id_error(call, name);
    }

    return call;
  }

  void print_why_cant_set_func_id_error(VertexPtr call, std::string unexisting_func_name) {
    if (call->type() == op_constructor_call) {
      kphp_error(0, dl_pstr("Calling 'new %s()', but this class is fully static", call->get_string().c_str()));
    } else if (call->type() == op_func_call && call->extra_type == op_ex_func_member) {
      ClassPtr klass;
      infer_class_of_expr(current_function, call.as<op_func_call>()->args()[0], klass);
      kphp_error(0, dl_pstr("Unknown function ->%s() of %s\n", call->get_string().c_str(), klass ? klass->name.c_str() : "Unknown class"));
    } else {
      kphp_error(0, dl_pstr("Unknown function %s()\n", unexisting_func_name.c_str()));
    }
  }
};

class PreprocessFunctionF {
public:
  void execute(FunctionPtr function, PreprocessFunctionCPass::OStreamT &os) {
    PreprocessFunctionCPass pass(os);

    pass.init();
    if (pass.on_start(function)) {
      PreprocessFunctionCPass::LocalT local;
      function->root = run_function_pass(function->root, &pass, &local);
      pass.on_finish();
    }

    if (!stage::has_error() && !function->is_template) {
      (*os.project_to_nth_data_stream<0>()) << function;
    }
  }
};


/*** Check function calls ***/
class CheckFunctionCallsPass : public FunctionPassBase {
private:
  AUTO_PROF (check_function_calls);
public:
  string get_description() {
    return "Check function calls";
  }

  bool check_function(FunctionPtr function) {
    return default_check_function(function) && function->root->type() == op_function;
  }

  bool on_start(FunctionPtr function) {
    if (!FunctionPassBase::on_start(function)) {
      return false;
    }
    return true;
  }

  void check_func_call(VertexPtr call) {
    FunctionPtr f = call->get_func_id();
    kphp_assert (f);
    kphp_error_return (f->root, dl_pstr("Function [%s] undeclared", f->name.c_str()));

    if (call->type() == op_func_ptr) {
      return;
    }

    VertexRange func_params = f->root.as<meta_op_function>()->params().
      as<op_func_param_list>()->params();

    if (f->varg_flag) {
      return;
    }

    VertexRange call_params = call->type() == op_constructor_call ? call.as<op_constructor_call>()->args()
                                                                  : call.as<op_func_call>()->args();
    int func_params_n = (int)func_params.size(), call_params_n = (int)call_params.size();

    kphp_error_return (
      call_params_n >= f->min_argn,
      dl_pstr("Not enough arguments in function [%s:%s] [found %d] [expected at least %d]",
              f->file_id->file_name.c_str(), f->name.c_str(), call_params_n, f->min_argn)
    );

    kphp_error (
      call_params.begin() == call_params.end() || call_params[0]->type() != op_varg,
      dl_pstr(
        "call_user_func_array is used for function [%s:%s]",
        f->file_id->file_name.c_str(), f->name.c_str()
      )
    );

    kphp_error_return (
      func_params_n >= call_params_n,
      dl_pstr(
        "Too much arguments in function [%s:%s] [found %d] [expected %d]",
        f->file_id->file_name.c_str(), f->name.c_str(), call_params_n, func_params_n
      )
    );
    for (int i = 0; i < call_params_n; i++) {
      if (func_params[i]->type() == op_func_param_callback) {
        kphp_error_act (
          call_params[i]->type() != op_string,
          "Can't use a string as callback function's name",
          continue
        );
        kphp_error_act (
          call_params[i]->type() == op_func_ptr,
          "Function pointer expected",
          continue
        );

        FunctionPtr func_ptr = call_params[i]->get_func_id();

        kphp_error_act (
          func_ptr->root,
          dl_pstr("Unknown callback function [%s]", func_ptr->name.c_str()),
          continue
        );
        VertexRange cur_params = func_ptr->root.as<meta_op_function>()->params().
          as<op_func_param_list>()->params();
        kphp_error (
          (int)cur_params.size() == func_params[i].as<op_func_param_callback>()->param_cnt,
          "Wrong callback arguments count"
        );
        for (int j = 0; j < (int)cur_params.size(); j++) {
          kphp_error (cur_params[j]->type() == op_func_param,
                      "Callback function with callback parameter");
          kphp_error (cur_params[j].as<op_func_param>()->var()->ref_flag == 0,
                      "Callback function with reference parameter");
        }
      } else {
        kphp_error (call_params[i]->type() != op_func_ptr, "Unexpected function pointer");
      }
    }
  }

  VertexPtr on_enter_vertex(VertexPtr v, LocalT *local __attribute__((unused))) {
    if (v->type() == op_func_ptr || v->type() == op_func_call || v->type() == op_constructor_call) {
      check_func_call(v);
    }
    return v;
  }
};


/*** Control Flow Graph ***/
class CFGCallback : public cfg::CFGCallbackBase {
private:
  FunctionPtr function;

  vector<VertexPtr> uninited_vars;
  vector<VarPtr> todo_var;
  vector<vector<vector<VertexPtr>>> todo_parts;
public:
  void set_function(FunctionPtr new_function) {
    function = new_function;
  }

  void split_var(VarPtr var, vector<vector<VertexPtr>> &parts) {
    assert (var->type() == VarData::var_local_t || var->type() == VarData::var_param_t);
    int parts_size = (int)parts.size();
    if (parts_size == 0) {
      if (var->type() == VarData::var_local_t) {
        function->local_var_ids.erase(
          std::find(
            function->local_var_ids.begin(),
            function->local_var_ids.end(),
            var));
      }
      return;
    }
    assert (parts_size > 1);

    for (int i = 0; i < parts_size; i++) {
      string new_name = var->name + "$v_" + int_to_str(i);
      VarPtr new_var = G->create_var(new_name, var->type());

      for (int j = 0; j < (int)parts[i].size(); j++) {
        VertexPtr v = parts[i][j];
        v->set_var_id(new_var);
      }

      VertexRange params = function->root.
        as<meta_op_function>()->params().
                                     as<op_func_param_list>()->args();
      if (var->type() == VarData::var_local_t) {
        new_var->type() = VarData::var_local_t;
        function->local_var_ids.push_back(new_var);
      } else if (var->type() == VarData::var_param_t) {
        bool was_var = std::find(
          parts[i].begin(),
          parts[i].end(),
          params[var->param_i].as<op_func_param>()->var()
        ) != parts[i].end();

        if (was_var) { //union of part that contains function argument
          new_var->type() = VarData::var_param_t;
          new_var->param_i = var->param_i;
          new_var->init_val = var->init_val;
          function->param_ids[var->param_i] = new_var;
        } else {
          new_var->type() = VarData::var_local_t;
          function->local_var_ids.push_back(new_var);
        }
      } else {
        kphp_fail();
      }

    }

    if (var->type() == VarData::var_local_t) {
      vector<VarPtr>::iterator tmp = std::find(function->local_var_ids.begin(), function->local_var_ids.end(), var);
      if (function->local_var_ids.end() != tmp) {
        function->local_var_ids.erase(tmp);
      } else {
        kphp_fail();
      }
    }

    todo_var.push_back(var);

    //it could be simple std::move
    todo_parts.push_back(vector<vector<VertexPtr>>());
    std::swap(todo_parts.back(), parts);
  }

  void unused_vertices(vector<VertexPtr *> &v) {
    for (auto i: v) {
      auto empty = VertexAdaptor<op_empty>::create();
      *i = empty;
    }
  }

  FunctionPtr get_function() {
    return function;
  }

  void uninited(VertexPtr v) {
    if (v && v->type() == op_var && v->extra_type != op_ex_var_superlocal && v->extra_type != op_ex_var_this) {
      uninited_vars.push_back(v);
      v->get_var_id()->set_uninited_flag(true);
    }
  }

  void check_uninited() {
    for (int i = 0; i < (int)uninited_vars.size(); i++) {
      VertexPtr v = uninited_vars[i];
      VarPtr var = v->get_var_id();
      if (tinf::get_type(v)->ptype() == tp_var) {
        continue;
      }

      stage::set_location(v->get_location());
      kphp_warning (dl_pstr("Variable [%s] may be used uninitialized", var->name.c_str()));
    }
  }

  VarPtr merge_vars(vector<VarPtr> vars, const string &new_name) {
    VarPtr new_var = G->create_var(new_name, VarData::var_unknown_t);;
    //new_var->tinf = vars[0]->tinf; //hack, TODO: fix it
    new_var->tinf_node.copy_type_from(tinf::get_type(vars[0]));

    int param_i = -1;
    for (VarPtr var : vars) {
      if (var->type() == VarData::var_param_t) {
        param_i = var->param_i;
      } else if (var->type() == VarData::var_local_t) {
        //FIXME: remember to remove all unused variables
        //func->local_var_ids.erase (*i);
        vector<VarPtr>::iterator tmp = std::find(function->local_var_ids.begin(), function->local_var_ids.end(), var);
        if (function->local_var_ids.end() != tmp) {
          function->local_var_ids.erase(tmp);
        } else {
          kphp_fail();
        }

      } else {
        assert (0 && "unreachable");
      }
    }
    if (param_i != -1) {
      new_var->type() = VarData::var_param_t;
      function->param_ids[param_i] = new_var;
    } else {
      new_var->type() = VarData::var_local_t;
      function->local_var_ids.push_back(new_var);
    }

    return new_var;
  }


  struct MergeData {
    int id;
    VarPtr var;

    MergeData(int id, VarPtr var) :
      id(id),
      var(var) {
    }
  };

  static bool cmp_merge_data(const MergeData &a, const MergeData &b) {
    return type_out(tinf::get_type(a.var)) <
           type_out(tinf::get_type(b.var));
  }

  static bool eq_merge_data(const MergeData &a, const MergeData &b) {
    return type_out(tinf::get_type(a.var)) ==
           type_out(tinf::get_type(b.var));
  }

  void merge_same_type() {
    int todo_n = (int)todo_parts.size();
    for (int todo_i = 0; todo_i < todo_n; todo_i++) {
      vector<vector<VertexPtr>> &parts = todo_parts[todo_i];

      int n = (int)parts.size();
      vector<MergeData> to_merge;
      for (int i = 0; i < n; i++) {
        to_merge.push_back(MergeData(i, parts[i][0]->get_var_id()));
      }
      sort(to_merge.begin(), to_merge.end(), cmp_merge_data);

      vector<int> ids;
      int merge_id = 0;
      for (int i = 0; i <= n; i++) {
        if (i == n || (i > 0 && !eq_merge_data(to_merge[i - 1], to_merge[i]))) {
          vector<VarPtr> vars;
          for (int id : ids) {
            vars.push_back(parts[id][0]->get_var_id());
          }
          string new_name = vars[0]->name;
          int name_i = (int)new_name.size() - 1;
          while (new_name[name_i] != '$') {
            name_i--;
          }
          new_name.erase(name_i);
          new_name += "$v";
          new_name += int_to_str(merge_id++);

          VarPtr new_var = merge_vars(vars, new_name);
          for (int id : ids) {
            for (VertexPtr v : parts[id]) {
              v->set_var_id(new_var);
            }
          }

          ids.clear();
        }
        if (i == n) {
          break;
        }
        ids.push_back(to_merge[i].id);
      }
    }
  }
};


class CFGBeginF {
public:
  void execute(FunctionPtr function, DataStream<FunctionAndCFG> &os) {
    AUTO_PROF (CFG);
    stage::set_name("Calc control flow graph");
    stage::set_function(function);

    cfg::CFG cfg;
    CFGCallback *callback = new CFGCallback();
    callback->set_function(function);
    cfg.run(callback);

    if (stage::has_error()) {
      return;
    }

    os << FunctionAndCFG(function, callback);
  }
};

/*** Type inference ***/
class CollectMainEdgesCallback : public CollectMainEdgesCallbackBase {
private:
  tinf::TypeInferer *inferer_;

public:
  CollectMainEdgesCallback(tinf::TypeInferer *inferer) :
    inferer_(inferer) {
  }

  tinf::Node *node_from_rvalue(const RValue &rvalue) {
    if (rvalue.node == nullptr) {
      kphp_assert (rvalue.type != nullptr);
      return new tinf::TypeNode(rvalue.type);
    }

    return rvalue.node;
  }

  virtual void require_node(const RValue &rvalue) {
    if (rvalue.node != nullptr) {
      inferer_->add_node(rvalue.node);
    }
  }

  virtual void create_set(const LValue &lvalue, const RValue &rvalue) {
    tinf::Edge *edge = new tinf::Edge();
    edge->from = lvalue.value;
    edge->from_at = lvalue.key;
    edge->to = node_from_rvalue(rvalue);
    inferer_->add_edge(edge);
    inferer_->add_node(edge->from);
  }

  virtual void create_less(const RValue &lhs, const RValue &rhs) {
    tinf::Node *a = node_from_rvalue(lhs);
    tinf::Node *b = node_from_rvalue(rhs);
    inferer_->add_node(a);
    inferer_->add_node(b);
    inferer_->add_restriction(new RestrictionLess(a, b));
  }

  virtual void create_isset_check(const RValue &rvalue) {
    tinf::Node *a = node_from_rvalue(rvalue);
    inferer_->add_node(a);
    inferer_->add_restriction(new RestrictionIsset(a));
  }
};

class TypeInfererF {
  //TODO: extract pattern
private:
public:
  TypeInfererF() {
    tinf::register_inferer(new tinf::TypeInferer());
  }

  void execute(FunctionAndCFG input, DataStream<FunctionAndCFG> &os) {
    AUTO_PROF (tinf_infer_gen_dep);
    CollectMainEdgesCallback callback(tinf::get_inferer());
    CollectMainEdgesPass pass(&callback);
    run_function_pass(input.function, &pass);
    os << input;
  }

  void on_finish(DataStream<FunctionAndCFG> &os __attribute__((unused))) {
    //FIXME: rebalance Queues
    vector<Task *> tasks = tinf::get_inferer()->get_tasks();
    for (int i = 0; i < (int)tasks.size(); i++) {
      register_async_task(tasks[i]);
    }
  }
};

class TypeInfererEndF {
private:
  DataStream<FunctionAndCFG> tmp_stream;
public:
  TypeInfererEndF() {
    tmp_stream.set_sink(true);
  }

  void execute(FunctionAndCFG input, DataStream<FunctionAndCFG> &os __attribute__((unused))) {
    tmp_stream << input;
  }

  void on_finish(DataStream<FunctionAndCFG> &os) {
    tinf::get_inferer()->check_restrictions();
    tinf::get_inferer()->finish();

    for (auto &f_and_cfg : tmp_stream.get_as_vector()) {
      os << f_and_cfg;
    }
  }
};

/*** Control flow graph. End ***/
class CFGEndF {
public:

  void execute(FunctionAndCFG data, DataStream<FunctionPtr> &os) {
    AUTO_PROF (CFG_End);
    stage::set_name("Control flow graph. End");
    stage::set_function(data.function);
    if (G->env().get_warnings_level() >= 1) {
      data.callback->check_uninited();
    }
    data.callback->merge_same_type();
    delete data.callback;

    if (stage::has_error()) {
      return;
    }

    os << data.function;
  }
};

// todo почему-то удалилось из server
void prepare_generate_class(ClassPtr klass __attribute__((unused))) {
}

template<class OutputStream>
class WriterCallback : public WriterCallbackBase {
private:
  OutputStream &os;
public:
  WriterCallback(OutputStream &os, const string dir __attribute__((unused)) = "./") :
    os(os) {
  }

  void on_end_write(WriterData *data) {
    if (stage::has_error()) {
      return;
    }

    WriterData *data_copy = new WriterData();
    data_copy->swap(*data);
    data_copy->calc_crc();
    os << data_copy;
  }
};

class CodeGenF {
  //TODO: extract pattern
private:
  DataStream<FunctionPtr> tmp_stream;
  map<string, long long> subdir_hash;

public:
  CodeGenF() {
    tmp_stream.set_sink(true);
  }

  void execute(FunctionPtr input, DataStream<WriterData *> &os __attribute__((unused))) {
    tmp_stream << input;
  }

  int calc_count_of_parts(size_t cnt_global_vars) {
    return cnt_global_vars > 1000 ? 64 : 1;
  }

  void on_finish(DataStream<WriterData *> &os) {
    AUTO_PROF (code_gen);

    stage::set_name("GenerateCode");
    stage::set_file(SrcFilePtr());
    stage::die_if_global_errors();

    vector<FunctionPtr> xall = tmp_stream.get_as_vector();
    sort(xall.begin(), xall.end());
    const vector<ClassPtr> &all_classes = G->get_classes();

    //TODO: delete W_ptr
    CodeGenerator *W_ptr = new CodeGenerator();
    CodeGenerator &W = *W_ptr;

    if (G->env().get_use_safe_integer_arithmetic()) {
      W.use_safe_integer_arithmetic();
    }

    G->init_dest_dir();
    G->load_index();

    W.init(new WriterCallback<DataStream<WriterData *>>(os));

    //TODO: parallelize;
    for (const auto &fun : xall) {
      prepare_generate_function(fun);
    }
    for (const auto &c : all_classes) {
      if (c && c->was_constructor_invoked) {
        prepare_generate_class(c);
      }
    }

    vector<SrcFilePtr> main_files = G->get_main_files();
    vector<FunctionPtr> all_functions;
    vector<FunctionPtr> source_functions;
    for (int i = 0; i < (int)xall.size(); i++) {
      FunctionPtr function = xall[i];
      if (function->used_in_source) {
        source_functions.push_back(function);
      }
      if (function->type() == FunctionData::func_extern) {
        continue;
      }
      all_functions.push_back(function);
      W << Async(FunctionH(function));
      W << Async(FunctionCpp(function));
    }

    for (const auto &c : all_classes) {
      if (c && c->was_constructor_invoked) {
        W << Async(ClassDeclaration(c));
      }
    }

    //W << Async (XmainCpp());
    W << Async(InitScriptsH());
    for (const auto &main_file : main_files) {
      W << Async(DfsInit(main_file));
    }
    W << Async(InitScriptsCpp(/*std::move*/main_files, source_functions, all_functions));

    vector<VarPtr> vars = G->get_global_vars();
    int parts_cnt = calc_count_of_parts(vars.size());
    W << Async(VarsCpp(vars, parts_cnt));

    write_hashes_of_subdirs_to_dep_files(W);

    write_tl_schema(W);
  }

private:
  void prepare_generate_function(FunctionPtr func) {
    string file_name = func->name;
    std::replace(file_name.begin(), file_name.end(), '$', '@');

    string file_subdir = func->file_id->short_file_name;

    func->header_name = file_name + ".h";
    func->subdir = get_subdir(file_subdir);

    recalc_hash_of_subdirectory(func->subdir, func->header_name);

    if (!func->root->inline_flag) {
      func->src_name = file_name + ".cpp";
      func->src_full_name = func->subdir + "/" + func->src_name;

      recalc_hash_of_subdirectory(func->subdir, func->src_name);
    }

    func->header_full_name = func->subdir + "/" + func->header_name;

    my_unique(&func->static_var_ids);
    my_unique(&func->global_var_ids);
    my_unique(&func->header_global_var_ids);
    my_unique(&func->local_var_ids);
  }

  string get_subdir(const string &base) {
    int func_hash = hash(base);
    int bucket = func_hash % 100;

    return string("o_") + int_to_str(bucket);
  }

  void recalc_hash_of_subdirectory(const string &subdir, const string &file_name) {
    long long &cur_hash = subdir_hash[subdir];
    cur_hash = cur_hash * 987654321 + hash(file_name);
  }

  void write_hashes_of_subdirs_to_dep_files(CodeGenerator &W) {
    for (const auto &dir_and_hash : subdir_hash) {
      W << OpenFile("_dep.cpp", dir_and_hash.first, false);
      char tmp[100];
      sprintf(tmp, "%llx", dir_and_hash.second);
      W << "//" << (const char *)tmp << NL;
      W << CloseFile();
    }
  }

  void write_tl_schema(CodeGenerator &W) {
    string schema;
    int schema_length = -1;
    if (G->env().get_tl_schema_file() != "") {
      FILE *f = fopen(G->env().get_tl_schema_file().c_str(), "r");
      const int MAX_SCHEMA_LEN = 1024 * 1024;
      static char buf[MAX_SCHEMA_LEN + 1];
      kphp_assert (f && "can't open tl schema file");
      schema_length = fread(buf, 1, sizeof(buf), f);
      kphp_assert (schema_length > 0 && schema_length < MAX_SCHEMA_LEN);
      schema.assign(buf, buf + schema_length);
      kphp_assert (!fclose(f));
    }
    W << OpenFile("_tl_schema.cpp", "", false);
    W << "extern \"C\" " << BEGIN;
    W << "const char *builtin_tl_schema = " << NL << Indent(2);
    compile_string_raw(schema, W);
    W << ";" << NL;
    W << "int builtin_tl_schema_length = ";
    char buf[100];
    sprintf(buf, "%d", schema_length);
    W << string(buf) << ";" << NL;
    W << END;
    W << CloseFile();
  }
};


class lockf_wrapper {
  std::string locked_filename_;
  int fd_ = -1;
  bool locked_ = false;

  void close_and_unlink() {
    if (close(fd_) == -1) {
      perror(("Can't close file: " + locked_filename_).c_str());
      return;
    }

    if (unlink(locked_filename_.c_str()) == -1) {
      perror(("Can't unlink file: " + locked_filename_).c_str());
      return;
    }
  }

public:
  bool lock() {
    std::string dest_path = G->env().get_dest_dir();
    if (G->env().get_use_auto_dest()) {
      dest_path += G->get_subdir_name();
    }

    std::stringstream ss;
    ss << std::hex << hash(dest_path) << "_kphp_temp_lock";
    locked_filename_ = ss.str();

    fd_ = open(locked_filename_.c_str(), O_RDWR | O_CREAT, 0666);
    assert(fd_ != -1);

    locked_ = true;
    if (lockf(fd_, F_TLOCK, 0) == -1) {
      perror("\nCan't lock file, maybe you have already run kphp2cpp");
      fprintf(stderr, "\n");

      close(fd_);
      locked_ = false;
    }

    return locked_;
  }

  ~lockf_wrapper() {
    if (locked_) {
      if (lockf(fd_, F_ULOCK, 0) == -1) {
        perror(("Can't unlock file: " + locked_filename_).c_str());
      }

      close_and_unlink();
      locked_ = false;
    }
  }
};

template<typename F>
using ExecuteFunctionArguments = vk::FunctionTraits<decltype(&F::execute)>;
template<typename F>
using ExecuteFunctionInput = typename ExecuteFunctionArguments<F>::template Argument<0>;
template<typename F>
using ExecuteFunctionOutput = typename std::remove_reference<typename ExecuteFunctionArguments<F>::template Argument<1>>::type;

template<class PipeFunctionT>
using PipeStream = Pipe<
  PipeFunctionT,
  DataStream<ExecuteFunctionInput<PipeFunctionT>>,
  ExecuteFunctionOutput<PipeFunctionT>
>;

template<class Pass>
using FunctionPassPipe = PipeStream<FunctionPassF<Pass>>;

template<class PipeFunctionT, bool parallel = true>
using PipeC = pipe_creator_tag<PipeStream<PipeFunctionT>, parallel>;

template<class Pass>
using PassC = pipe_creator_tag<FunctionPassPipe<Pass>>;

template<class PipeFunctionT>
using SyncC = sync_pipe_creator_tag<PipeStream<PipeFunctionT>>;



bool compiler_execute(KphpEnviroment *env) {
  double st = dl_time();
  G = new CompilerCore();
  G->register_env(env);
  G->start();
  if (!env->get_warnings_filename().empty()) {
    FILE *f = fopen(env->get_warnings_filename().c_str(), "w");
    if (!f) {
      fprintf(stderr, "Can't open warnings-file %s\n", env->get_warnings_filename().c_str());
      return false;
    }

    env->set_warnings_file(f);
  } else {
    env->set_warnings_file(nullptr);
  }
  if (!env->get_stats_filename().empty()) {
    FILE *f = fopen(env->get_stats_filename().c_str(), "w");
    if (!f) {
      fprintf(stderr, "Can't open stats-file %s\n", env->get_stats_filename().c_str());
      return false;
    }
    env->set_stats_file(f);
  } else {
    env->set_stats_file(nullptr);
  }

  //TODO: call it with pthread_once on need
  lexer_init();
  gen_tree_init();
  OpInfo::init_static();
  MultiKey::init_static();
  TypeData::init_static();
//  PhpDocTypeRuleParser::run_tipa_unit_tests_parsing_tags(); return true;

  DataStream<SrcFilePtr> file_stream;

  for (const auto &main_file : env->get_main_files()) {
    G->register_main_file(main_file, file_stream);
  }

  static lockf_wrapper unique_file_lock;
  if (!unique_file_lock.lock()) {
    return false;
  }

  SchedulerBase *scheduler;
  if (G->env().get_threads_count() == 1) {
    scheduler = new OneThreadScheduler();
  } else {
    auto s = new Scheduler();
    s->set_threads_count(G->env().get_threads_count());
    scheduler = s;
  }

  PipeC<LoadFileF>::get()->set_input_stream(&file_stream);


  SchedulerConstructor{scheduler}
    >> PipeC<LoadFileF>{}
    >> PipeC<FileToTokensF>{}
    >> PipeC<ParseF>{}
    >> PipeC<SplitSwitchF>{}
    >> PassC<CreateSwitchForeachVarsF>{}
    >> PipeC<CollectRequiredF>{} >> use_nth_output_tag<0>{}
    >> sync_node_tag{}
    >> PipeC<CollectClassF>{}
    >> PassC<CalcLocationsPass>{}
    >> SyncC<PreprocessDefinesF>{}
    >> PassC<CollectDefinesPass>{}
    >> sync_node_tag{}
    >> PipeC<PrepareFunctionF>{}
    >> sync_node_tag{}
    >> PassC<RegisterDefinesPass>{}
    >> PassC<PreprocessVarargPass>{}
    >> PassC<PreprocessEq3Pass>{}
    // functions which were generated from templates
    // need to be preprocessed therefore we tie second output and input of Pipe
    >> PipeC<PreprocessFunctionF>{} >> use_nth_output_tag<1>{}
    >> PipeC<PreprocessFunctionF>{} >> use_nth_output_tag<0>{}
    >> PipeC<CalcActualCallsEdgesF>{}
    >> SyncC<FilterOnlyActuallyUsedFunctionsF>{}
    >> PassC<PreprocessBreakPass>{}
    >> PassC<CalcConstTypePass>{}
    >> PassC<CollectConstVarsPass>{}
    >> PassC<CheckInstanceProps>{}
    >> PassC<ConvertListAssignmentsPass>{}
    >> PassC<RegisterVariables>{}
    >> PassC<CheckFunctionCallsPass>{}
    >> PipeC<CalcRLF>{}
    >> PipeC<CFGBeginF>{}
    >> PipeC<CheckReturnsF>{}
    >> sync_node_tag{}
    >> SyncC<TypeInfererF>{}
    >> SyncC<TypeInfererEndF>{}
    >> PipeC<CFGEndF>{}
    >> PipeC<CheckInferredInstances>{}
    >> PassC<OptimizationPass>{}
    >> PassC<CalcValRefPass>{}
    >> SyncC<CalcBadVarsF>{}
    >> PipeC<CheckUBF>{}
    >> PassC<ExtractResumableCallsPass>{}
    >> PassC<ExtractAsyncPass>{}
    >> PassC<CheckNestedForeachPass>{}
    >> PassC<CommonAnalyzerPass>{}
    >> PassC<CheckAccessModifiers>{}
    >> PassC<FinalCheckPass>{}
    >> SyncC<CodeGenF>{}
    >> PipeC<WriteFilesF, false>{};

  SchedulerConstructor{scheduler}
    >> PipeC<CollectRequiredF>{} >> use_nth_output_tag<1>{}
    >> PipeC<LoadFileF>{};

  SchedulerConstructor{scheduler}
    >> PipeC<CollectRequiredF>{} >> use_nth_output_tag<2>{}
    >> PipeC<SplitSwitchF>{};

  get_scheduler()->execute();

  if (G->env().get_error_on_warns() && stage::warnings_count > 0) {
    stage::error();
  }

  stage::die_if_global_errors();
  int verbosity = G->env().get_verbosity();
  if (G->env().get_use_make()) {
    fprintf(stderr, "start make\n");
    G->make();
  }
  G->finish();
  if (verbosity > 1) {
    profiler_print_all();
    double en = dl_time();
    double passed = en - st;
    fprintf(stderr, "PASSED: %lf\n", passed);
    mem_info_t mem_info;
    get_mem_stats(getpid(), &mem_info);
    fprintf(stderr, "RSS: %lluKb\n", mem_info.rss);
  }
  return true;
}
