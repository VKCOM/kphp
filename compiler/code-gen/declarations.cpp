#include "compiler/code-gen/declarations.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/files/tl2cpp.h"
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
    W << (extern_flag ? "extern " : "") <<
      "decltype(const_begin(" << VarName(var) << "))" << " " << VarName(var) << "$it;" << NL;
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
    case gen_out_style::cpp:
      W << ret_type_gen << " " << FunctionName(function) << "(" << params_gen << ")";
      break;
    case gen_out_style::txt:
      W << "function " << function->name << "(" << params_gen << ") ::: " << ret_type_gen;
      break;
  }
}

FunctionForkDeclaration::FunctionForkDeclaration(FunctionPtr function, bool in_header) :
  function(function),
  in_header(in_header) {
}

void FunctionForkDeclaration::compile(CodeGenerator &W) const {
  W << "int " << FunctionForkName(function) <<
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
  if (var->ref_flag) {
    W << "&";
  }
  auto var_ptr = var->var_id;
  if (var_ptr->marked_as_const) {
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

  W << "struct " << interface->src_name << " : public " << parent_class << " " << BEGIN;

  ClassDeclaration::compile_get_class(W, interface);

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


void ClassDeclaration::compile(CodeGenerator &W) const {
  W << OpenFile(klass->header_name, klass->get_subdir());
  W << "#pragma once" << NL;

  W << OpenNamespace()
    << "struct " << klass->src_name << ";" << NL
    << CloseNamespace();

  compile_includes(W);

  W << OpenNamespace();

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
  if (ClassPtr base = klass->parent_class) {
    auto final_keyword = klass->derived_classes.empty() ? " final" : "";
    W << final_keyword << " : public " << base->src_name;
  } else if (klass->is_not_empty_class()) {
    W << " : public refcountable_php_classes<" << klass->src_name;
    if (interface) {
      W << ", " << interface->src_name;
    }
    W << "> ";
  } else {
    W << " final : public refcountable_empty_php_classes ";
  }
  W << BEGIN;

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

  if (klass->need_generate_accept_method()) {
    compile_get_class(W, klass);

    W << NL;
    W << "template<class Visitor>" << NL
      << "static void accept(Visitor &&visitor) " << BEGIN;

    for (auto cur_klass = klass; cur_klass; cur_klass = cur_klass->parent_class) {
      cur_klass->members.for_each([&](const ClassMemberInstanceField &f) {
        // will generate visitor.template operator()<int>("field_name", &ClassName::field_name);
        W << "visitor.template operator()<" << TypeName(tinf::get_type(f.var)) << ">(\"" << f.local_name() << "\", &" << klass->src_name << "::$" << f.local_name() << ");" << NL;
      });
    }
    W << END << NL;
  }

  // для rpc-функций генерим метод-член класса store(), который перевызывает сторилку из codegen tl2cpp
  if (tl_gen::is_php_class_a_tl_function(klass)) {
    W << NL;
    W << "unique_object<tl_func_base> store() const final " << BEGIN;
    std::string f_tl_cpp_struct_name = tl_gen::cpp_tl_struct_name("f_", tl_gen::get_tl_function_of_php_class(klass));
    W << "return " << f_tl_cpp_struct_name << "::typed_store(this);" << NL;
    W << END << NL;
  }

  W << END << ";" << NL;
  W << CloseNamespace();
  W << CloseFile();
}

void ClassDeclaration::compile_get_class(CodeGenerator &W, ClassPtr klass) {
  if (!klass->derived_classes.empty()) {
    W << "virtual ";
  }
  const char * final_kw = (!klass->implements.empty() || klass->parent_class) && klass->derived_classes.empty() ? "final" : "";
  W << "const char *get_class() const " << final_kw << BEGIN;
  {
    W << "return ";
    compile_string_raw(klass->name, W);
    W << ";" << NL;
  }
  W << END << NL;
}

void ClassDeclaration::compile_includes(CodeGenerator &W) const {
  IncludesCollector includes;
  klass->members.for_each([&includes](const ClassMemberInstanceField &f) {
    includes.add_var_signature_depends(f.var);
  });

  includes.add_base_classes_include(klass);

  W << includes;

  if (tl_gen::is_php_class_a_tl_function(klass)) {
    std::string tl_src_name = tl_gen::get_tl_function_of_php_class(klass);  // 'net.pid', 'rpcPing'
    tl_src_name = tl_src_name.substr(0, tl_src_name.find('.'));  // 'net', ''
    W << Include("tl/" + (tl_src_name.empty() ? "common" : tl_src_name) + ".h");
  }
}

ClassForwardDeclaration::ClassForwardDeclaration(ClassPtr klass) :
  klass(klass) {
}

void ClassForwardDeclaration::compile(CodeGenerator &W) const {
  W << "struct " << klass->src_name << ";" << NL;
}

StaticLibraryRunGlobal::StaticLibraryRunGlobal(gen_out_style style) :
  style(style) {
}

void StaticLibraryRunGlobal::compile(CodeGenerator &W) const {
  switch (style) {
    case gen_out_style::cpp:
      W << "void f$" << LibData::run_global_function_name(G->get_global_namespace()) << "()";
      break;
    case gen_out_style::txt:
      W << "function " << LibData::run_global_function_name(G->get_global_namespace()) << "() ::: void";
      break;
  }
}
