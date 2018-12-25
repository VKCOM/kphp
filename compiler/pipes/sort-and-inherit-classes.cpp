#include "compiler/pipes/sort-and-inherit-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/threading/profiler.h"
#include "compiler/gentree.h"

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
    if (root->type() == op_constructor_call && root->get_func_id() && root->get_func_id()->is_lambda()) {
      ClassPtr lambda_class = root->get_func_id()->class_id;
      FunctionPtr invoke_method = lambda_class->members.get_instance_method("__invoke")->function;
      vector<VertexPtr> uses_of_lambda;

      return GenTree::generate_anonymous_class(invoke_method->root, function_stream, current_function, std::move(uses_of_lambda));
    }

    return root;
  }
};

// если все dependents класса уже обработаны, возвращает пустую строку
// если же какой-то из dependents (имя класса/интерфейса) ещё не обработан (его надо подождать), возвращает его
std::string SortAndInheritClassesF::is_class_ready(ClassPtr klass) {
  for (const auto &dep : klass->str_dependents) {
    if (!ht.at(hash_ll(dep.class_name))->data.done) {
      return dep.class_name;
    }
  }
  return std::string();
}

// делаем функцию childclassname$$localname, которая выглядит как
// function childclassname$$localname($args) { return baseclassname$$localname$$childclassname(...$args); }
VertexPtr SortAndInheritClassesF::generate_function_with_parent_call(VertexAdaptor<op_function> root, ClassPtr parent_class, ClassPtr child_class, const string &local_name) {
  auto new_name = VertexAdaptor<op_func_name>::create();
  new_name->set_string(replace_backslashes(child_class->name) + "$$" + local_name);
  vector<VertexPtr> new_params_next;
  vector<VertexPtr> new_params_call;
  for (const auto &parameter : *root->params()) {
    if (parameter->type() == op_func_param) {
      new_params_call.push_back(parameter.as<op_func_param>()->var().as<op_var>().clone());
      new_params_next.push_back(parameter.clone());
    } else if (parameter->type() == op_func_param_callback) {
      if (!kphp_error(false, "Callbacks are not supported in class static methods")) {
        return VertexPtr();
      }
    }
  }

  string parent_function_name = replace_backslashes(parent_class->name) + "$$" + local_name + "$$" + replace_backslashes(child_class->name);
  // it's equivalent to new_func_call->set_string("parent::" + local_name);
  auto new_func_call = VertexAdaptor<op_func_call>::create(new_params_call);
  new_func_call->set_string(parent_function_name);

  auto new_return = VertexAdaptor<op_return>::create(new_func_call);
  auto new_cmd = VertexAdaptor<op_seq>::create(new_return);
  auto new_params = VertexAdaptor<op_func_param_list>::create(new_params_next);
  auto func = VertexAdaptor<op_function>::create(new_name, new_params, new_cmd);
  func->copy_location_and_flags(*root);
  func->inline_flag = true;

  return func;
}

// клонировать функцию baseclassname$$fname в контекстную baseclassname$$fname$$contextclassname
// (контекстные нужны для реализации статического наследования)
FunctionPtr SortAndInheritClassesF::create_function_with_context(FunctionPtr parent_f, const std::string &ctx_function_name) {
  VertexAdaptor<op_function> root = clone_vertex(parent_f->root);
  root->name()->set_string(ctx_function_name);

  FunctionPtr context_function = FunctionData::create_function(root, FunctionData::func_local);
  root->set_func_id(context_function);

  context_function->access_type = parent_f->access_type;
  context_function->file_id = parent_f->file_id;
  context_function->class_id = parent_f->class_id;   // self:: это он, а parent:: это его parent (если есть)
  context_function->phpdoc_token = parent_f->phpdoc_token;

  return context_function;
}

