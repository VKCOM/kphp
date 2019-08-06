#include "compiler/data/class-data.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/inferring/type-data.h"
#include "compiler/phpdoc.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"
#include "common/termformat/termformat.h"

const char * ClassData::NAME_OF_VIRT_CLONE = "__virt_clone$";

ClassData::ClassData() :
  members(this),
  type_data(TypeData::create_for_class(ClassPtr(this))) {
}

void ClassData::set_name_and_src_name(const string &name, const vk::string_view &phpdoc_str) {
  this->name = name;
  this->src_name = std::string("C$").append(replace_backslashes(name));
  this->header_name = replace_characters(src_name + ".h", '$', '@');
  this->phpdoc_str = phpdoc_str;
  this->is_tl_class = !phpdoc_str.empty() && PhpDocTypeRuleParser::is_tag_in_phpdoc(phpdoc_str, php_doc_tag::kphp_tl_class);

  size_t pos = name.find_last_of('\\');
  std::string namespace_name = pos == std::string::npos ? "" : name.substr(0, pos);
  std::string class_name = pos == std::string::npos ? name : name.substr(pos + 1);

  this->can_be_php_autoloaded = file_id && namespace_name == file_id->namespace_name && class_name == file_id->short_file_name;
  this->can_be_php_autoloaded |= this->is_builtin();
}

