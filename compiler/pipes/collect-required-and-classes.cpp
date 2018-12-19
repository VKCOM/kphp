#include "compiler/pipes/collect-required-and-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/function-pass.h"
#include "compiler/gentree.h"
#include "compiler/pipes/gen-tree-postprocess.h"

class CollectRequiredPass : public FunctionPassBase {
private:
  AUTO_PROF (collect_required);
  DataStream<SrcFilePtr> &file_stream;
  DataStream<FunctionPtr> &function_stream;

  pair<SrcFilePtr, bool> require_file(const string &file_name, ClassPtr context_class) {
    return G->require_file(file_name, context_class, file_stream);
  }

  void require_function(const string &name) {
    G->require_function(name, function_stream);
  }

  void require_class(const string &class_name, ClassPtr context_class) {
    pair<SrcFilePtr, bool> res = require_file(class_name + ".php", context_class);
    kphp_error(res.first, format("Class %s not found", class_name.c_str()));
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
        require_class(path_classname, function->context_class);
        require_class(path_classname, ClassPtr());
      }
    }
    return true;
  }

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *) {
    if (root->type() == op_func_call && root->extra_type != op_ex_func_member) {
      require_function(get_full_static_member_name(current_function, root->get_string(), true));
    }

    if (root->type() == op_func_call || root->type() == op_var || root->type() == op_func_name) {
      size_t pos$$ = root->get_string().find("::");
      if (pos$$ != string::npos) {
        const string &class_name = root->get_string().substr(0, pos$$);
        require_class(resolve_uses(current_function, class_name, '/'), ClassPtr());
      }
    }

    if (root->type() == op_constructor_call) {
      bool is_lambda = root->get_func_id() && root->get_func_id()->is_lambda();
      if (!is_lambda && likely(!root->type_help)) {     // type_help <=> Memcache | Exception
        require_class(resolve_uses(current_function, root->get_string(), '/'), ClassPtr());
      }
    }

    return root;
  }

  VertexAdaptor<op_func_call> make_require_call(VertexPtr v) {
    kphp_error_act (v->type() == op_string, "Not a string in 'require' arguments", return {});
    if (SrcFilePtr file = require_file(v->get_string(), ClassPtr()).first) {
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

class PatchInheritedMethodPass : public FunctionPassBase {
  DataStream<FunctionPtr> &function_stream;
public:
  explicit PatchInheritedMethodPass(DataStream<FunctionPtr> &function_stream) :
    function_stream(function_stream) {}

  VertexPtr on_enter_vertex(VertexPtr root, LocalT *) {
    // временно! потом уйдёт!
    // для того, чтобы не было codegen diff относительно раньше: заменяем имена переменных из ?:
    if (root->type() == op_var) {
      string var_name = root->get_string();
      if (vk::string_view(var_name).starts_with("shorthand_ternary_cond")) {
        string emulated_file_concat = current_function->file_id->unified_file_name + current_function->context_class->name;
        unsigned long long h = hash_ll(emulated_file_concat), cur_h;
        int cur_i;
        char tmp[50];
        sscanf(var_name.c_str(), "shorthand_ternary_cond$ut%llx_%d", &cur_h, &cur_i);
        sprintf(tmp, "shorthand_ternary_cond$ut%llx_%d", h, cur_i);
        root->set_string(std::string(tmp));
      }
    }

    if (root->type() == op_constructor_call && root->get_func_id() && root->get_func_id()->is_lambda()) {
      ClassPtr lambda_class = root->get_func_id()->class_id;
      FunctionPtr invoke_method = lambda_class->members.get_instance_method("__invoke")->function;

      // временно! потом уйдёт!
      // для того, чтобы не было codegen diff относительно раньше, генерируем строгое имя для лямбда-классов при копировании
      string emulated_file_concat = current_function->file_id->unified_file_name + current_function->context_class->name;
      unsigned long long h = hash_ll(emulated_file_concat), cur_h;
      int cur_i;
      char tmp[50];
      sscanf(lambda_class->name.c_str(), "$L\\anon$ut%llx_%d", &cur_h, &cur_i);
      sprintf(tmp, "anon$ut%llx_%d", h, cur_i);
      std::string kostyl_explicit_name = tmp;

      vector<VertexPtr> uses_of_lambda;
      auto anon_constructor_call = GenTree::generate_anonymous_class(invoke_method->root, function_stream, access_static_public, std::move(uses_of_lambda), kostyl_explicit_name);
      ClassPtr new_anon_class = anon_constructor_call->get_func_id()->class_id;
      new_anon_class->members.for_each([&](const ClassMemberInstanceMethod &m) {
        m.function->function_in_which_lambda_was_created = current_function;
        G->require_function(m.global_name(), function_stream);
      });
      FunctionPtr new_invoke_method = new_anon_class->members.get_instance_method("__invoke")->function;
      current_function->lambdas_inside.push_back(new_invoke_method);
      return anon_constructor_call;
    }

    return root;
  }
};

void CollectRequiredAndClassesF::inherit_static_method_from_parent(ClassPtr child_class, ClassPtr parent_class, const string &local_name, DataStream<FunctionPtr> &function_stream) {
  FunctionPtr parent_f = parent_class->members.get_static_method(local_name)->function;
  if (parent_f->is_auto_inherited) {      // A::f() -> B -> C; при A->B сделался A::f$$B
    return;                               // но при B->C должно быть A::f$$C, а не B::f$$C
  }                                       // (чтобы B::f$$C не считать required)

  if (!child_class->members.has_static_method(local_name)) {
    VertexPtr child_root = GenTree::generate_function_with_parent_call(parent_f->root, parent_class, child_class, local_name);

    FunctionPtr child_function = FunctionData::create_function(child_root, FunctionData::func_local);
    child_function->context_class = child_class;
    child_function->file_id = parent_f->file_id;
    child_function->phpdoc_token = parent_f->phpdoc_token;
    child_function->is_auto_inherited = true;
    child_function->root->inline_flag = true;

    if (G->register_function(child_function)) {
      G->require_function(child_function->name, function_stream);
    }

    child_class->members.add_static_method(child_function, parent_f->access_type);    // пока наследование только статическое
  }

  string ctx_f_name = replace_backslashes(parent_class->name) + "$$" + local_name + "$$" + replace_backslashes(child_class->name);
  // создаём функцию baseclassname$$fname$$contextclassname — это клон baseclassname$$fname
  // но! клонируем не здесь, а в отдельном пайпе — тот пайп сделан для мультипоточности, т.к. тут mutex
  // так что создаём пустую функцию и отправляем по pipeline — она обработается в CloneParentMethodWithContextF::execute()
  VertexPtr dummy_name = VertexAdaptor<op_func_name>::create();
  dummy_name->set_string(ctx_f_name);
  VertexPtr dummy_root = VertexAdaptor<op_function>::create(dummy_name);

  FunctionPtr context_function = FunctionData::create_function(dummy_root, FunctionData::func_local);
  context_function->context_class = child_class;
  context_function->access_type = parent_f->access_type;
  context_function->file_id = parent_f->file_id;
  context_function->class_id = parent_class;   // self:: это он, а parent:: это его parent (если есть)
  context_function->phpdoc_token = parent_f->phpdoc_token;

  if (G->register_function(context_function)) {
    G->require_function(context_function->name, function_stream);
  }
}



void CollectRequiredAndClassesF::inherit_child_class_from_parent(ClassPtr child_class, ClassPtr parent_class, DataStream<FunctionPtr> &function_stream) {
  child_class->parent_class = parent_class;

  kphp_error(!child_class->members.has_constructor() && !child_class->parent_class->members.has_constructor(),
             format("Invalid class extends %s and %s: extends is available only if classes are only-static",
                    child_class->name.c_str(), child_class->parent_class->name.c_str()));

  // A::f -> B -> C -> D; для D нужно C::f$$D, B::f$$D, A::f$$D
  for (; parent_class; parent_class = parent_class->parent_class) {
    parent_class->members.for_each([&](const ClassMemberStaticMethod &m) {
      inherit_static_method_from_parent(child_class, parent_class, m.local_name(), function_stream);
    });
  }
}

bool CollectRequiredAndClassesF::is_class_ready(ClassPtr klass) {
  for (const auto &dep : klass->str_dependents) {
    std::string dep_class_name = resolve_uses(klass->init_function, dep.class_name, '\\');
    ClassPtr dep_class_if_exists = G->get_class(dep_class_name);
    if (!dep_class_if_exists || !is_class_ready(dep_class_if_exists)) {
      return false;
    }
  }
  return true;
}

// упомянуть про гарантию 1 вызова  
void CollectRequiredAndClassesF::on_class_ready(ClassPtr klass, DataStream<FunctionPtr> &function_stream) {
  kphp_assert(klass->init_function->class_id == klass);

  for (const auto &dep : klass->str_dependents) {
    ClassPtr dep_class = G->get_class(resolve_uses(klass->init_function, dep.class_name, '\\'));
    auto pos = std::find(classes_waiting.begin(), classes_waiting.end(), dep_class);
    if (pos != classes_waiting.end()) {
      classes_waiting.erase(pos);
      on_class_ready(dep_class, function_stream);
    }

    if (dep.type == ctype_class) {
      inherit_child_class_from_parent(klass, dep_class, function_stream);
    }
  }
}


void CollectRequiredAndClassesF::on_class_executing(ClassPtr klass, DataStream<FunctionPtr> &function_stream) {
  std::lock_guard<std::mutex> locker(mutex_classes_waiting);

  G->register_class(klass);

  if (!is_class_ready(klass)) {
    classes_waiting.push_back(klass);
    return;
  }

  auto pos = std::find(classes_waiting.begin(), classes_waiting.end(), klass);
  if (pos != classes_waiting.end()) {
    classes_waiting.erase(pos);
  }
  on_class_ready(klass, function_stream);

  for (auto it = classes_waiting.begin(); it != classes_waiting.end();) {
    if (is_class_ready(*it)) {
      on_class_ready(*it, function_stream);
      it = classes_waiting.erase(it);
    } else {
      ++it;
    }
  }
}


void CollectRequiredAndClassesF::execute(FunctionPtr function, CollectRequiredAndClassesF::OStreamT &os) {
  auto &ready_function_stream = *os.template project_to_nth_data_stream<0>();
  auto &file_stream = *os.template project_to_nth_data_stream<1>();
  auto &function_stream = *os.template project_to_nth_data_stream<2>();

  CollectRequiredPass pass(file_stream, function_stream);

  run_function_pass(function, &pass);

  if (stage::has_error()) {
    return;
  }

  if (function->type() == FunctionData::func_global && function->class_id && function->class_id != function->context_class) {
    kphp_assert(0);
    return;
  }

  if (function->class_id && function->class_id->init_function == function) {
    on_class_executing(function->class_id, function_stream);
  }

  ready_function_stream << function;
}
