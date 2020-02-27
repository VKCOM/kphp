#include "compiler/code-gen/declarations.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/naming.h"
#include "compiler/code-gen/raw-data.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/type-data.h"
#include "compiler/tl-classes.h"
#include "compiler/vertex.h"

VarDeclaration VarExternDeclaration(VarPtr var) {
  return VarDeclaration(var, true, false);
}

VarDeclaration VarPlainDeclaration(VarPtr var) {
  return VarDeclaration(var, false, false);
}

VarDeclaration::VarDeclaration(VarPtr var, bool extern_flag, bool defval_flag) :
  var(var),
  extern_flag(extern_flag),
  defval_flag(defval_flag) {
}

void VarDeclaration::compile(CodeGenerator &W) const {
  const TypeData *type = tinf::get_type(var);

  if (var->is_builtin_global()) {
    W << CloseNamespace();
  }

  kphp_assert(type->ptype() != tp_void);

  W << (extern_flag ? "extern " : "") << TypeName(type) << " " << VarName(var);

  if (defval_flag) {
    if (vk::any_of_equal(type->ptype(), tp_float, tp_int, tp_future, tp_future_queue)) {
      W << " = 0";
    } else if (type->get_real_ptype() == tp_bool) {
      W << " = false";
    }
  }
  W << ";" << NL;
  if (var->needs_const_iterator_flag) {
    for (const auto &name : {"$it", "$it$end"}) {
      W << (extern_flag ? "extern " : "") <<
        "decltype(const_begin(" << VarName(var) << "))" << " " << VarName(var) << name << ";" << NL;
    }
  }

  if (var->is_builtin_global()) {
    W << OpenNamespace();
  }
}

FunctionDeclaration::FunctionDeclaration(FunctionPtr function, bool in_header, gen_out_style style) :
  function(function),
  in_header(in_header),
  style(style) {
}

void FunctionDeclaration::compile(CodeGenerator &W) const {
  TypeName ret_type_gen(tinf::get_type(function, -1), style);
  FunctionParams params_gen(function, in_header, style);

  switch (style) {
    case gen_out_style::tagger:
    case gen_out_style::cpp: {
      FunctionSignatureGenerator(W) << ret_type_gen << " " << FunctionName(function) << "(" << params_gen << ")";
      break;
    }
    case gen_out_style::txt: {
      W << "function " << function->name << "(" << params_gen << ") ::: " << ret_type_gen;
      break;
    }
  }
}

FunctionForkDeclaration::FunctionForkDeclaration(FunctionPtr function, bool in_header) :
  function(function),
  in_header(in_header) {
}

void FunctionForkDeclaration::compile(CodeGenerator &W) const {
  FunctionSignatureGenerator(W) << "int " << FunctionForkName(function) <<
                                "(" << FunctionParams(function, in_header) << ")";
}

FunctionParams::FunctionParams(FunctionPtr function, bool in_header, gen_out_style style) :
  FunctionParams(function, 0, in_header, style) {
}
FunctionParams::FunctionParams(FunctionPtr function, size_t shift, bool in_header, gen_out_style style) :
  function(function),
  params(function->get_params()),
  in_header(in_header),
  style(style),
  shift(shift) {
  if (shift > 0) {
    params = {std::next(params.begin(), shift), params.end()};
  }
}

void FunctionParams::declare_cpp_param(CodeGenerator &W, VertexAdaptor<op_var> var, const TypeName &type) const {
  W << type << " ";
  auto var_ptr = var->var_id;
  if (var->ref_flag) {
    W << "&";
  } else if (var_ptr->marked_as_const || (!function->has_variadic_param && var_ptr->is_read_only)) {
    W << "const &";
  }
  W << VarName(var_ptr);
}

void FunctionParams::declare_txt_param(CodeGenerator &W, VertexAdaptor<op_var> var, const TypeName &type) const {
  if (var->ref_flag) {
    W << "&";
  }
  W << "$" << var->var_id->name << " :<=: " << type;
}

