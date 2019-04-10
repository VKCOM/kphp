#include "compiler/pipes/sort-and-inherit-classes.h"

#include "compiler/compiler-core.h"
#include "compiler/data/class-data.h"
#include "compiler/gentree.h"
#include "compiler/phpdoc.h"
#include "compiler/stage.h"
#include "compiler/threading/profiler.h"
#include "common/algorithms/hashes.h"

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
      vector<VertexAdaptor<op_func_param>> uses_of_lambda;

      lambda_class->members.for_each([&](ClassMemberInstanceField &f) {
        auto new_var_use = VertexAdaptor<op_var>::create();
        new_var_use->set_string(f.local_name());
        set_location(new_var_use, f.root->location);
        auto func_param = VertexAdaptor<op_func_param>::create(new_var_use);
        set_location(func_param, f.root->location);

        uses_of_lambda.insert(uses_of_lambda.begin(), func_param);
      });

      return GenTree::generate_anonymous_class(invoke_method->root, function_stream, current_function, std::move(uses_of_lambda));
    }

    return root;
  }
};

TSHashTable<SortAndInheritClassesF::wait_list> SortAndInheritClassesF::ht;

// если все dependents класса уже обработаны, возвращает nullptr
// если же какой-то из dependents (класс/интерфейс) ещё не обработан (его надо подождать), возвращает указатель на его
auto SortAndInheritClassesF::get_not_ready_dependency(ClassPtr klass) -> decltype(ht)::HTNode* {
  for (const auto &dep : klass->str_dependents) {
    auto node = ht.at(vk::std_hash(dep.class_name));
    kphp_assert(node);
    if (!node->data.done) {
      return node;
    }
  }

  return {};
}

// делаем функцию childclassname$$localname, которая выглядит как
// function childclassname$$localname($args) { return baseclassname$$localname$$childclassname(...$args); }
VertexAdaptor<op_function> SortAndInheritClassesF::generate_function_with_parent_call(FunctionPtr parent_f, ClassPtr child_class, const ClassMemberStaticMethod &parent_method) {
  auto local_name = parent_method.local_name();

  auto new_name = VertexAdaptor<op_func_name>::create();
  new_name->set_string(replace_backslashes(child_class->name) + "$$" + local_name);

  auto parent_class = parent_method.function->class_id;
  auto parent_function_name = replace_backslashes(parent_class->name) + "$$" + local_name + "$$" + replace_backslashes(child_class->name);
  // it's equivalent to new_func_call->set_string("parent::" + local_name);
  auto new_func_call = VertexAdaptor<op_func_call>::create(parent_f->get_params_as_vector_of_vars());
  new_func_call->set_string(parent_function_name);

  auto new_return = VertexAdaptor<op_return>::create(new_func_call);
  auto new_cmd = VertexAdaptor<op_seq>::create(new_return);
  auto new_params = parent_f->root->params().clone();
  auto func = VertexAdaptor<op_function>::create(new_name, new_params, new_cmd);
  func->copy_location_and_flags(*parent_f->root);

  return func;
}

// клонировать функцию baseclassname$$fname в контекстную baseclassname$$fname$$contextclassname
// (контекстные нужны для реализации статического наследования)
FunctionPtr SortAndInheritClassesF::create_function_with_context(FunctionPtr parent_f, const std::string &ctx_function_name) {
  auto root = parent_f->root.clone();
  root->name()->set_string(ctx_function_name);

  auto context_function = FunctionData::clone_from(parent_f, root);
  context_function->has_variadic_param = false;

  return context_function;
}

