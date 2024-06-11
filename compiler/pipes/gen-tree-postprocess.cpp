// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/gen-tree-postprocess.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/src-file.h"
#include "compiler/vertex-util.h"

namespace {
template <typename F>
bool contains_vertex(const VertexPtr &root, F fn) {
  if (fn(root)) {
    return true;
  }
  for (const auto &v : *root) {
    if (contains_vertex(v, fn)) {
      return true;
    }
  }
  return false;
}

VertexAdaptor<op_set> convert_set_null_coalesce(VertexAdaptor<op_set_null_coalesce> v) {
  // forbid any lhs that contains a function/method calls for now as PHP
  // behavior is not well-documented in this case;
  // it seems like this will be good enough even for the time when __get and __set
  // magic methods will be implemented
  bool has_call = contains_vertex(v->lhs(), [](VertexPtr v) {
    return v->type() == op_func_call;
  });
  kphp_error(!has_call, "Function calls on the left-hand side of \?\?= are not supported");

  // lhs() is cloned to make it possible for the compiler to split
  // variables and make null coalesce result better typed
  auto rhs = VertexAdaptor<op_null_coalesce>::create(v->lhs(), v->rhs()).set_location(v);
  return VertexAdaptor<op_set>::create(v->lhs().clone(), rhs).set_location(v);
}

VertexAdaptor<op_list> make_list_op(VertexAdaptor<op_set> assign) {
  bool has_explicit_keys = false;
  bool has_empty_entries = false; // To give the same error message as PHP

  auto list = assign->lhs();
  auto arr = assign->rhs();

  std::vector<VertexAdaptor<op_list_keyval>> mappings;
  mappings.reserve(list->size());

  int implicit_key = -1;

  for (auto x : *list) {
    if (const auto arrow = x.try_as<op_double_arrow>()) {
      has_explicit_keys = true;
      mappings.emplace_back(VertexAdaptor<op_list_keyval>::create(arrow->lhs(), arrow->rhs()));
      continue;
    }

    implicit_key++;

    if (x->type() == op_lvalue_null) {
      has_empty_entries = true;
      continue;
    }

    auto var = x;
    auto key = VertexUtil::create_int_const(implicit_key);
    mappings.emplace_back(VertexAdaptor<op_list_keyval>::create(key, var));
  }

  bool has_implicit_keys = implicit_key != -1;
  if (has_explicit_keys && has_empty_entries) {
    kphp_error(0, "Cannot use empty array entries in keyed array assignment");
  } else if (has_explicit_keys && has_implicit_keys) {
    kphp_error(0, "Cannot mix keyed and unkeyed array entries in assignments");
  }

  return VertexAdaptor<op_list>::create(mappings, arr).set_location(assign);
}

VertexAdaptor<op_require> make_require_once_call(SrcFilePtr lib_main_file, VertexAdaptor<op_func_call> require_lib_call) {
  auto lib_main_file_name = VertexAdaptor<op_string>::create();
  lib_main_file_name->set_string(lib_main_file->file_name);
  auto req_once = VertexAdaptor<op_require>::create(lib_main_file_name).set_location(require_lib_call);
  req_once->once = true;
  return req_once;
}

VertexPtr process_require_lib(VertexAdaptor<op_func_call> require_lib_call) {
  kphp_error_act (!G->settings().is_static_lib_mode(), "require_lib is forbidden to use for compiling libs", return require_lib_call);
  VertexRange args = require_lib_call->args();
  kphp_error_act (args.size() == 1, fmt_format("require_lib expected 1 arguments, got {}", args.size()), return require_lib_call);
  auto lib_name_node = args[0];
  kphp_error_act (lib_name_node->type() == op_string, "First argument of require_lib must be a string", return require_lib_call);

  std::string lib_require_name = lib_name_node->get_string();
  kphp_error_act (!lib_require_name.empty() && lib_require_name.back() != '/',
                  fmt_format("require_lib got bad lib name '{}'", lib_require_name), return require_lib_call);

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
      lib->update_lib_main_file(header_file->file_name, header_file->relative_dir_name);
      auto req_header_txt = make_require_once_call(header_file, require_lib_call);
      auto lib_run_global_call = VertexAdaptor<op_func_call>::create();
      lib_run_global_call->set_string(lib->run_global_function_name());
      new_vertex = VertexAdaptor<op_seq>::create(req_header_txt, lib_run_global_call).set_location(require_lib_call);
    }
  }
  if (!new_vertex) {
    const std::string &php_file = index_php_full.empty() ? index_php : index_php_full;
    if (SrcFilePtr lib_index_file = G->register_file(php_file, registered_lib)) {
      lib->update_lib_main_file(lib_index_file->file_name, lib_index_file->relative_dir_name);
      new_vertex = make_require_once_call(lib_index_file, require_lib_call);
    }
  }
  kphp_error_act (new_vertex, fmt_format("Can't find '{}' lib", lib_require_name), return require_lib_call);
  return new_vertex;
}
} // namespace

