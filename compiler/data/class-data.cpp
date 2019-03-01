#include "compiler/data/class-data.h"

#include "compiler/compiler-core.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/data/src-file.h"
#include "compiler/gentree.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

ClassData::ClassData() :
  id(0),
  class_type(ctype_class),
  assumptions_inited_vars(0),
  was_constructor_invoked(false),
  can_be_php_autoloaded(false),
  members(this),
  type_data(TypeData::create_for_class(ClassPtr(this))) {
}


void ClassData::set_name_and_src_name(const string &name) {
  this->name = name;
  this->src_name = std::string("C$").append(replace_backslashes(name));
  this->header_name = replace_characters(src_name + ".h", '$', '@');

  size_t pos = name.find_last_of('\\');
  std::string namespace_name = pos == std::string::npos ? "" : name.substr(0, pos);
  std::string class_name = pos == std::string::npos ? name : name.substr(pos + 1);

  this->can_be_php_autoloaded = file_id && namespace_name == file_id->namespace_name && class_name == file_id->short_file_name;
  this->can_be_php_autoloaded |= this->is_builtin();
}

void ClassData::debugPrint() {
  string str_class_type =
    class_type == ctype_interface ? "interface" :
    class_type == ctype_trait ? "trait" :
    "class";
  printf("=== %s %s\n", str_class_type.c_str(), name.c_str());

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

VertexAdaptor<op_var> ClassData::gen_vertex_this_with_type_rule(int location_line_num) {
  auto this_var = gen_vertex_this(location_line_num);
  auto rule_this_var = GenTree::create_type_help_class_vertex(ClassPtr{this});

  this_var->type_rule = VertexAdaptor<op_common_type_rule>::create(rule_this_var);
  return this_var;
}

FunctionPtr ClassData::gen_holder_function(const std::string &name) {
  auto func_name = VertexAdaptor<op_func_name>::create();
  func_name->str_val = "$" + name;  // function-wrapper for class
  auto func_params = VertexAdaptor<op_func_param_list>::create();
  auto func_body = VertexAdaptor<op_seq>::create();
  auto func_root = VertexAdaptor<op_function>::create(func_name, func_params, func_body);

  auto res = FunctionData::create_function(func_root, FunctionData::func_class_holder);
  res->class_id = ClassPtr{this};
  return res;
}

void ClassData::patch_func_constructor(VertexAdaptor<op_function> func, int location_line_num) {
  auto return_node = VertexAdaptor<op_return>::create(ClassData::gen_vertex_this(location_line_num));
  return_node->location.set_line(location_line_num);

  std::vector<VertexPtr> next = func->cmd()->get_next();
  next.insert(next.begin(), gen_vertex_this_with_type_rule(location_line_num));

  // выносим "$var = 0" в начало конструктора; переменные класса — в порядке, обратном объявлению, это не страшно
  members.for_each([&](ClassMemberInstanceField &f) {
    if (f.def_val) {
      auto inst_prop = VertexAdaptor<op_instance_prop>::create(ClassData::gen_vertex_this(location_line_num));
      inst_prop->location = f.root->location;
      inst_prop->str_val = f.local_name();

      next.insert(next.begin() + 1, VertexAdaptor<op_set>::create(inst_prop, f.def_val));
    }
  });

  next.emplace_back(return_node);

  func->cmd() = VertexAdaptor<op_seq>::create(next);
}

void ClassData::create_default_constructor(int location_line_num, DataStream<FunctionPtr> &os) {
  create_constructor_with_args(location_line_num, VertexAdaptor<op_func_param_list>::create(), os);
}

void ClassData::create_constructor_with_args(int location_line_num, VertexAdaptor<op_func_param_list> params) {
  static DataStream<FunctionPtr> unused;
  create_constructor_with_args(location_line_num, params, unused, false);
  kphp_assert(!construct_function->is_required);
}

void ClassData::create_constructor_with_args(int location_line_num, VertexAdaptor<op_func_param_list> params, DataStream<FunctionPtr> &os, bool auto_required) {
  auto func_name = VertexAdaptor<op_func_name>::create();
  func_name->str_val = replace_backslashes(name) + "$$__construct";

  std::vector<VertexPtr> fields_initializers;
  for (auto param : params->params()) {
    param = param.as<op_func_param>()->var();
    auto inst_prop = VertexAdaptor<op_instance_prop>::create(gen_vertex_this(location_line_num));
    inst_prop->location.set_line(location_line_num);
    inst_prop->str_val = param->get_string();

    fields_initializers.emplace_back(VertexAdaptor<op_set>::create(inst_prop, param.clone()));
  }
  auto func_root = VertexAdaptor<op_seq>::create(fields_initializers);

  auto func = VertexAdaptor<op_function>::create(func_name, params, func_root);
  func->location.set_line(location_line_num);

  patch_func_constructor(func, location_line_num);

  auto ctor_function = FunctionData::create_function(func, FunctionData::func_local);
  ctor_function->update_location_in_body();
  ctor_function->is_inline = true;
  members.add_instance_method(ctor_function, access_public);
  G->register_and_require_function(ctor_function, os, auto_required);
}

void ClassData::patch_func_add_this(vector<VertexAdaptor<meta_op_func_param>> &params_next, int location_line_num) {
  auto vertex_this = gen_vertex_this_with_type_rule(location_line_num);
  auto param_this = VertexAdaptor<op_func_param>::create(vertex_this);
  params_next.emplace(params_next.begin(), param_this);
}

VertexAdaptor<op_var> ClassData::gen_vertex_this(int location_line_num) {
  auto this_var = VertexAdaptor<op_var>::create();
  this_var->str_val = "this";
  this_var->extra_type = op_ex_var_this;
  this_var->const_type = cnst_const_val;
  this_var->location.line = location_line_num;

  return this_var;
}

bool ClassData::is_builtin() const {
  // TODO: how can be !file_id?
  return file_id && file_id->file_name == G->env().get_functions();
}
