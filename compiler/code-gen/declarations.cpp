// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/code-gen/declarations.h"

#include "common/algorithms/compare.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/files/json-encoder-tags.h"
#include "compiler/code-gen/files/tl2cpp/tl2cpp-utils.h"
#include "compiler/code-gen/includes.h"
#include "compiler/code-gen/namespace.h"
#include "compiler/code-gen/naming.h"
#include "compiler/code-gen/vertex-compiler.h"
#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/ffi-data.h"
#include "compiler/data/kphp-json-tags.h"
#include "compiler/data/src-file.h"
#include "compiler/data/lib-data.h"
#include "compiler/data/var-data.h"
#include "compiler/gentree.h"
#include "compiler/inferring/public.h"
#include "compiler/inferring/type-data.h"
#include "compiler/tl-classes.h"

VarDeclaration VarExternDeclaration(VarPtr var) {
  return {var, true, false};
}

VarDeclaration VarPlainDeclaration(VarPtr var) {
  return {var, false, false};
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
  FunctionSignatureGenerator(W) << "int64_t " << FunctionForkName(function) <<
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
    W << (!type.type->is_primitive_type() ? "const &" : "");
  }
  W << VarName(var_ptr);
}

void FunctionParams::declare_txt_param(CodeGenerator &W, VertexAdaptor<op_var> var, const TypeName &type) const {
  W << type << " " << (var->ref_flag ? "&" : "") << "$" << var->var_id->name;
}

