// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>
#include <string>
#include <vector>

#include "common/wrappers/string_view.h"
#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/debug.h"

// see the .cpp file for detailed comments of how do generics work

// when we have `f<T1,T2>`, then f->genericTs is an instance of this class, itemsT = [ {nameT = T1}, {nameT = T2} ]
// syntactically, such functions are declared with @kphp-generic, parsed in gentree
class GenericsDeclarationMixin {
  DEBUG_STRING_METHOD {
    return as_human_readable();
  }

  GenericsDeclarationMixin() = default;

public:
  struct GenericsItem {
    std::string nameT;
    const TypeHint* extends_hint; // in @kphp-generic, it goes after ':', e.g. T1:SomeInterface
    const TypeHint* def_hint;     // in @kphp-generic, it goes after '=', e.g. T2=Err
    bool is_variadic{false};      // in @kphp-generic, it's declared as ...T, not T
  };

  std::vector<GenericsItem> itemsT;

  std::string as_human_readable() const;

  bool empty() const {
    return itemsT.empty();
  }
  size_t size() const {
    return itemsT.size();
  }
  auto begin() const {
    return itemsT.begin();
  }
  auto end() const {
    return itemsT.end();
  }
  bool is_variadic() const {
    return !itemsT.empty() && itemsT.back().is_variadic;
  }

  bool has_nameT(const std::string& nameT) const;
  void add_itemT(const std::string& nameT, const TypeHint* extends_hint, const TypeHint* def_hint, bool is_variadic = false);
  const TypeHint* get_extends_hint(const std::string& nameT) const;

  std::string prompt_provide_commentTs_human_readable(VertexPtr call) const;

  static void check_declarationT_extends_hint(const TypeHint* extends_hint, const std::string& nameT);
  static void check_declarationT_def_hint(const TypeHint* def_hint, const std::string& nameT);
  static bool is_new_kphp_generic_syntax(const PhpDocComment* phpdoc);
  static void make_function_generic_on_callable_arg(FunctionPtr f, VertexPtr func_param);
  static void make_function_generic_on_object_arg(FunctionPtr f, VertexPtr func_param);
  static GenericsDeclarationMixin* create_for_function_empty(FunctionPtr f);
  static GenericsDeclarationMixin* create_for_function_from_phpdoc(FunctionPtr f, const PhpDocComment* phpdoc);
  static GenericsDeclarationMixin* create_for_function_cloning_from_variadic(FunctionPtr generic_f, int n_variadic);
};

// we have a special syntax in PHP to explicitly provide generic types inside a PHP comment: `f/*<T1, T2>*/(...)`
// at the moment of parsing (in gentree), we store only vector of T until it's matched with real `f`
class GenericsInstantiationPhpComment {
  DEBUG_STRING_METHOD {
    return as_human_readable();
  }

public:
  std::vector<const TypeHint*> vectorTs;

  explicit GenericsInstantiationPhpComment(std::vector<const TypeHint*> vectorTs)
      : vectorTs(std::move(vectorTs)) {}

  std::string as_human_readable() const;

  size_t size() const {
    return vectorTs.size();
  }
};

// when we have `f<T1,T2>` and a call `f($o1,$o2)`, then it has call->reifiedTs set
class GenericsInstantiationMixin {
  DEBUG_STRING_METHOD {
    return as_human_readable();
  }

public:
  GenericsInstantiationPhpComment* commentTs{nullptr};   // non-null if created in gentree from /*<...>*/ syntax
  std::map<std::string, const TypeHint*> instantiations; // {"T1": SomeClass, "T2": int[], ...}
  Location location;                                     // from where this instance of a generic function is instantiated

  explicit GenericsInstantiationMixin(Location location)
      : location(std::move(location)) {}
  GenericsInstantiationMixin(const GenericsInstantiationMixin& rhs);

  std::string as_human_readable() const;

  void provideT(const std::string& nameT, const TypeHint* instantiationT, VertexPtr call);
  const TypeHint* find(const std::string& nameT) const;

  // generates a name like "f$_${T1}$_${T2}"
  std::string generate_instantiated_name(const std::string& orig_name, const GenericsDeclarationMixin* genericTs) const;

  size_t size() const {
    return instantiations.size();
  }
  bool empty() const {
    return instantiations.empty();
  }
  auto begin() const {
    return instantiations.begin();
  }
  auto end() const {
    return instantiations.end();
  }
};
