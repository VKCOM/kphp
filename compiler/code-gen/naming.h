#pragma once

#include "compiler/code-gen/gen-out-style.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/type-data.h"

struct LabelName {
  int label_id;
  explicit LabelName(int label_id) : label_id(label_id) {}

  void compile(CodeGenerator &W) const {
    W << "label" << label_id;
  }
};

struct MacroBegin {
  void compile(CodeGenerator &W) const {
    W.get_context().inside_macro++;
    W << "(";
  }
};

struct MacroEnd {
  void compile(CodeGenerator &W) const {
    W.get_context().inside_macro--;
    W << ")";
  }
};

struct TypeName {
  const TypeData *type;
  gen_out_style style;
  explicit TypeName(const TypeData *type, gen_out_style style = gen_out_style::cpp) :
    type(type),
    style(style) {
  }

  void compile(CodeGenerator &W) const {
    string s = type_out(type, style == gen_out_style::cpp);
    if (W.get_context().inside_macro) {
      while (s.find(',') != string::npos) {
        s = s.replace(s.find(','), 1, " COMMA ");   // такое есть у tuple'ов
      }
    }
    W << s;
  }
};

struct FunctionName {
  FunctionPtr function;
  explicit FunctionName(FunctionPtr function) :
    function(function) {
  }

  void compile(CodeGenerator &W) const {
    W << "f$" << function->name;
  }
};

struct FunctionForkName {
  FunctionPtr function;
  inline FunctionForkName(FunctionPtr function) : function(function) {}

  void compile(CodeGenerator &W) const {
    W << "f$fork$" << function->name;
  }
};

struct FunctionClassName {
  FunctionPtr function;
  explicit FunctionClassName(FunctionPtr function) :
    function(function) {
  }

  void compile(CodeGenerator &W) const {
    W << "c$" << function->name;
  }
};

struct VarName {
  VarPtr var;
  explicit VarName(VarPtr var) : var(var) {}

  void compile(CodeGenerator &W) const {
    if (var->is_function_static_var()) {
      W << FunctionName(var->holder_func) << "$";
    }

    W << "v$" << var->name;
  }
};

struct GlobalVarsResetFuncName {
  explicit GlobalVarsResetFuncName(FunctionPtr main_func, int part = -1) :
    main_func_(main_func),
    part_(part) {}

  void compile(CodeGenerator &W) const {
    W << FunctionName(main_func_) << "$global_vars_reset";
    if (part_ >= 0) {
      W << std::to_string(part_);
    }
    W << "()";
  }

private:
  const FunctionPtr main_func_;
  const int part_{-1};
};
