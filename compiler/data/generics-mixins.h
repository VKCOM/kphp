// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>
#include <string>
#include <map>

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "common/wrappers/string_view.h"

// see the .cpp file for detailed comments of how do template functions work

// when we have `f<T1,T2>`, then f->generics_declaration is an instance of this class, itemsT = [ {nameT = T1}, {nameT = T2} ]
// syntactically, such functions are declared with @kphp-template
// later, when we have generics, classes would be able to be generics too, ClassData would also have such a field
struct GenericsDeclarationMixin {
  struct GenericsItem {
    std::string nameT;
    const TypeHint *extends_hint;
  };

  std::vector<GenericsItem> itemsT;

  bool empty() const { return itemsT.empty(); }
  size_t size() const { return itemsT.size(); }
  auto begin() const { return itemsT.begin(); }
  auto end() const { return itemsT.end(); }

  bool has_nameT(const std::string &nameT) const;
  void add_itemT(const std::string &nameT, const TypeHint *extends_hint);
  const TypeHint *find(const std::string &nameT) const;

  static void apply_from_phpdoc(FunctionPtr f, const PhpDocComment *phpdoc);
  static void make_function_generics_on_callable_arg(FunctionPtr f, VertexPtr func_param);
  static void make_function_generics_on_object_arg(FunctionPtr f, VertexPtr func_param);
};

// when we have `f<T1,T2>` and a call `f($o1,$o2)`, then it has call->instantiation_list set
// currently, T1 and T2 could be detected only by assumptions (instances), but the structure supports any type hints
struct GenericsInstantiationMixin {
  std::map<std::string, const TypeHint *> instantiations;   // {"T1": SomeClass, "T2": int[], ...}
  Location location;                                        // from where this instance of a generics function is instantiated

  void add_instantiationT(const std::string &nameT, const TypeHint *instantiation);
  const TypeHint *find(const std::string &nameT) const;

  // generates a name like "f$_${T1}${T2}"
  std::string generate_instantiated_func_name(FunctionPtr template_function) const;

  size_t size() const { return instantiations.size(); }
  bool empty() const { return instantiations.empty(); }
  auto begin() const { return instantiations.begin(); }
  auto end() const { return instantiations.end(); }
};
