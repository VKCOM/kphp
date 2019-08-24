#include "compiler/pipes/gen-tree-postprocess.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"

namespace {
VertexAdaptor<op_require> make_require_once_call(SrcFilePtr lib_main_file, VertexAdaptor<op_func_call> require_lib_call) {
  auto lib_main_file_name = VertexAdaptor<op_string>::create();
  lib_main_file_name->set_string(lib_main_file->file_name);
  auto req_once = VertexAdaptor<op_require>::create(lib_main_file_name);
  req_once->once = true;
  set_location(req_once, require_lib_call->get_location());
  return req_once;
}

VertexPtr process_require_lib(VertexAdaptor<op_func_call> require_lib_call) {
  kphp_error_act (!G->env().is_static_lib_mode(), "require_lib is forbidden to use for compiling libs", return require_lib_call);
  VertexRange args = require_lib_call->args();
  kphp_error_act (args.size() == 1, format("require_lib expected 1 arguments, got %zu", args.size()), return require_lib_call);
  auto lib_name_node = args[0];
  kphp_error_act (lib_name_node->type() == op_string, "First argument of require_lib must be a string", return require_lib_call);

  std::string lib_require_name = lib_name_node->get_string();
  kphp_error_act (!lib_require_name.empty() && lib_require_name.back() != '/',
                  format("require_lib got bad lib name '%s'", lib_require_name.c_str()), return require_lib_call);

  size_t txt_file_index = std::numeric_limits<size_t>::max();
  const std::string functions_txt = lib_require_name + "/lib/functions.txt";
  const std::string functions_txt_full = G->search_file_in_include_dirs(functions_txt, &txt_file_index);
  size_t php_file_index = std::numeric_limits<size_t>::max();
  const std::string index_php = lib_require_name + "/php/index.php";
  const std::string index_php_full = G->search_file_in_include_dirs(index_php, &php_file_index);

  VertexPtr new_vertex;
  LibPtr lib(new LibData(lib_require_name));
  LibPtr registered_lib = G->register_lib(lib);
  if (txt_file_index <= php_file_index) {
    const std::string &txt_file = functions_txt_full.empty() ? functions_txt : functions_txt_full;
    if (SrcFilePtr header_file = G->register_file(txt_file, registered_lib)) {
      lib->update_lib_main_file(header_file->file_name, header_file->unified_dir_name);
      auto req_header_txt = make_require_once_call(header_file, require_lib_call);
      auto lib_run_global_call = VertexAdaptor<op_func_call>::create();
      lib_run_global_call->set_string(lib->run_global_function_name());
      new_vertex = VertexAdaptor<op_seq>::create(req_header_txt, lib_run_global_call);
      set_location(new_vertex, require_lib_call->get_location());
    }
  }
  if (!new_vertex) {
    const std::string &php_file = index_php_full.empty() ? index_php : index_php_full;
    if (SrcFilePtr lib_index_file = G->register_file(php_file, registered_lib)) {
      lib->update_lib_main_file(lib_index_file->file_name, lib_index_file->unified_dir_name);
      new_vertex = make_require_once_call(lib_index_file, require_lib_call);
    }
  }
  if (lib != registered_lib) {
    lib.clear();
  }
  kphp_error_act (new_vertex, format("Can't find '%s' lib", lib_require_name.c_str()), return require_lib_call);
  return new_vertex;
}
} // namespace

GenTreePostprocessPass::builtin_fun GenTreePostprocessPass::get_builtin_function(const std::string &name) {
  static std::map<std::string, builtin_fun> functions = {
    {"strval",   {op_conv_string, 1}},
    {"intval",   {op_conv_int,    1}},
    {"boolval",  {op_conv_bool,   1}},
    {"floatval", {op_conv_float,  1}},
    {"arrayval", {op_conv_array,  1}},
    {"uintval",  {op_conv_uint,   1}},
    {"longval",  {op_conv_long,   1}},
    {"ulongval", {op_conv_ulong,  1}},
    {"fork",     {op_fork,        1}},
    {"pow",      {op_pow,         2}}
  };
  auto it = functions.find(name);
  if (it == functions.end()) {
    return {op_err, -1};
  }
  return it->second;
}

