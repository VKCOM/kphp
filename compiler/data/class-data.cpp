#include "compiler/data/class-data.h"

#include "compiler/compiler-core.h"
#include "compiler/data/src-file.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lambda-class-data.h"
#include "compiler/utils/string-utils.h"
#include "compiler/vertex.h"

ClassData::ClassData() :
  id(0),
  class_type(ctype_class),
  assumptions_inited_vars(0),
  was_constructor_invoked(false),
  can_be_php_autoloaded(false),
  members(this) {
}


void ClassData::set_name_and_src_name(const string &name) {
  this->name = name;
  this->src_name = std::string("C$").append(replace_backslashes(name));
  this->header_name = replace_characters(src_name + ".h", '$', '@');

  size_t pos = name.find_last_of('\\');
  std::string namespace_name = pos == std::string::npos ? "" : name.substr(0, pos);
  std::string class_name = pos == std::string::npos ? name : name.substr(pos + 1);

  this->can_be_php_autoloaded = file_id && namespace_name == file_id->namespace_name && class_name == file_id->short_file_name;
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

VertexPtr ClassData::gen_constructor_call_pass_fields_as_args() const {
  std::vector<VertexPtr> args;
  members.for_each([&](const ClassMemberInstanceField &field) {
    VertexPtr res = VertexAdaptor<op_var>::create();
    if (field.local_name() == "parent$this") {
      res->set_string("this");
    } else {
      res->set_string(field.root->get_string());
    }
    res->location = field.root->location;
    args.emplace_back(res);
  });

  return gen_constructor_call_with_args(std::move(args));
}

VertexAdaptor<op_constructor_call> ClassData::gen_constructor_call_with_args(std::vector<VertexPtr> args) const {
  auto constructor_call = VertexAdaptor<op_constructor_call>::create(std::move(args));
  constructor_call->set_string(name);
  constructor_call->set_func_id(construct_function);

  return constructor_call;
}

VertexAdaptor<op_var> ClassData::gen_vertex_this_with_type_rule(int location_line_num) {
  auto this_var = gen_vertex_this(location_line_num);
  auto rule_this_var = VertexAdaptor<op_class_type_rule>::create();
  rule_this_var->type_help = tp_Class;
  rule_this_var->class_ptr = ClassPtr{this};

  this_var->type_rule = VertexAdaptor<op_common_type_rule>::create(rule_this_var);
  return this_var;
}

void ClassData::patch_func_add_this(vector<VertexPtr> &params_next, int location_line_num) {
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