void FunctionParams::compile(CodeGenerator &W) const {
  bool first = true;
  size_t ii = shift;
  for (auto i : params) {
    if (i->type() == op_func_param) {
      assert ("functions with callback are not supported");
    }

    if (first) {
      first = false;
    } else {
      W << ", ";
    }

    auto param = i.as<op_func_param>();
    auto var = param->var();
    TypeName type_gen(tinf::get_type(function, ii), style);
    switch (style) {
      case gen_out_style::tagger:
      case gen_out_style::cpp: {
        declare_cpp_param(W, var, type_gen);
        if (param->has_default_value() && param->default_value() && in_header) {
          W << " = " << param->default_value();
        }
        break;
      }
      case gen_out_style::txt: {
        declare_txt_param(W, var, type_gen);
        if (param->has_default_value() && param->default_value() && in_header) {
          VertexPtr default_value = GenTree::get_actual_value(param->default_value());
          kphp_assert(vk::any_of_equal(default_value->type(), op_int_const, op_float_const));
          W << " = " << default_value->get_string();
        }
        break;
      }
    }
    ii++;
  }
}

InterfaceDeclaration::InterfaceDeclaration(InterfacePtr interface) :
  interface(interface) {
}

TlDependentTypesUsings::TlDependentTypesUsings(vk::tl::type *tl_type, const std::string &php_tl_class_name) {
  int template_suf_start = php_tl_class_name.find("__");
  kphp_assert(template_suf_start != std::string::npos);
  specialization_suffix = php_tl_class_name.substr(template_suf_start);
  for (const auto &constructor : tl_type->constructors) {
    for (const auto &arg : constructor->args) {
      if (arg->is_optional()) {
        continue;
      }
      cur_tl_constructor = constructor.get();
      cur_tl_arg = arg.get();
      std::vector<InnerParamTypeAccess> recursion_stack;
      deduce_params_from_type_tree(arg->type_expr.get(), recursion_stack);
      kphp_assert(recursion_stack.empty());
    }
  }
}

TlDependentTypesUsings::DeducingInfo::DeducingInfo(std::string deduced_type, vector<TlDependentTypesUsings::InnerParamTypeAccess> path) :
  deduced_type(std::move(deduced_type)),
  path_to_inner_param(std::move(path)) {}