void ClassData::debugPrint() {
  const char *str_class_type =
    is_interface() ? "interface" :
    is_trait() ? "trait" : "class";
  printf("=== %s %s\n", str_class_type, name.c_str());

  members.for_each([](ClassMemberConstant &m) {
    printf("const %s\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberStaticField &m) {
    printf("static $%s\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberStaticMethod &m) {
    printf("static %s()\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberInstanceField &m) {
    printf("var $%s\n", m.local_name().c_str());
  });
  members.for_each([](ClassMemberInstanceMethod &m) {
    printf("method %s()\n", m.local_name().c_str());
  });
}

std::string ClassData::get_namespace() const {
  return file_id->namespace_name;
}

VertexAdaptor<op_var> ClassData::gen_vertex_this_with_type_rule(Location location) {
  auto this_var = gen_vertex_this(location);
  auto rule_this_var = GenTree::create_type_help_class_vertex(ClassPtr{this});

  this_var->type_rule = VertexAdaptor<op_common_type_rule>::create(rule_this_var);
  return this_var;
}

FunctionPtr ClassData::gen_holder_function(const std::string &name) {
  std::string func_name = "$" + name;  // function-wrapper for class
  auto func_params = VertexAdaptor<op_func_param_list>::create();
  auto func_body = VertexAdaptor<op_seq>::create();
  auto func_root = VertexAdaptor<op_function>::create(func_params, func_body);

  auto res = FunctionData::create_function(func_name, func_root, FunctionData::func_class_holder);
  res->class_id = ClassPtr{this};
  return res;
}

/**
 * Generate and add `__virt_clone` method to class (or interface); method is dedicated for cloning interfaces
 *
 * For classes:
 * function __virt_clone() { return clone $this; }
 *
 * For interfaces:
 * / ** @return InterfaceName * /
 * function __virt_clone();
 *
 * @param os for register new method
 * @param with_body when it's true, generates only empty body it's needed for interfaces
 * @return generated method
 */
FunctionPtr ClassData::add_virt_clone(DataStream<FunctionPtr> &os, bool with_body /** =true */) {
  std::string clone_func_name = replace_backslashes(name) + "$$" + NAME_OF_VIRT_CLONE;

  std::vector<VertexAdaptor<meta_op_func_param>> params;
  patch_func_add_this(params, Location{});
  auto param_list = VertexAdaptor<op_func_param_list>::create(params);

  VertexAdaptor<op_seq> body;
  if (with_body) {
    auto clone_this = VertexAdaptor<op_clone>::create(gen_vertex_this(Location{}));
    auto return_clone_this = VertexAdaptor<op_return>::create(clone_this);
    body = VertexAdaptor<op_seq>::create(return_clone_this);
  } else {
    body = VertexAdaptor<op_seq>::create();
  }

  auto virt_clone_func = VertexAdaptor<op_function>::create(param_list, body);
  auto virt_clone_func_ptr = FunctionData::create_function(clone_func_name, virt_clone_func, FunctionData::func_local);
  virt_clone_func_ptr->file_id = file_id;
  virt_clone_func_ptr->update_location_in_body();
  virt_clone_func_ptr->assumptions_inited_return = 2;
  virt_clone_func_ptr->assumption_for_return = Assumption{AssumType::assum_instance, {}, ClassPtr{this}};
  virt_clone_func_ptr->is_inline = with_body;

  members.add_instance_method(virt_clone_func_ptr, AccessType::access_public);
  G->register_and_require_function(virt_clone_func_ptr, os, true);

  return virt_clone_func_ptr;
}

void ClassData::create_default_constructor(Location location, DataStream<FunctionPtr> &os) {
  auto func = VertexAdaptor<op_function>::create(VertexAdaptor<op_func_param_list>::create(),
                                                 VertexAdaptor<op_seq>::create());
  func->location = location;
  create_constructor(func);

  G->register_and_require_function(construct_function, os, true);
}

void ClassData::create_constructor(VertexAdaptor<op_function> func) {
  std::string func_name = replace_backslashes(name) + "$$__construct";
  auto params = func->params()->get_next();
  patch_func_add_this(params, func->get_location());
  func->params_ref() = VertexAdaptor<op_func_param_list>::create(std::move(params));

  GenTree::func_force_return(func, gen_vertex_this(func->location));

  auto ctor_function = FunctionData::create_function(func_name, func, FunctionData::func_local);
  ctor_function->update_location_in_body();
  ctor_function->is_inline = true;
  members.add_instance_method(ctor_function, access_public);
}

bool ClassData::has_only_default_constructor() const {
  kphp_assert(construct_function && construct_function->root);
  bool constructor_contains_only_return_this_statement = construct_function->root->cmd()->size() == 1;
  return constructor_contains_only_return_this_statement;
}

template<Operation Op>
void ClassData::patch_func_add_this(std::vector<VertexAdaptor<Op>> &params_next, Location location) {
  static_assert(vk::any_of_equal(Op, meta_op_base, meta_op_func_param, op_func_param), "disallowed vector of Operation");
  auto vertex_this = gen_vertex_this_with_type_rule(location);
  auto param_this = VertexAdaptor<op_func_param>::create(vertex_this);
  params_next.emplace(params_next.begin(), param_this);
}

bool ClassData::is_parent_of(ClassPtr other) {
  return other->parent_class && (other->parent_class == ClassPtr{this} || is_parent_of(other->parent_class));
}

InterfacePtr ClassData::get_common_base_or_interface(ClassPtr other) const {
  if (!other) {
    return {};
  }

  auto self = get_self();
  if (self == other) {
    return self;
  }

  if (class_type == other->class_type) {
    switch (class_type) {
      case ClassType::klass: {
        if (implements.size() == other->implements.size() && implements.size() == 1) {
          return implements[0]->get_common_base_or_interface(other->implements[0]);
        }
      }
      /* fallthrough */
      case ClassType::interface: {
        // performance doesn't matter
        for (; self && other; self = self->parent_class, other = other->parent_class) {
          if (self == other) {
            return self;
          } else if (self->is_parent_of(other)) {
            return self;
          } else if (other->is_parent_of(self)) {
            return other;
          }
        }
        break;
      }
      default:
        break;
    }
  } else {
    if (other->is_class()) {
      std::swap(self, other);
    }

    if (self->is_class() && other->is_interface()) {
      auto top_parent = self->get_top_parent();
      kphp_assert(top_parent->implements.size() == 1);
      return top_parent->implements[0]->get_common_base_or_interface(other);
    }
  }

  return {};
}

const ClassMemberInstanceMethod *ClassData::get_instance_method(const std::string &local_name) const {
  return find_by_local_name<ClassMemberInstanceMethod>(local_name);
}

const ClassMemberInstanceField *ClassData::get_instance_field(const std::string &local_name) const {
  return find_by_local_name<ClassMemberInstanceField>(local_name);
}

const ClassMemberStaticField *ClassData::get_static_field(const std::string &local_name) const {
  return find_by_local_name<ClassMemberStaticField>(local_name);
}

const ClassMemberConstant *ClassData::get_constant(const std::string &local_name) const {
  return find_by_local_name<ClassMemberConstant>(local_name);
}

void ClassData::check_parent_constructor() {
  if (!construct_function || !construct_function->root) {
    return;
  }

  if (!parent_class || !parent_class->construct_function || !parent_class->construct_function->root) {
    return;
  }

  if (has_only_default_constructor() && !parent_class->has_only_default_constructor()) {
    kphp_error(false,
               format("You must write `__constructor` in class: %s because one of your parent(%s) has constructor",
                      TermStringFormat::paint_green(name).c_str(), TermStringFormat::paint_green(parent_class->name).c_str())
    );
  }
}

VertexAdaptor<op_var> ClassData::gen_vertex_this(Location location) {
  auto this_var = VertexAdaptor<op_var>::create();
  this_var->str_val = "this";
  this_var->extra_type = op_ex_var_this;
  this_var->const_type = cnst_const_val;
  this_var->location = location;
  this_var->is_const = true;

  return this_var;
}

bool ClassData::need_generate_accept_method() const {
  return !is_lambda();
}

bool ClassData::is_builtin() const {
  return file_id && file_id->is_builtin();
}

bool ClassData::is_interface_or_has_interface_member() const {
  if (is_interface()) {
    return true;
  }
  std::unordered_set<ClassPtr> checked{get_self()};
  return has_interface_member_dfs(checked);
}

bool ClassData::has_interface_member_dfs(std::unordered_set<ClassPtr> &checked) const {
  return nullptr != members.find_member(
    [&checked](const ClassMemberInstanceField &field) {
      std::unordered_set<ClassPtr> sub_classes;
      field.var->tinf_node.get_type()->get_all_class_types_inside(sub_classes);
      for (auto klass : sub_classes) {
        if (checked.insert(klass).second) {
          if (klass->is_interface() || klass->has_interface_member_dfs(checked)) {
            return true;
          }
        }
      }
      return false;
    });
}

bool ClassData::does_need_codegen(ClassPtr c) {
  return c && !c->is_fully_static() && !c->is_builtin() && c->really_used;
}

bool operator<(const ClassPtr &lhs, const ClassPtr &rhs) {
  return lhs->name < rhs->name;
}

void ClassData::mark_as_used() {
  if (really_used)
    return;
  really_used = true;
  if (parent_class) {
    parent_class->mark_as_used();
  }
  if (implements.size()) {
    kphp_assert(implements.size() == 1);
    implements[0]->mark_as_used();
  }
}