void SortAndInheritClassesF::inherit_static_method_from_parent(ClassPtr child_class, ClassPtr parent_class, const string &local_name, DataStream<FunctionPtr> &function_stream) {
  FunctionPtr parent_f = parent_class->members.get_static_method(local_name)->function;
  if (parent_f->is_auto_inherited) {      // A::f() -> B -> C; при A->B сделался A::f$$B
    return;                               // но при B->C должно быть A::f$$C, а не B::f$$C
  }                                       // (чтобы B::f$$C не считать required)

  if (!child_class->members.has_static_method(local_name)) {
    VertexPtr child_root = generate_function_with_parent_call(parent_f->root, parent_class, child_class, local_name);

    FunctionPtr child_function = FunctionData::create_function(child_root, FunctionData::func_local);
    child_function->file_id = parent_f->file_id;
    child_function->phpdoc_token = parent_f->phpdoc_token;
    child_function->is_auto_inherited = true;

    child_class->members.add_static_method(child_function, parent_f->access_type);    // пока наследование только статическое

    G->register_and_require_function(child_function, function_stream);
  }

  // контекстная функция имеет название baseclassname$$fname$$contextclassname
  // это склонированное дерево baseclassname$$fname (parent_f) + патч для лямбд (который должен когда-то уйти)
  string ctx_function_name = replace_backslashes(parent_class->name) + "$$" + local_name + "$$" + replace_backslashes(child_class->name);
  FunctionPtr context_function = create_function_with_context(parent_f, ctx_function_name);
  context_function->context_class = child_class;

  PatchInheritedMethodPass pass(function_stream);
  run_function_pass(context_function, &pass);

  G->register_and_require_function(context_function, function_stream);
}



void SortAndInheritClassesF::inherit_child_class_from_parent(ClassPtr child_class, ClassPtr parent_class, DataStream<FunctionPtr> &function_stream) {
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

/**
 * Каждый класс поступает сюда один и ровно один раз — когда он и все его dependents
 * (родители, трейты, интерфейсы) тоже готовы.
 */
void SortAndInheritClassesF::on_class_ready(ClassPtr klass, DataStream<FunctionPtr> &function_stream) {
  kphp_assert(klass->init_function->class_id == klass);

  for (const auto &dep : klass->str_dependents) {
    ClassPtr dep_class = G->get_class(dep.class_name);

    switch (dep.type) {
      case ctype_class:
        inherit_child_class_from_parent(klass, dep_class, function_stream);
        break;
      case ctype_interface:
        kphp_assert(0 && "implementing interfaces is not supported yet");
        break;
      case ctype_trait:
        kphp_assert(0 && "mixin traits is not supported yet");
        break;
    }
  }
}


void SortAndInheritClassesF::execute(ClassPtr klass, MultipleDataStreams<FunctionPtr, ClassPtr> &os) {
  AUTO_PROF(sort_and_inherit_classes);
  auto &function_stream = *os.template project_to_nth_data_stream<0>();
  auto &restart_class_stream = *os.template project_to_nth_data_stream<1>();

  std::string to_wait_classname_if_not_ready = is_class_ready(klass);
  if (!to_wait_classname_if_not_ready.empty()) {
    auto node = ht.at(hash_ll(to_wait_classname_if_not_ready));
    AutoLocker<Lockable*> locker(node);
    if (node->data.done) {              // вдруг между вызовом ready и этим моментом стало done
      restart_class_stream << klass;
      return;
    }
    node->data.waiting.emplace_front(klass);
    return;
  }

  on_class_ready(klass, function_stream);

  {
    auto node = ht.at(hash_ll(klass->name));
    kphp_assert(!node->data.done);

    AutoLocker<Lockable *> locker(node);
    for (ClassPtr restart_klass: node->data.waiting) {    // все классы, которые ждали текущего —
      restart_class_stream << restart_klass;              // запустить SortAndInheritClassesF ещё раз для них
    }

    node->data.waiting.clear();
    node->data.done = true;
  }
}