GenTreePostprocessPass::builtin_fun GenTreePostprocessPass::get_builtin_function(const std::string &name) {
  static std::unordered_map<vk::string_view, builtin_fun> functions = {
    {"strval",        {op_conv_string,         1}},
    {"intval",        {op_conv_int,            1}},
    {"boolval",       {op_conv_bool,           1}},
    {"floatval",      {op_conv_float,          1}},
    {"arrayval",      {op_conv_array,          1}},
    {"not_false",     {op_conv_drop_false,     1}},
    {"not_null",      {op_conv_drop_null,      1}},
    {"fork",          {op_fork,                1}},
    {"force_sync",    {op_force_sync,          1}},
    {"pow",           {op_pow,                 2}}
  };
  auto it = functions.find(name);
  if (it == functions.end()) {
    if (name == "profiler_is_enabled") {
      return {G->settings().profiler_level.get() ? op_true : op_false, 0};
    }
    return {op_err, -1};
  }
  return it->second;
}

VertexPtr GenTreePostprocessPass::on_enter_vertex(VertexPtr root) {
  stage::set_line(root->location.line);
  if (auto set_op = root.try_as<op_set>()) {
    // list(...) = ... or short syntax (PHP 7) [...] = ...
    if (vk::any_of_equal(set_op->lhs()->type(), op_list_ce, op_array)) {
      return make_list_op(set_op);
    }
  }
  if (root->type() == op_list_ce) {
    kphp_error(0, "Unexpected list() not as left side of assignment\n");
  }

  if (auto set_coalesce = root.try_as<op_set_null_coalesce>()) {
    return convert_set_null_coalesce(set_coalesce);
  }

  if (auto instanceof = root.try_as<op_instanceof>()) {
    kphp_error(instanceof->rhs()->type() == op_func_name, "right side of `instanceof` should be a class name");
    if (!vk::string_view{instanceof->rhs()->get_string()}.ends_with("::class")) {
      instanceof->rhs()->set_string(instanceof->rhs()->get_string() + "::class");
    }
    return root;
  }

  if (auto call = root.try_as<op_func_call>()) {
    const auto &name = call->get_string();

    auto builtin = get_builtin_function(name);
    if (builtin.op != op_err && call->size() == builtin.args) {
      return create_vertex(builtin.op, call->args()).set_location(root);
    }

    if (name == "call_user_func_array") {
      auto args = call->args();
      kphp_error (args.size() == 2, fmt_format("call_user_func_array expected 2 arguments, got {}", (int)root->size()));
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
  }

  if (auto return_vertex = root.try_as<op_return>()) {
    if (current_function->is_constructor() && !return_vertex->has_expr()) {
      root = VertexAdaptor<op_return>::create(ClassData::gen_vertex_this(return_vertex->location));
      root.set_location(return_vertex);
    }
  }

  // an array with "holes" like [$a, ,$c] can be used only as LHS in list assignments [...] = ...
  // we replace such things to op_list, so no valid array should contain empty elements after it
  if (auto as_array = root.try_as<op_array>()) {
    for (auto array_item : *as_array) {
      kphp_error(array_item->type() != op_lvalue_null, fmt_format("Can not use array with empty elements here"));
    }
  }

  return root;
}

VertexPtr GenTreePostprocessPass::on_exit_vertex(VertexPtr root) {
  if (root->type() == op_var) {
    if (VertexUtil::is_superglobal(root->get_string())) {
      root->extra_type = op_ex_var_superglobal;
    }
  }

  if (auto call = root.try_as<op_func_call>()) {
    if (!G->settings().profiler_level.get() && call->size() == 1 &&
        vk::any_of_equal(call->get_string(), "profiler_set_log_suffix", "profiler_set_function_label")) {
      return VertexAdaptor<op_if>::create(VertexAdaptor<op_false>::create(), VertexUtil::embrace(call)).set_location_recursively(root);
    }
  }

  if (auto array = root.try_as<op_array>()) {
    return convert_array_with_spread_operators(array);
  }

  return root;
}

VertexAdaptor<op_array> array_vertex_from_slice(const VertexRange &args, size_t start, size_t end) {
  return VertexAdaptor<op_array>::create(
    std::vector<VertexPtr>{args.begin() + start, args.begin() + end}
  );
}

VertexPtr GenTreePostprocessPass::convert_array_with_spread_operators(VertexAdaptor<op_array> array_vertex) {
  const auto args = array_vertex->args();
  const auto with_spread = std::any_of(args.begin(), args.end(), [](const auto &arg) {
    return arg->type() == op_varg;
  });

  if (!with_spread) {
    return array_vertex;
  }

  std::vector<VertexPtr> parts;
  constexpr size_t UNINITIALIZED = -1;

  size_t last_spread_index = UNINITIALIZED;
  size_t prev_spread_index = UNINITIALIZED;

  for (int i = 0; i < args.size(); ++i) {
    if (args[i]->type() == op_varg) {
      last_spread_index = i;

      if (prev_spread_index == UNINITIALIZED) {
        const auto has_elements_before = (last_spread_index != 0);

        if (has_elements_before) {
          parts.emplace_back(
            array_vertex_from_slice(args, 0, last_spread_index)
          );
        }
      } else {
        const auto count_elements = last_spread_index - (prev_spread_index + 1);

        if (count_elements != 0) {
          parts.emplace_back(
            array_vertex_from_slice(args, prev_spread_index + 1, last_spread_index)
          );
        }
      }

      parts.emplace_back(args[i].try_as<op_varg>()->array());

      prev_spread_index = last_spread_index;
    }
  }

  if (last_spread_index != args.size() - 1) {
    parts.emplace_back(
      array_vertex_from_slice(args, last_spread_index + 1, args.size())
    );
  }

  auto call = VertexAdaptor<op_func_call>::create(parts).set_location(array_vertex->location);
  call->set_string("array_merge_spread");

  return call;
}