// Обходим дерево, игнорируя все вершины, кроме vk::tl::type_var и vk::tl::type_expr. Собираем пути до type_var'ов для того чтобы получить тип.
// Пример того, что хотим получить:
// ********************** tl-схема **********************
// test1 {t1 : Type} {t2 : Type}
//  x : (Either (Maybe (%Vector t1)) (Maybe (%VectorTotal t2)))
// = Test t1 t2;
// ********************** cl/C@VK@TL@Types@Test__int__graph_Vertex.h **********************
//struct C$VK$TL$Types$Test__int__graph_Vertex : public abstract_refcountable_php_interface {
//  using t1 = class_instance<C$VK$TL$Types$Either__maybe_array_int__VectorTotal__graph_Vertex>::ClassType::X::InnerType::ValueType;
//  using t2 = class_instance<C$VK$TL$Types$Either__maybe_array_int__VectorTotal__graph_Vertex>::ClassType::Y::ClassType::t;
//  virtual const char *get_class() const {
//    return "VK\\TL\\Types\\Test__int__graph_Vertex";
//  }
//};
// Считаем, что типовых переменных нет внутри tl массивов, т.е нет такого:
//  test {t:Type} [x:t] = Test;
// Все такие случаи (vector, tuple) написаны руками в tl_builtins.h
void TlDependentTypesUsings::deduce_params_from_type_tree(vk::tl::type_expr_base *type_tree, std::vector<InnerParamTypeAccess> &recursion_stack) {
  if (auto type_var = dynamic_cast<vk::tl::type_var *>(type_tree)) {
    std::string type_var_name = cur_tl_constructor->get_var_num_arg(type_var->var_num)->name;
    if (!deduced_params.count(type_var_name)) {
      ClassPtr tl_constructor_php_class = tl2cpp::get_php_class_of_tl_constructor_specialization(cur_tl_constructor, specialization_suffix);
      const TypeData *deduced_type = tl_constructor_php_class->members.get_instance_field(cur_tl_arg->name)->get_inferred_type();
      deduced_params[type_var_name] = DeducingInfo(type_out(deduced_type), recursion_stack);
      std::unordered_set<ClassPtr> classes;
      deduced_type->get_all_class_types_inside(classes);
      for (const auto &klass : classes) {
        kphp_assert(klass);
        dependencies.add_class_forward_declaration(klass);
      }
    }
    return;
  }
  if (auto type_expr = dynamic_cast<vk::tl::type_expr *>(type_tree)) {
    int i = 0;
    for (const auto &child : type_expr->children) {
      if (auto casted_child = dynamic_cast<vk::tl::type_expr_base *>(child.get())) {
        vk::tl::type *parent_tl_type = G->get_tl_classes().get_scheme()->get_type_by_magic(type_expr->type_id);
        kphp_assert(parent_tl_type);

        InnerParamTypeAccess inner_access;
        bool skip_maybe = false;
        if (tl2cpp::is_tl_type_a_php_array(parent_tl_type)) {
          inner_access.drop_class_instance = false;
          inner_access.inner_type_name = "ValueType";
        } else if (parent_tl_type->name == "Maybe") {
          auto child_type_expr = dynamic_cast<vk::tl::type_expr *>(casted_child);
          kphp_assert(child_type_expr);
          vk::tl::type *child_tl_type = G->get_tl_classes().get_scheme()->get_type_by_magic(child_type_expr->type_id);
          kphp_assert(child_tl_type);
          if (tl2cpp::is_tl_type_wrapped_to_Optional(child_tl_type)) {
            inner_access.drop_class_instance = false;
            inner_access.inner_type_name = "InnerType";
          } else {
            skip_maybe = true;
          }
        } else {
          inner_access.drop_class_instance = true;
          inner_access.inner_type_name = parent_tl_type->constructors[0]->args[i]->name; // корректность такого проверяется в tl2cpp.cpp::check_constructor()
          auto php_classes = tl2cpp::get_all_php_classes_of_tl_type(parent_tl_type);
          std::for_each(php_classes.begin(), php_classes.end(), [&](ClassPtr klass){ dependencies.add_class_include(klass); });
        }
        if (!skip_maybe) {
          recursion_stack.emplace_back(inner_access);
        }
        deduce_params_from_type_tree(casted_child, recursion_stack);
        if (!skip_maybe) {
          recursion_stack.pop_back();
        }
      }
      ++i;
    }
    return;
  }
}

void TlDependentTypesUsings::compile(CodeGenerator &W) const {
  for (const auto &item : deduced_params) {
    const auto &info = item.second;
    W << "using " << item.first << " = " << info.deduced_type;
    for (const auto &step : info.path_to_inner_param) {
      if (step.drop_class_instance) {
        W << "::ClassType";
      }
      W << "::" << step.inner_type_name;
    }
    W << ";" << NL;
  }
}

void TlDependentTypesUsings::compile_dependencies(CodeGenerator &W) {
  W << dependencies << NL;
}

std::unique_ptr<TlDependentTypesUsings> InterfaceDeclaration::detect_if_needs_tl_usings() const {
  if (tl2cpp::is_php_class_a_tl_polymorphic_type(interface)) {
    vk::tl::type *cur_tl_type = tl2cpp::get_tl_type_of_php_class(interface);

    bool needs_tl_usings = tl2cpp::is_type_dependent(cur_tl_type, G->get_tl_classes().get_scheme().get());
    if (needs_tl_usings) {
      return std::make_unique<TlDependentTypesUsings>(cur_tl_type, interface->name);
    }
  }
  return {};
}

void InterfaceDeclaration::compile(CodeGenerator &W) const {
  W << OpenFile(interface->header_name, interface->get_subdir());
  W << "#pragma once" << NL;

  std::string parent_class;
  if (auto parent = interface->parent_class) {
    parent_class = parent->src_name;
    IncludesCollector includes;
    includes.add_class_include(parent);
    W << includes;
  } else {
    parent_class = "abstract_refcountable_php_interface";
  }

  W << OpenNamespace() << NL;

  std::unique_ptr<TlDependentTypesUsings> tl_dep_usings = detect_if_needs_tl_usings();
  if (tl_dep_usings) {
    tl_dep_usings->compile_dependencies(W);
  }

  W << "struct " << interface->src_name << " : public " << parent_class << " " << BEGIN;

  if (tl_dep_usings) {
    W << *tl_dep_usings << NL;
  }

  ClassDeclaration::compile_inner_methods(W, interface);


  W << END << ";" << NL;

  W << CloseNamespace();

  W << CloseFile();
}