void FunctionParams::compile(CodeGenerator &W) const {
  bool first = true;
  size_t ii = shift;
  for (auto i : params) {
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

FFIDeclaration::FFIDeclaration(ClassPtr ffi_scope): ffi_scope{ffi_scope} {}

InterfaceDeclaration::InterfaceDeclaration(InterfacePtr interface) :
  interface(interface) {
}

TlDependentTypesUsings::TlDependentTypesUsings(vk::tlo_parsing::type *tl_type, const std::string &php_tl_class_name) : tl_type(tl_type) {
  int template_suf_start = php_tl_class_name.find("__");
  kphp_assert(template_suf_start != std::string::npos);
  specialization_suffix = php_tl_class_name.substr(template_suf_start);
  for (const auto &constructor : tl_type->constructors) {
    for (const auto &arg : constructor->args) {
      if (arg->is_optional()) {
        continue;
      }
      bool arg_wrapped_to_optional = false;
      if (arg->is_fields_mask_optional()) {
        if (arg->type_expr->as<vk::tlo_parsing::type_var>()) {
          // Can't deduce in cases like 'arg:fields_mask.1? t', where {t: Type}: don't know if 't' is wrapped to optional or not
          continue;
        }
        auto *arg_type_expr = arg->type_expr->as<vk::tlo_parsing::type_expr>();
        kphp_assert(arg_type_expr); // .fields_mask.0?[int] is prohibited in TL
        arg_wrapped_to_optional = tl2cpp::is_tl_type_wrapped_to_Optional(G->get_tl_classes().get_scheme()->get_type_by_magic(arg_type_expr->type_id));
      }
      cur_tl_constructor = constructor.get();
      cur_tl_arg = arg.get();
      std::vector<InnerParamTypeAccess> path_to_inner;
      if (arg_wrapped_to_optional) {
        path_to_inner.emplace_back(InnerParamTypeAccess(false, "InnerType"));
      }
      deduce_params_from_type_tree(arg->type_expr.get(), path_to_inner);
      if (arg_wrapped_to_optional) {
        path_to_inner.pop_back();
      }
      kphp_assert(path_to_inner.empty());
    }
  }
  if (!check_deduction_result()) {
    kphp_error(false,
               fmt_format("Couldn't deduce generic type-parameters for TL type '{}'.\n"
                             "Each type variable in this type must be used at least once "
                             "NOT like '... Maybe t ... ' and '<arg> :fields_mask.<n>? t'", tl_type->name));
  }
}

TlDependentTypesUsings::DeducingInfo::DeducingInfo(std::string deduced_type, std::vector<TlDependentTypesUsings::InnerParamTypeAccess> path) :
  deduced_type(std::move(deduced_type)),
  path_to_inner_param(std::move(path)) {}

// Traverse a tree while ignoring all vertices except vk::tlo_parsing::type_var and vk::tlo_parsing::type_expr.
// Collect a path to a type_var to get the type.
// An example of what we want to do:
// ********************** TL scheme **********************
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
// Assume that TL arrays won't have any type vars, i.e. this case is impossible:
//  test {t:Type} [x:t] = Test;
// All such cases (vector, tuple) are implemented manually in tl_builtins.h
void TlDependentTypesUsings::deduce_params_from_type_tree(vk::tlo_parsing::type_expr_base *type_tree, std::vector<InnerParamTypeAccess> &recursion_stack) {
  if (auto *type_var = dynamic_cast<vk::tlo_parsing::type_var *>(type_tree)) {
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
  if (auto *type_expr = dynamic_cast<vk::tlo_parsing::type_expr *>(type_tree)) {
    int i = 0;
    for (const auto &child : type_expr->children) {
      if (auto *casted_child = dynamic_cast<vk::tlo_parsing::type_expr_base *>(child.get())) {
        vk::tlo_parsing::type *parent_tl_type = G->get_tl_classes().get_scheme()->get_type_by_magic(type_expr->type_id);
        kphp_assert(parent_tl_type);

        InnerParamTypeAccess inner_access;
        bool skip_maybe = false;
        if (tl2cpp::is_tl_type_a_php_array(parent_tl_type)) {
          inner_access.drop_class_instance = false;
          inner_access.inner_type_name = "ValueType";
        } else if (parent_tl_type->name == "Maybe") {
          if (casted_child->as<vk::tlo_parsing::type_var>() || casted_child->as<vk::tlo_parsing::type_array>()) {
            // Can't deduce in cases like 'Maybe t', where {t: Type}: don't know if 't' is wrapped to optional or not
            continue;
          }
          auto *child_type_expr = dynamic_cast<vk::tlo_parsing::type_expr *>(casted_child);
          kphp_assert(child_type_expr);
          vk::tlo_parsing::type *child_tl_type = G->get_tl_classes().get_scheme()->get_type_by_magic(child_type_expr->type_id);
          kphp_assert(child_tl_type);
          if (tl2cpp::is_tl_type_wrapped_to_Optional(child_tl_type)) {
            inner_access.drop_class_instance = false;
            inner_access.inner_type_name = "InnerType";
          } else {
            skip_maybe = true;
          }
        } else {
          inner_access.drop_class_instance = true;
          // correctness of this is verified inside tl_scheme_final_check()
          inner_access.inner_type_name = parent_tl_type->constructors[0]->args[i]->name;
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

bool TlDependentTypesUsings::check_deduction_result() const {
  const auto &args = tl_type->constructors.front()->args;
  return std::all_of(args.begin(), args.end(), [&](const auto &arg) {
      return !arg->is_type() || deduced_params.count(arg->name);
  });
}

std::unique_ptr<TlDependentTypesUsings> InterfaceDeclaration::detect_if_needs_tl_usings() const {
  if (tl2cpp::is_php_class_a_tl_polymorphic_type(interface)) {
    vk::tlo_parsing::type *cur_tl_type = tl2cpp::get_tl_type_of_php_class(interface);

    bool needs_tl_usings = tl2cpp::is_type_dependent(cur_tl_type, G->get_tl_classes().get_scheme().get());
    if (needs_tl_usings) {
      return std::make_unique<TlDependentTypesUsings>(cur_tl_type, interface->name);
    }
  }
  return {};
}

void FFIDeclaration::compile(CodeGenerator &W) const {
  W << OpenFile(ffi_scope->header_name, ffi_scope->get_subdir());

  W << "#pragma once" << NL;

  W << ExternInclude("runtime/ffi.h");

  auto *scope = ffi_scope->ffi_scope_mixin;

  W << "extern \"C\" {" << NL;

  // struct definitions are emitted for both shared (dynamic) and static libraries
  for (const auto &type : scope->types) {
    W << (type->kind == FFITypeKind::StructDef ? "struct" : "union");
    W << " " << FFIRoot::c_name_mangle(scope->scope_name, type->str);
    // empty members list mean "forward declaration", don't emit {} for them
    if (!type->members.empty()) {
      W << " " << BEGIN;
      for (const FFIType *field : type->members) {
        W << ffi_mangled_type_string(scope->scope_name, field) << ";" << NL;
      }
      W << END;
    }
    W << ";" << NL;
  }

  // for shared libraries we don't emit function declarations as they're accessed
  // via FFIEnv object that contains dynamically loaded symbols inside
  if (!scope->is_shared_lib()) {
    for (const auto &sym : scope->variables) {
      W << "extern " << ffi_type_string(sym.type) << ";" << NL;
    }
    for (const auto &sym : scope->functions) {
      // note: static lib function symbols are unmangled
      W << ffi_decltype_string(sym.type->members[0]) << " " << sym.type->str << "(";
      for (int i = 1; i < sym.type->members.size(); i++) {
        W << ffi_type_string(sym.type->members[i]);
        if (i < sym.type->members.size() - 1) {
          W << ", ";
        }
      }
      if (sym.type->is_variadic()) {
        W << ", ...";
      }
      W << ");" << NL;
    }
  }

  W << "}" << NL;

  W << CloseFile();
}

void InterfaceDeclaration::compile(CodeGenerator &W) const {
  W << OpenFile(interface->header_name, interface->get_subdir());
  W << "#pragma once" << NL;

  IncludesCollector includes;

  if (!interface->implements.empty()) {
    includes.add_base_classes_include(interface);
  }
  if (!interface->json_encoders.empty()) {
    includes.add_raw_filename_include(JsonEncoderTags::all_tags_file_);
  }
  W << includes;

  W << OpenNamespace() << NL;

  std::unique_ptr<TlDependentTypesUsings> tl_dep_usings = detect_if_needs_tl_usings();
  if (tl_dep_usings) {
    tl_dep_usings->compile_dependencies(W);
  }

  W << "struct " << interface->src_name << " : public ";
  if (!interface->implements.empty()) {
    auto transform_to_src_name = [](CodeGenerator &W, const InterfacePtr &i) { W << i->src_name; };
    W << JoinValues(interface->implements, ", public ", join_mode::one_line, transform_to_src_name);
  } else {
    W << (interface->need_virtual_modifier() ? "virtual " : "") << "abstract_refcountable_php_interface";
  }
  W << " " << BEGIN;

  if (tl_dep_usings) {
    W << *tl_dep_usings << NL;
  }

  ClassDeclaration::compile_inner_methods(W, interface);
  // special hack to avoid many lines of asm code to initialize virtual base, it's not inlined due to -Os flag
  W << interface->src_name << "() __attribute__((always_inline)) = default;" << NL;
  W << "~" << interface->src_name << "() __attribute__((always_inline)) = default;" << NL;


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
    vk::tlo_parsing::combinator *cur_tl_constructor = tl2cpp::get_tl_constructor_of_php_class(klass);
    vk::tlo_parsing::type *cur_tl_type = scheme->get_type_by_magic(cur_tl_constructor->type_id);

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

  auto get_all_interfaces = [klass = this->klass] {
    auto transform_to_src_name = [](CodeGenerator &W, InterfacePtr i) { W << i->src_name.c_str(); };
    return JoinValues(klass->implements, ", ", join_mode::one_line, transform_to_src_name);
  };
  W << NL << "struct " << klass->src_name;
  // builtin classes that should not be extended must be marked with "final";
  // classes without "final" are expected to be a proper KPHP classes that can be extended
  bool parent_class_is_builtin = klass->parent_class &&
                                 klass->parent_class->is_builtin() &&
                                 klass->parent_class->is_class();
  if (parent_class_is_builtin || (klass->parent_class && klass->parent_class->does_need_codegen())) {
    W << (klass->derived_classes.empty() ? " final" : "") << " : public ";
    if (!klass->implements.empty()) {
      W << "refcountable_polymorphic_php_classes_virt<" << klass->parent_class->src_name << ", " << get_all_interfaces() << ">";
    } else {
      W << klass->parent_class->src_name;
    }
  } else if (klass->is_empty_class()) {
    W << " final : public refcountable_empty_php_classes";
  } else if (!klass->implements.empty()) {
    W << ": public refcountable_polymorphic_php_classes<" << get_all_interfaces() << ">";
  } else if (!klass->derived_classes.empty()) {
    if (klass->need_virtual_modifier()) {
      W << ": public refcountable_polymorphic_php_classes_virt<>";
    } else {
      W << ": public refcountable_polymorphic_php_classes<abstract_refcountable_php_interface>";
    }
  } else { // not polymorphic
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

  compile_inner_methods(W, klass);

  // generate the member method store() for RPC functions which will call the store function generated by the tl2cpp
  if (tl2cpp::is_php_class_a_tl_function(klass)) {
    W << NL;
    FunctionSignatureGenerator(W).set_final().set_const_this() << "std::unique_ptr<tl_func_base> store() " << BEGIN;
    std::string f_tl_cpp_struct_name = tl2cpp::cpp_tl_struct_name("f_", tl2cpp::get_tl_function_name_of_php_class(klass));
    W << "return " << f_tl_cpp_struct_name << "::typed_store(this);" << NL;
    W << END << NL;
  }

  compile_job_worker_shared_memory_piece_methods(W, true);

  W << END << ";" << NL;
  W << CloseNamespace();

  compile_back_includes(W, std::move(front_includes));

  W << OpenNamespace{};
  compile_job_worker_shared_memory_piece_methods(W);
  W << CloseNamespace{};

  W << CloseFile();
}

template<class ReturnValueT>
void ClassDeclaration::compile_class_method(FunctionSignatureGenerator &&W, ClassPtr klass, vk::string_view method_signature, const ReturnValueT &return_value) {
  const bool has_parent = (klass->parent_class && klass->parent_class->does_need_codegen()) ||
    vk::any_of(klass->implements, [](InterfacePtr i) { return i->does_need_codegen() || i->is_builtin(); });
  const bool has_derived = !klass->derived_classes.empty();
  const bool is_overridden = has_parent && has_derived;
  const bool is_final = has_parent && !has_derived;
  const bool is_pure_virtual = klass->class_type == ClassType::interface;

  FunctionSignatureGenerator &&signature = std::move(W)
    .set_is_virtual(is_pure_virtual || has_derived)
    .set_final(is_final)
    .set_overridden(is_overridden)
    .set_pure_virtual(is_pure_virtual)
    << method_signature;

  if (is_pure_virtual) {
    std::move(signature) << SemicolonAndNL{} << NL;
  } else {
    std::move(signature) << BEGIN << "return " << return_value << SemicolonAndNL{} << END << NL << NL;
  }
}

void ClassDeclaration::compile_inner_methods(CodeGenerator &W, ClassPtr klass) {
  compile_json_flatten_flag(W, klass);
  compile_has_wakeup_flag(W, klass);
  compile_get_class(W, klass);
  compile_get_hash(W, klass);
  compile_accept_visitor_methods(W, klass);
  compile_msgpack_serialize(W, klass);
  compile_msgpack_deserialize(W, klass);
  compile_virtual_builtin_functions(W, klass);
  compile_wakeup(W, klass);
}

void ClassDeclaration::compile_json_flatten_flag(CodeGenerator &W, ClassPtr klass) {
  const auto *tag_flatten = klass->kphp_json_tags
                            ? klass->kphp_json_tags->find_tag([](const kphp_json::KphpJsonTag &tag) { return tag.attr_type == kphp_json::json_attr_flatten && tag.flatten; })
                            : nullptr;
  if (tag_flatten) {
    W << "constexpr static bool json_flatten_class{true};" << NL << NL;
  }
}

void ClassDeclaration::compile_has_wakeup_flag(CodeGenerator &W, ClassPtr klass) {
  bool has_wakeup = klass->members.has_instance_method("__wakeup");
  if (has_wakeup) {
    W << "constexpr static bool has_wakeup_method{true};" << NL << NL;
  }
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

void ClassDeclaration::compile_generic_accept(CodeGenerator &W, ClassPtr klass) {
  FunctionSignatureGenerator(W) << "template<class Visitor>" << NL
                                << "void generic_accept(Visitor &&visitor) " << BEGIN;
  for (auto cur_klass = klass; cur_klass; cur_klass = cur_klass->parent_class) {
    cur_klass->members.for_each([&W](const ClassMemberInstanceField &f) {
      // will generate visitor("field_name", $field_name);
      W << "visitor(\"" << f.local_name() << "\", $" << f.local_name() << ");" << NL;
    });
  }
  W << END << NL;
}

// generate `visitor("json_key", $field_name);`, wrapped with `if ($field_name != default)` if skip_if_default, etc.
static void compile_json_visitor_call(CodeGenerator &W, ClassPtr json_encoder, ClassPtr klass, const ClassMemberInstanceField &field, bool to_encode) noexcept {
  auto props = kphp_json::merge_and_inherit_json_tags(field, klass, json_encoder);

  if (to_encode ? props.skip_when_encoding : props.skip_when_decoding) {
    return;
  }

  if (to_encode && props.skip_if_default) {
    W << "if (";
    if (field.var->init_val) {
      W << "!eq2($" << field.local_name() << ", " << field.var->init_val << ")";
    } else {
      W << "f$boolval($" << field.local_name() << ")";
    }
    W << ") " << BEGIN;
  }
  if (to_encode && props.float_precision) {
    W << "visitor.set_precision(" << props.float_precision << ");" << NL;
  }
  if (props.raw_string) {
    W << "JsonRawString raw_string$" << field.local_name() << "{$" << field.local_name() << "};" << NL;
  }

  W << "visitor(\"" << props.json_key << "\", " << (props.raw_string ? "raw_string$" : "$") << field.local_name();
  if ((to_encode && props.array_as_hashmap) || (!to_encode && props.required)) {
    W << ", true";
  }

  W << ");" << NL;
  if (to_encode && props.float_precision) {
    W << "visitor.restore_precision();" << NL;
  }
  if (to_encode && props.skip_if_default) {
    W << END << NL;
  }
}

void ClassDeclaration::compile_accept_json_visitor(CodeGenerator &W, ClassPtr klass, bool to_encode, ClassPtr json_encoder) {
  bool parent_has_method = false;
  if (ClassPtr parent = klass->parent_class) {
    parent_has_method |= parent->json_encoders.end() != std::find(parent->json_encoders.begin(), parent->json_encoders.end(), std::pair{json_encoder, to_encode});
  }
  for (InterfacePtr parent : klass->implements) {
    parent_has_method |= parent->json_encoders.end() != std::find(parent->json_encoders.begin(), parent->json_encoders.end(), std::pair{json_encoder, to_encode});
  }

  const bool has_derived = !klass->derived_classes.empty();
  const bool is_pure_virtual = klass->is_interface();

  FunctionSignatureGenerator &&signature = FunctionSignatureGenerator(W)
    .set_is_virtual(is_pure_virtual || has_derived)
    .set_final(parent_has_method && !has_derived)
    .set_overridden(parent_has_method && has_derived)
    .set_pure_virtual(is_pure_virtual)
    << fmt_format("void accept({}<{}> &visitor)", to_encode ? "ToJsonVisitor" : "FromJsonVisitor", JsonEncoderTags::get_cppStructTag_name(json_encoder->name));

  if (is_pure_virtual) {
    std::move(signature) << SemicolonAndNL{};
    return;
  } else {
    std::move(signature) << BEGIN;
  }

  // generates `visitor("json_key", $field_name)` calls in appropriate order
  auto compile_visitor_calls_for_class_fields = [](CodeGenerator &W, ClassPtr klass, ClassPtr json_encoder, bool to_encode) {
    // if @kphp-json 'fields' exists above a class, we use it to output fields in that order
    if (klass->kphp_json_tags) {
      const kphp_json::KphpJsonTag *tag_fields = klass->kphp_json_tags->find_tag([json_encoder](const kphp_json::KphpJsonTag &tag) {
        return tag.attr_type == kphp_json::json_attr_fields && (!tag.for_encoder || tag.for_encoder == json_encoder);
      });
      if (tag_fields) {
        for (vk::string_view field_name : split_skipping_delimeters(tag_fields->fields, ",")) {
          compile_json_visitor_call(W, json_encoder, klass, *klass->members.get_instance_field(field_name), to_encode);
        }
        return;
      }
    }
    // otherwise, we output fields in an order they are declared
    klass->members.for_each([&W, json_encoder, klass, to_encode](const ClassMemberInstanceField &field) {
      compile_json_visitor_call(W, json_encoder, klass, field, to_encode);
    });
  };

  if (!klass->parent_class) {
    compile_visitor_calls_for_class_fields(W, klass, json_encoder, to_encode);
  } else {
    // in final json we want parent fields to appear first
    std::forward_list<ClassPtr> parents;
    for (auto cur_klass = klass; cur_klass; cur_klass = cur_klass->parent_class) {
      parents.emplace_front(cur_klass);
    }
    for (auto parent : parents) {
      compile_visitor_calls_for_class_fields(W, parent, json_encoder, to_encode);
    }
  }

  W << END << NL;
}

void ClassDeclaration::compile_accept_visitor_methods(CodeGenerator &W, ClassPtr klass) {
  bool need_generic_accept =
    klass->need_to_array_debug_visitor ||
    klass->need_instance_cache_visitors ||
    klass->need_instance_memory_estimate_visitor;

  if (!need_generic_accept && klass->json_encoders.empty()) {
    return;
  }

  if (need_generic_accept) {
    W << NL;
    compile_generic_accept(W, klass);
  }

  if (klass->need_to_array_debug_visitor) {
    W << NL;
    compile_accept_visitor(W, klass, "ToArrayVisitor");
  }

  if (klass->need_instance_memory_estimate_visitor) {
    W << NL;
    compile_accept_visitor(W, klass, "InstanceMemoryEstimateVisitor");
  }

  if (klass->need_instance_cache_visitors) {
    W << NL;
    compile_accept_visitor(W, klass, "InstanceReferencesCountingVisitor");
    compile_accept_visitor(W, klass, "InstanceDeepCopyVisitor");
    compile_accept_visitor(W, klass, "InstanceDeepDestroyVisitor");
  }

  for (auto[encoder, to_encode] : klass->json_encoders) {
    W << NL;
    compile_accept_json_visitor(W, klass, to_encode, encoder);
  }
}

void ClassDeclaration::compile_msgpack_serialize(CodeGenerator &W, ClassPtr klass) {
  if (!klass->is_serializable) {
    return;
  }

  //template<typename Packer>
  //void msgpack_pack(Packer &packer) const {
  //   packer.pack(tag_1);
  //   packer.pack(field_1);
  //   ...
  //}
  std::string body;
  uint16_t cnt_fields = 0;

  klass->members.for_each([&](ClassMemberInstanceField &field) {
    if (field.serialization_tag != -1) {
      auto func_name = fmt_format("vk::msgpack::packer_float32_decorator::pack_value{}", field.serialize_as_float32 ? "_float32" : "");
      body += fmt_format("packer.pack({}); {}(packer, ${});\n", field.serialization_tag, func_name, field.var->name);
      cnt_fields += 2;
    }
  });

  FunctionSignatureGenerator(W).set_const_this()
    << "void msgpack_pack(vk::msgpack::packer<string_buffer> &packer)" << BEGIN
    << "packer.pack_array(" << cnt_fields << ");" << NL
    << body << NL
    << END << NL;
}

void ClassDeclaration::compile_msgpack_deserialize(CodeGenerator &W, ClassPtr klass) {
  if (!klass->is_serializable) {
    return;
  }

  //if (msgpack_o.type != vk::msgpack::stored_type::ARRAY) { throw vk::msgpack::type_error{}; }
  //auto arr = msgpack_o.via.array;
  //for (size_t i = 0; i < arr.size; i += 2) {
  //  auto tag = arr.ptr[i].as<uint8_t>();
  //  [[maybe_unused]] auto elem = arr.ptr[i + 1];
  //  switch (tag) {
  //    case tag_x: elem.convert(x); break;
  //    case tag_s: elem.convert(s); break;
  //    default   : break;
  //  }
  //}
  //

  std::vector<std::string> cases;
  klass->members.for_each([&](ClassMemberInstanceField &field) {
    if (field.serialization_tag != -1) {
      cases.emplace_back(fmt_format("case {}: elem.convert(${}); break;", field.serialization_tag, field.var->name));
    }
  });

  cases.emplace_back("default: break;");

  W << "void msgpack_unpack(const vk::msgpack::object &msgpack_o)" << BEGIN
      << "if (msgpack_o.type != vk::msgpack::stored_type::ARRAY) { throw vk::msgpack::type_error{}; }" << NL
      << "auto arr = msgpack_o.via.array;" << NL
      << "for (size_t i = 0; i < arr.size; i += 2)" << BEGIN
        << "auto tag = arr.ptr[i].as<uint8_t>();" << NL
        << "[[maybe_unused]] auto elem = arr.ptr[i + 1];" << NL
        << "switch (tag)" << BEGIN
          << JoinValues(cases, "", join_mode::multiple_lines) << NL
        << END << NL
      << END << NL
    << END << NL;
}

void ClassDeclaration::compile_virtual_builtin_functions(CodeGenerator &W, ClassPtr klass) {
  if (!klass->need_virtual_builtin_functions) {
    return;
  }

  compile_class_method(FunctionSignatureGenerator(W).set_const_this(), klass,
                       "size_t virtual_builtin_sizeof()", "sizeof(*this)");

  compile_class_method(FunctionSignatureGenerator(W).set_const_this(), klass,
                       klass->src_name + "* virtual_builtin_clone()", "new " + klass->src_name + "{*this}");
}

void ClassDeclaration::compile_wakeup(CodeGenerator &W, ClassPtr klass) {
  const auto *m_wakeup = klass->members.get_instance_method("__wakeup");
  if (!m_wakeup || klass->is_interface()) {
    return;
  }
  FunctionPtr f_wakeup = m_wakeup->function;

  // wakeup() method would look like this:
  //    void wakeup(class_instance<C$A> const &v$this) noexcept {
  //      void f$A$$__wakeup(class_instance<C$A> const &v$this) noexcept ;
  //      f$A$$__wakeup(v$this);
  //    }
  // (we insert a prototype into function body intentionally, to avoid cross includes)
  // even on inheritance, prototypes differ in children, that's why it's not virtual
  FunctionSignatureGenerator(W) << fmt_format("void wakeup(class_instance<{}> const &v$this)", klass->src_name) << BEGIN;
  W << FunctionDeclaration(f_wakeup, true) << ";" << NL;
  W << "f$" << f_wakeup->name << "(v$this);" << NL;
  W << END << NL;
}

IncludesCollector ClassDeclaration::compile_front_includes(CodeGenerator &W) const {
  IncludesCollector includes;
  klass->members.for_each([&includes](const ClassMemberInstanceField &f) {
    includes.add_var_signature_forward_declarations(f.var);
  });

  includes.add_base_classes_include(klass);
  if (!klass->json_encoders.empty()) {
    includes.add_raw_filename_include(JsonEncoderTags::all_tags_file_);
  }

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

void ClassDeclaration::compile_job_worker_shared_memory_piece_methods(CodeGenerator &W, bool compile_declaration_only) const {
  auto request_interface = G->get_class("KphpJobWorkerRequest");
  if (!request_interface) {   // when functions.txt deleted while development
    return;
  }

  kphp_assert(request_interface);
  if (!request_interface->is_parent_of(klass)) {
    return;
  }
  if (compile_declaration_only) {
    FunctionSignatureGenerator(W).set_overridden().set_const_this() << "class_instance<C$KphpJobWorkerSharedMemoryPiece> get_shared_memory_piece()" << SemicolonAndNL{};
    FunctionSignatureGenerator(W).set_overridden() << "void set_shared_memory_piece(const class_instance<C$KphpJobWorkerSharedMemoryPiece> &instance)" << SemicolonAndNL{};
    return;
  }
  const ClassMemberInstanceField *field = nullptr;
  if (klass->has_job_shared_memory_piece) {
    field = klass->get_job_shared_memory_pieces().front();
  }
  W << NL;
  FunctionSignatureGenerator(W).set_const_this().set_inline()
    << "class_instance<C$KphpJobWorkerSharedMemoryPiece> " << klass->src_name << "::get_shared_memory_piece() " << BEGIN;
  W << "return " << (field ? field->get_hash_name() : "{}") << SemicolonAndNL{};
  W << END << NL << NL;

  FunctionSignatureGenerator(W).set_inline() << "void " << klass->src_name
                                                     << "::set_shared_memory_piece(const class_instance<C$KphpJobWorkerSharedMemoryPiece> &instance) " << BEGIN;
  if (field) {
    W << "auto casted = instance.cast_to<" << field->get_inferred_type()->class_type()->src_name << ">();" << NL;
    W << "php_assert(instance.is_null() || !casted.is_null());" << NL;
    W << field->get_hash_name() << " = std::move(casted);" << NL;
  } else {
    W << "(void)instance;" << NL;
  }
  W << END << NL;
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