VertexPtr GenTreePostprocessPass::on_enter_vertex(VertexPtr root, LocalT *) {
  stage::set_line(root->location.line);
  if (auto set_op = root.try_as<op_set>()) {
    if (set_op->lhs()->type() == op_list_ce) {
      auto list = VertexAdaptor<op_list>::create(set_op->lhs()->get_next(), set_op->rhs()).set_location(root);
      list->phpdoc_str = root.as<op_set>()->phpdoc_str;
      return list;
    }
  }
  if (root->type() == op_list_ce) {
    kphp_error(0, "Unexpected list() not as left side of assignment\n");
  }

  if (auto instanceof = root.try_as<op_instanceof>()) {
    kphp_error(instanceof->rhs()->type() == op_func_name, "right side of `instanceof` should be class name");
    if (!vk::string_view{instanceof->rhs()->get_string()}.ends_with("::class")) {
      instanceof->rhs()->set_string(instanceof->rhs()->get_string() + "::class");
    }
    return root;
  }

  if (auto call = root.try_as<op_func_call>()) {
    auto &name = call->get_string();

    auto builtin = get_builtin_function(name);
    if (builtin.op != op_err && call->size() == builtin.args) {
      return create_vertex(builtin.op, call->args()).set_location(root);
    }

    if (name == "call_user_func_array") {
      auto args = call->args();
      kphp_error (args.size() == 2, format("call_user_func_array expected 2 arguments, got %d", (int)root->size()));
      kphp_error_act (args[0]->type() == op_string, "First argument of call_user_func_array must be a const string", return root);
      auto arg = VertexAdaptor<op_varg>::create(args[1]).set_location(args[1]);
      auto new_root = VertexAdaptor<op_func_call>::create(arg).set_location(arg);
      new_root->set_string(args[0]->get_string());
      return new_root;
    }

    if (name == "require_lib") {
      return process_require_lib(call);
    }

    if (name == "min" || name == "max") {
      auto args = call->args();
      if (args.size() == 1) {
        if (kphp_error(args[0]->type() != op_varg, "Argument unpacking is not allowed as only argument of min/max")) {
          return call;
        }
        args[0] = VertexAdaptor<op_varg>::create(args[0]);
      }
    }

    if (vk::any_of_equal(name, "func_get_args", "func_get_arg", "func_num_args")) {
      current_function->is_vararg = true;
      current_function->has_variadic_param = true;
    }
  }

  if (auto return_vertex = root.try_as<op_return>()) {
    if (current_function->is_constructor() && !return_vertex->has_expr()) {
      root = VertexAdaptor<op_return>::create(ClassData::gen_vertex_this(return_vertex->location));
      set_location(root, return_vertex->location);
    }
  }

  return root;
}

VertexPtr GenTreePostprocessPass::on_exit_vertex(VertexPtr root, LocalT *) {
  if (root->type() == op_var) {
    if (is_superglobal(root->get_string())) {
      root->extra_type = op_ex_var_superglobal;
    }
  }

  if (auto arrow = root.try_as<op_arrow>()) {
    VertexPtr rhs = arrow->rhs();

    if (rhs->type() == op_func_name) {
      auto inst_prop = VertexAdaptor<op_instance_prop>::create(arrow->lhs());
      ::set_location(inst_prop, root->get_location());
      inst_prop->set_string(rhs->get_string());

      return inst_prop;
    } else if (auto call = rhs.try_as<op_func_call>()) {
      vector<VertexPtr> new_next;
      const vector<VertexPtr> &old_next = rhs->get_next();

      new_next.push_back(arrow->lhs());
      new_next.insert(new_next.end(), old_next.begin(), old_next.end());

      auto new_root = create_vertex(call->type(), new_next).as<op_func_call>();
      ::set_location(new_root, root->get_location());
      new_root->extra_type = op_ex_func_call_arrow;
      new_root->str_val = call->get_string();
      new_root->func_id = call->func_id;

      return new_root;
    } else {
      kphp_error (false, "Operator '->' expects property or function call as its right operand");
    }
  }

  return root;
}

bool GenTreePostprocessPass::is_superglobal(const string &s) {
  static std::set<string> names = {
    "_SERVER",
    "_GET",
    "_POST",
    "_FILES",
    "_COOKIE",
    "_REQUEST",
    "_ENV"
  };
  return names.find(s) != names.end();
}