ClassDeclaration::ClassDeclaration(ClassPtr klass) :
  klass(klass) {
}

void ClassDeclaration::declare_all_variables(VertexPtr vertex, CodeGenerator &W) const {
  if (!vertex) {
    return;
  }
  for (auto child: *vertex) {
    declare_all_variables(child, W);
  }
  if (auto var = vertex.try_as<op_var>()) {
    W << VarExternDeclaration(var->var_id);
  }
}

std::unique_ptr<TlDependentTypesUsings> ClassDeclaration::detect_if_needs_tl_usings() const {
  if (tl2cpp::is_php_class_a_tl_constructor(klass) && !tl2cpp::is_php_class_a_tl_array_item(klass)) {
    const auto &scheme = G->get_tl_classes().get_scheme();
    vk::tl::combinator *cur_tl_constructor = tl2cpp::get_tl_constructor_of_php_class(klass);
    vk::tl::type *cur_tl_type = scheme->get_type_by_magic(cur_tl_constructor->type_id);

    bool needs_tl_usings = tl2cpp::is_type_dependent(cur_tl_constructor, scheme.get()) && !cur_tl_type->is_polymorphic();
    if (needs_tl_usings) {
      return std::make_unique<TlDependentTypesUsings>(cur_tl_type, klass->name);
    }
  }
  return {};
}

void ClassDeclaration::compile(CodeGenerator &W) const {
  W << OpenFile(klass->header_name, klass->get_subdir());
  W << "#pragma once" << NL;

  auto front_includes = compile_front_includes(W);

  W << OpenNamespace();

  std::unique_ptr<TlDependentTypesUsings> tl_dep_usings = detect_if_needs_tl_usings();
  if (tl_dep_usings) {
    tl_dep_usings->compile_dependencies(W);
  }

  klass->members.for_each([&](const ClassMemberInstanceField &f) {
    if (f.var->init_val) {
      declare_all_variables(f.var->init_val, W);
    }
  });

  InterfacePtr interface;
  if (!klass->implements.empty()) {
    kphp_assert(klass->implements.size() == 1);
    interface = klass->implements.front();
  }

  W << NL << "struct " << klass->src_name;
  if (ClassData::does_need_codegen(klass->parent_class)) {
    auto final_keyword = klass->derived_classes.empty() ? " final" : "";
    W << final_keyword << " : public " << klass->parent_class->src_name;
  } else if (klass->is_empty_class()) {
    W << " final : public refcountable_empty_php_classes";
  } else if (interface || !klass->derived_classes.empty()) {
    auto base_name = interface ? interface->src_name.c_str() : "abstract_refcountable_php_interface";
    W << ": public refcountable_polymorphic_php_classes<" << base_name << ">";
  } else {
    W << ": public refcountable_php_classes<" << klass->src_name << ">";
  }
  W << " " << BEGIN;

  if (tl_dep_usings) {
    W << *tl_dep_usings << NL;
  }

  klass->members.for_each([&](const ClassMemberInstanceField &f) {
    W << TypeName(tinf::get_type(f.var)) << " $" << f.local_name() << "{";
    if (f.var->init_val) {
      W << f.var->init_val;
    }
    W << "};" << NL;
  });

  if (!klass->derived_classes.empty()) {
    W << "virtual ~" << klass->src_name << "() = default;" << NL;
  }

  if (!klass->is_lambda()) {
    compile_inner_methods(W, klass);
  } else {
    compile_accept_visitor_methods(W, klass);
  }

  // для rpc-функций генерим метод-член класса store(), который перевызывает сторилку из codegen tl2cpp
  if (tl2cpp::is_php_class_a_tl_function(klass)) {
    W << NL;
    FunctionSignatureGenerator(W).set_final().set_const_this() << "std::unique_ptr<tl_func_base> store() " << BEGIN;
    std::string f_tl_cpp_struct_name = tl2cpp::cpp_tl_struct_name("f_", tl2cpp::get_tl_function_name_of_php_class(klass));
    W << "return " << f_tl_cpp_struct_name << "::typed_store(this);" << NL;
    W << END << NL;
  }

  W << END << ";" << NL;
  W << CloseNamespace();

  compile_back_includes(W, std::move(front_includes));

  W << CloseFile();
}