void SortAndInheritClassesF::inherit_static_method_from_parent(ClassPtr child_class, const ClassMemberStaticMethod &parent_method, DataStream<FunctionPtr> &function_stream) {
  auto local_name = parent_method.local_name();
  auto parent_f = parent_method.function;
  auto parent_class = parent_f->class_id;
  if (parent_f->is_auto_inherited) {      // A::f() -> B -> C; при A->B сделался A::f$$B
    return;                               // но при B->C должно быть A::f$$C, а не B::f$$C
  }                                       // (чтобы B::f$$C не считать required)
  if (parent_f->is_final) {
    kphp_error(0, format("Can not override method marked as 'final': %s", parent_f->get_human_readable_name().c_str()));
    return;
  }

  if (!child_class->members.has_static_method(local_name)) {
    auto child_root = generate_function_with_parent_call(parent_f, child_class, parent_method);

    FunctionPtr child_function = FunctionData::clone_from(parent_f, child_root);
    child_function->is_auto_inherited = true;
    child_function->is_inline = true;

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
  kphp_error_return(parent_class->is_class() && child_class->is_class(),
                    format("Error extends %s and %s", child_class->name.c_str(), parent_class->name.c_str()));

  kphp_error_return(!child_class->members.has_any_instance_method() && !parent_class->members.has_any_instance_method(),
                    format("Invalid class extends %s and %s: extends is available only if classes are only-static",
                           child_class->name.c_str(), parent_class->name.c_str()));

  child_class->parent_class = parent_class;

  // A::f -> B -> C -> D; для D нужно C::f$$D, B::f$$D, A::f$$D
  for (; parent_class; parent_class = parent_class->parent_class) {
    parent_class->members.for_each([&](const ClassMemberStaticMethod &m) {
      inherit_static_method_from_parent(child_class, m, function_stream);
    });
  }
}

void SortAndInheritClassesF::inherit_class_from_interface(ClassPtr child_class, InterfacePtr interface_class) {
  kphp_error(interface_class->is_interface(),
             format("Error implements %s and %s", child_class->name.c_str(), interface_class->name.c_str()));
  child_class->implements.emplace_back(interface_class);
  AutoLocker<Lockable *> locker(&(*interface_class));
  interface_class->derived_classes.emplace_back(child_class);
}

/**
 * Каждый класс поступает сюда один и ровно один раз — когда он и все его dependents
 * (родители, трейты, интерфейсы) тоже готовы.
 */
void SortAndInheritClassesF::on_class_ready(ClassPtr klass, DataStream<FunctionPtr> &function_stream) {
  for (const auto &dep : klass->str_dependents) {
    ClassPtr dep_class = G->get_class(dep.class_name);

    switch (dep.type) {
      case ClassType::klass:
        inherit_child_class_from_parent(klass, dep_class, function_stream);
        break;
      case ClassType::interface:
        inherit_class_from_interface(klass, dep_class);
        break;
      case ClassType::trait:
        kphp_assert(0 && "mixin traits is not supported yet");
        break;
    }
  }

  if (!klass->phpdoc_str.empty()) {
    analyze_class_phpdoc(klass);
  }
}

inline void SortAndInheritClassesF::analyze_class_phpdoc(ClassPtr klass) {
  for (const auto &tag : parse_php_doc(klass->phpdoc_str)) {
    if (tag.type == php_doc_tag::kphp_memcache_class) {
      G->set_memcache_class(klass);
    }
  }
}


void SortAndInheritClassesF::execute(ClassPtr klass, MultipleDataStreams<FunctionPtr, ClassPtr> &os) {
  auto &function_stream = *os.template project_to_nth_data_stream<0>();
  auto &restart_class_stream = *os.template project_to_nth_data_stream<1>();

  if (auto dependency = get_not_ready_dependency(klass)) {
    AutoLocker<Lockable*> locker(dependency);
    if (dependency->data.done) {              // вдруг между вызовом ready и этим моментом стало done
      restart_class_stream << klass;
    } else {
      dependency->data.waiting.emplace_front(klass);
    }
    return;
  }

  on_class_ready(klass, function_stream);

  auto node = ht.at(vk::std_hash(klass->name));
  kphp_assert(!node->data.done);

  AutoLocker<Lockable *> locker(node);
  for (ClassPtr restart_klass: node->data.waiting) {    // все классы, которые ждали текущего —
    restart_class_stream << restart_klass;              // запустить SortAndInheritClassesF ещё раз для них
  }

  node->data.waiting.clear();
  node->data.done = true;
}
void SortAndInheritClassesF::check_on_finish() {
  for (auto c : G->get_classes()) {
    auto node = ht.at(vk::std_hash(c->name));
    kphp_error(node->data.done, format("class `%s` has unresolved dependencies", c->name.c_str()));

    // For stable code generation of interface method body
    if (c->is_interface()) {
      std::sort(c->derived_classes.begin(), c->derived_classes.end());
    }
  }
}
