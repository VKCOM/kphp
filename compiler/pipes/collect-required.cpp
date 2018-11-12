#include "compiler/pipes/collect-required.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"

class CollectRequiredPass : public FunctionPassBase {
private:
  AUTO_PROF (collect_required);
  DataStream<SrcFilePtr> &file_stream;
  DataStream<FunctionPtr> &function_stream;

  pair<SrcFilePtr, bool> require_file(const string &file_name, const string &class_context) {
    return G->require_file(file_name, class_context, current_function->file_id->owner_lib, file_stream);
  }

  void require_function(const string &name) {
    G->require_function(name, function_stream);
  }

  void require_class(const string &class_name, const string &context_name) {
    pair<SrcFilePtr, bool> res = require_file(class_name + ".php", context_name);
    kphp_error(res.first, format("Class %s not found", class_name.c_str()));
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

public:
  CollectRequiredPass(DataStream<SrcFilePtr> &file_stream, DataStream<FunctionPtr> &function_stream) :
    file_stream(file_stream),
    function_stream(function_stream) {
  }

  string get_description() {
    return "Collect required";
  }

  bool on_start(FunctionPtr function) {
    if (!FunctionPassBase::on_start(function)) {
      return false;
    }

    if (function->type() == FunctionData::func_global && function->class_id) {
      for (const auto &dep : function->class_id->str_dependents) {
        const string &path_classname = resolve_uses(function, dep.class_name, '/');
        require_class(path_classname, function->class_context_name);
        require_class(path_classname, "");  
      }
    }
    return true;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *) {
    if (root->type() == op_func_call || root->type() == op_func_name) {
      if (root->extra_type != op_ex_func_member) {
        string name = get_full_static_member_name(current_function, root->get_string(), root->type() == op_func_call);
        require_function(name);
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
      bool is_lambda = root->get_func_id() && root->get_func_id()->is_lambda();
      if (!is_lambda && likely(!root->type_help)) {     // type_help <=> Memcache | Exception
        const string &class_name = resolve_uses(current_function, root->get_string(), '/');
        require_class(class_name, "");
      }
    }

    if (root->type() == op_func_call) {
      const string &name = root->get_string();
      if (name == "func_get_args" || name == "func_get_arg" || name == "func_num_args") {
        current_function->varg_flag = true;
      }
    }

    return root;
  }

  VertexAdaptor<op_func_call> make_require_call(VertexPtr v) {
    kphp_error_act (v->type() == op_string, "Not a string in 'require' arguments", return {});
    if (SrcFilePtr file = require_file(v->get_string(), "").first) {
      auto call = VertexAdaptor<op_func_call>::create();
      call->str_val = file->main_func_name;
      return call;
    }
    kphp_error (0, format("Cannot require [%s]\n", v->get_string().c_str()));
    return {};
  }

  VertexPtr on_exit_vertex(VertexPtr root, LocalT *) {
    if (root->type() == op_require_once || root->type() == op_require) {
      kphp_assert_msg(root->size() == 1, "Only one child possible for require vertex");
      root->back() = make_require_call(root->back());
    }
    return root;
  }
};

void CollectRequiredF::execute(FunctionPtr function, CollectRequiredF::OStreamT &os) {
  auto &ready_function_stream = *os.template project_to_nth_data_stream<0>();
  auto &file_stream = *os.template project_to_nth_data_stream<1>();
  auto &function_stream = *os.template project_to_nth_data_stream<2>();

  CollectRequiredPass pass(file_stream, function_stream);

  run_function_pass(function, &pass);

  if (stage::has_error()) {
    return;
  }

  if (function->type() == FunctionData::func_global && function->class_id &&
      function->class_id->name != function->class_context_name) {
    return;
  }
  ready_function_stream << function;
}