template<class ReturnValueT>
void ClassDeclaration::compile_class_method(FunctionSignatureGenerator &&W, ClassPtr klass, vk::string_view method_signature, const ReturnValueT &return_value) {
  bool has_parent = ClassData::does_need_codegen(klass->get_parent_or_interface());
  bool has_derived = !klass->derived_classes.empty();
  bool is_overridden = has_parent && has_derived;
  bool is_final = has_parent && !has_derived;

  std::move(W)
    .set_is_virtual(has_derived)
    .set_final(is_final)
    .set_overridden(is_overridden)
    << method_signature <<
    BEGIN
    << "return " << return_value << ";" << NL <<
    END << NL << NL;
}

void ClassDeclaration::compile_inner_methods(CodeGenerator &W, ClassPtr klass) {
  compile_get_class(W, klass);
  compile_get_hash(W, klass);
  compile_accept_visitor_methods(W, klass);
  compile_serialization_methods(W, klass);
}

void ClassDeclaration::compile_get_class(CodeGenerator &W, ClassPtr klass) {
  auto class_name_as_raw_string = "R\"(" + klass->name + ")\"";
  compile_class_method(FunctionSignatureGenerator(W).set_const_this(), klass, "const char *get_class()", class_name_as_raw_string);
}

void ClassDeclaration::compile_get_hash(CodeGenerator &W, ClassPtr klass) {
  compile_class_method(FunctionSignatureGenerator(W).set_const_this(), klass, "int get_hash()", klass->get_hash());
}

void ClassDeclaration::compile_accept_visitor(CodeGenerator &W, ClassPtr klass, const char *visitor_type) {
  compile_class_method(FunctionSignatureGenerator(W), klass, fmt_format("void accept({} &visitor)", visitor_type), "generic_accept(visitor)");
}

void ClassDeclaration::compile_accept_visitor_methods(CodeGenerator &W, ClassPtr klass) {
  if (!klass->need_instance_to_array_visitor &&
      !klass->need_instance_cache_visitors &&
      !klass->need_instance_memory_estimate_visitor) {
    return;
  }

  W << NL;
  FunctionSignatureGenerator(W) << "template<class Visitor>" << NL
                                << "void generic_accept(Visitor &&visitor) " << BEGIN;
  for (auto cur_klass = klass; cur_klass; cur_klass = cur_klass->parent_class) {
    cur_klass->members.for_each([&W](const ClassMemberInstanceField &f) {
      // will generate visitor("field_name", $field_name);
      W << "visitor(\"" << f.local_name() << "\", $" << f.local_name() << ");" << NL;
    });
  }
  W << END << NL;

  if (klass->need_instance_to_array_visitor) {
    W << NL;
    compile_accept_visitor(W, klass, "InstanceToArrayVisitor");
  }

  if (klass->need_instance_memory_estimate_visitor) {
    W << NL;
    compile_accept_visitor(W, klass, "InstanceMemoryEstimateVisitor");
  }

  if (klass->need_instance_cache_visitors) {
    W << NL;
    compile_accept_visitor(W, klass, "DeepMoveFromScriptToCacheVisitor");
    compile_accept_visitor(W, klass, "DeepDestroyFromCacheVisitor");
    compile_accept_visitor(W, klass, "ShallowMoveFromCacheToScriptVisitor");
  }
}

void ClassDeclaration::compile_serialization_methods(CodeGenerator &W, ClassPtr klass) {
  if (!klass->is_serializable) {
    return;
  }

  compile_serialize(W, klass);
  compile_deserialize(W, klass);
}

