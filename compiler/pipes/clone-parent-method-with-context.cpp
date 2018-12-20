#include "compiler/pipes/clone-parent-method-with-context.h"

#include "compiler/data/function-data.h"
#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/name-gen.h"
#include "compiler/gentree.h"
#include "compiler/compiler-core.h"

/**
 * Через этот pass проходят функции вида
 * baseclassname$$fname$$contextclassname
 * которые получились клонированием родительского дерева
 * В будущем этот pass должен уйти, когда лямбды будут вычленяться не на уровне gentree, а позже
 */
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

void CloneParentMethodWithContextF::create_ast_of_function_with_context(FunctionPtr function, DataStream<FunctionPtr> &os) {
  // функция имеет название baseclassname$$fname$$contextclassname
  // нужно достать функцию baseclassname$$fname и клонировать её дерево
  std::string full_name = function->root->name()->get_string();
  auto pos = full_name.rfind("$$");

  FunctionPtr parent_f = G->get_function(full_name.substr(0, pos));

  function->root = clone_vertex(parent_f->root);
  function->root->name()->set_string(full_name);
  function->root->set_func_id(function);

  PatchInheritedMethodPass pass(os);
  run_function_pass(function, &pass);
}

void CloneParentMethodWithContextF::execute(FunctionPtr function, DataStream <FunctionPtr> &os) {
  AUTO_PROF (clone_method_with_context);

  // обрабатываем функции для статического наследования (контекстные) — вида baseclassname$$fname$$contextclassname
  // сюда прокидываются такие функции из collect required — этот пайп сделан специально,
  // чтобы работать в несколько потоков
  // причём это нужно до gen tree postprocess, т.к. родительские могут быть не required (и не прошли пайпы)
  if (function->context_class && function->context_class != function->class_id) {
    create_ast_of_function_with_context(function, os);
  }

  os << function;
}
