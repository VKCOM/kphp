// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

#include "common/type_traits/list_of_types.h"

#include "compiler/code-gen/common.h"
#include "compiler/code-gen/gen-out-style.h"
#include "compiler/data/function-data.h"
#include "compiler/data/var-data.h"
#include "compiler/inferring/type-data.h"

struct LabelName {
  int label_id;

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
    std::string s = type_out(type, style);
    if (W.get_context().inside_macro) {
      while (s.find(',') != std::string::npos) {
        s = s.replace(s.find(','), 1, " COMMA ");   // tuples have it
      }
    }
    W << s;
  }
};

class FunctionSignatureGenerator {
public:
  explicit FunctionSignatureGenerator(CodeGenerator &W) noexcept
    : W_(W) {
  }
  
  FunctionSignatureGenerator(const FunctionSignatureGenerator &) = delete;
  FunctionSignatureGenerator& operator=(const FunctionSignatureGenerator &) = delete;

  template<class T> 
  FunctionSignatureGenerator &&operator<<(const T &value) && noexcept {
    if (is_empty_) {
      if (inline_) {
        W_ << "inline ";
      }
      if (!definition_ && virtual_) {
        W_ << "virtual ";
      }
      is_empty_ = false;
    }

    W_ << value;
    return std::move(*this);
  }

  FunctionSignatureGenerator &&operator<<(const OpenBlock &value) && noexcept {
    return generate_specifiers(value);
  }

  FunctionSignatureGenerator &&operator<<(const SemicolonAndNL &value) && noexcept {
    return generate_specifiers(value);
  }

  ~FunctionSignatureGenerator() {
    generate_specifiers("");
  }

  FunctionSignatureGenerator &&set_is_virtual(bool new_value = true) && noexcept {
    virtual_ = new_value;
    return std::move(*this);
  }

  FunctionSignatureGenerator &&set_const_this(bool new_value = true) && noexcept {
    const_this_ = new_value;
    return std::move(*this);
  }

  FunctionSignatureGenerator &&set_overridden(bool new_value = true) && noexcept {
    overridden_ = new_value;
    return std::move(*this);
  }

  FunctionSignatureGenerator &&set_final(bool new_value = true) && noexcept {
    final_ = new_value;
    return std::move(*this);
  }

  FunctionSignatureGenerator &&set_pure_virtual(bool new_value = true) && noexcept {
    pure_virtual_ = new_value;
    return std::move(*this);
  }

  FunctionSignatureGenerator &&set_inline(bool new_value = true) && noexcept {
    inline_ = new_value;
    return std::move(*this);
  }

  FunctionSignatureGenerator &&set_definition(bool new_value = true) && noexcept {
    definition_ = new_value;
    return std::move(*this);
  }

private:
  template<class T>
  FunctionSignatureGenerator &&generate_specifiers(const T &value) noexcept {
    if (!specifiers_generated_) {
      if (const_this_) {
        W_ << " const ";
      }

      if (noexcept_) {
        W_ << " noexcept ";
      }

      if (!definition_ && final_) {
        W_ << " final ";
      }

      if (!definition_ && overridden_) {
        W_ << " override ";
      }

      if (pure_virtual_) {
        W_ << " = 0";
      }

      specifiers_generated_ = true;
    }

    W_ << value;
    return std::move(*this);
  }

private:
  bool is_empty_ = true;
  bool specifiers_generated_ = false;

  CodeGenerator &W_;

  bool virtual_ = false;
  bool const_this_ = false;
  const bool noexcept_ = true;
  bool overridden_ = false;
  bool final_ = false;
  bool pure_virtual_ = false;
  bool inline_ = false;
  bool definition_ = false;
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
  vk::string_view name;

  VarName() = default;
  explicit VarName(VarPtr var) : var(var) {}
  explicit VarName(vk::string_view name) : name{name} {}

  bool empty() const {
    return !var && name.empty();
  }

  void compile(CodeGenerator &W) const {
    if (!name.empty()) {
      W << name;
      return;
    }

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