void ClassDeclaration::compile_serialize(CodeGenerator &W, ClassPtr klass) {
  //template<typename Packer>
  //void msgpack_pack(Packer &packer) const {
  //   packer.pack(tag_1);
  //   packer.pack(field_1);
  //   ...
  //}
  std::string body;
  uint16_t cnt_fields = 0;

  klass->members.for_each([&](ClassMemberInstanceField &field) {
    if (field.var->serialization_tag != -1) {
      body += fmt_format("packer.pack({}); packer.pack(${});\n", field.var->serialization_tag, field.var->name);
      cnt_fields += 2;
    }
  });

  FunctionSignatureGenerator(W).set_const_this()
    << "void msgpack_pack(msgpack::packer<string_buffer> &packer)" << BEGIN
    << "packer.pack_array(" << cnt_fields << ");" << NL
    << body << NL
    << END << NL;
}

void ClassDeclaration::compile_deserialize(CodeGenerator &W, ClassPtr klass) {
  //if (msgpack_o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }
  //auto arr = msgpack_o.via.array;
  //for (size_t i = 0; i < arr.size; i += 2) {
  //  auto tag = arr.ptr[i].as<uint8_t>();
  //  auto elem = arr.ptr[i + 1];
  //  switch (tag) {
  //    case tag_x: elem.convert(x); break;
  //    case tag_s: elem.convert(s); break;
  //    default   : break;
  //  }
  //}
  //

  std::vector<std::string> cases;
  klass->members.for_each([&](ClassMemberInstanceField &field) {
    if (field.var->serialization_tag != -1) {
      cases.emplace_back(fmt_format("case {}: elem.convert(${}); break;", field.var->serialization_tag, field.var->name));
    }
  });

  cases.emplace_back("default: break;");

  W << "void msgpack_unpack(msgpack::object const &msgpack_o)" << BEGIN
      << "if (msgpack_o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }" << NL
      << "auto arr = msgpack_o.via.array;" << NL
      << "for (size_t i = 0; i < arr.size; i += 2)" << BEGIN
        << "auto tag = arr.ptr[i].as<uint8_t>();" << NL
        << "auto elem = arr.ptr[i + 1];" << NL
        << "(void) elem;" << NL
        << "switch (tag)" << BEGIN
          << JoinValues(cases, "", join_mode::multiple_lines) << NL
        << END << NL
      << END << NL
    << END << NL;
}

IncludesCollector ClassDeclaration::compile_front_includes(CodeGenerator &W) const {
  IncludesCollector includes;
  klass->members.for_each([&includes](const ClassMemberInstanceField &f) {
    includes.add_var_signature_forward_declarations(f.var);
  });

  includes.add_base_classes_include(klass);

  W << includes;

  if (tl2cpp::is_php_class_a_tl_function(klass)) {
    std::string tl_src_name = tl2cpp::get_tl_function_name_of_php_class(klass);  // 'net.pid', 'rpcPing'
    unsigned long pos = tl_src_name.find('.');
    W << Include("tl/" + (pos == std::string::npos ? "common" : tl_src_name.substr(0, pos)) + ".h");
  }

  return includes;
}

void ClassDeclaration::compile_back_includes(CodeGenerator &W, IncludesCollector &&front_includes) const {
  IncludesCollector includes{std::move(front_includes)};
  includes.start_next_block();

  klass->members.for_each([&includes](const ClassMemberInstanceField &f) {
    includes.add_var_signature_depends(f.var);
  });

  W << includes;
}

StaticLibraryRunGlobal::StaticLibraryRunGlobal(gen_out_style style) :
  style(style) {
}

void StaticLibraryRunGlobal::compile(CodeGenerator &W) const {
  switch (style) {
    case gen_out_style::tagger:
    case gen_out_style::cpp:
      FunctionSignatureGenerator(W) << "void f$" << LibData::run_global_function_name(G->get_global_namespace()) << "()";
      break;
    case gen_out_style::txt:
      W << "function " << LibData::run_global_function_name(G->get_global_namespace()) << "() ::: void";
      break;
  }
}

