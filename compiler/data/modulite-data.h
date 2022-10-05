// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <vector>
#include <string>
#include <map>

#include "common/wrappers/string_view.h"

#include "compiler/data/data_ptr.h"
#include "compiler/debug.h"

// in .modulite.yaml in we enumerate symbols, e.g.
// "require": ["@another_m"],
// "export": ["SomeClass", "A::someMethod()"]
// after parsing, every symbol is represented as this class
// depending on kind, a field of a union is set
struct ModuliteSymbol {
  enum Kind {
    kind_ref_stringname,    // @another_m or #composer/package until being resolved
    kind_modulite,          // @feed or @msg/channels
    kind_klass,             // SomeClass
    kind_function,          // someFunction() or A::someMethod()
    kind_constant,          // SOME_DEFINE or A::SOME_CONST
    kind_global_var,        // $global_var or A::$static_field
  } kind;

  int line;

  union {
    vk::string_view ref_stringname;
    ModulitePtr modulite;
    ClassPtr klass;
    FunctionPtr function;
    DefinePtr constant;
    vk::string_view global_var;
  };
};

// represents structure of .modulite.yaml
class ModuliteData {
  DEBUG_STRING_METHOD { return modulite_name; }

  ModulitePtr get_self_ptr() { return ModulitePtr(this); }

public:
  static ModulitePtr create_from_composer_json(ComposerJsonPtr composer_json, bool has_modulite_yaml_also);
  static ModulitePtr create_from_modulite_yaml(const std::string &yaml_filename, ModulitePtr parent);

  // full absolute path to .modulite.yaml, it's registered in G, it has ->dir, kphp_error can point to it
  SrcFilePtr yaml_file;

  // if it's a sub-modulite of a composer package (or a modulite created from composer.json itself)
  ComposerJsonPtr composer_json;

  // if it's a modulite created from composer.json (for instance, "export" is typically empty, which means all exported)
  bool is_composer_package;

  // "name" from yaml, starts with @, e.g. "@feed", "@messages", "@messages/channel"
  std::string modulite_name;
  // "namespace" from yaml, normalized: starts with symbol, ends with \, e.g. "VK\Messages\"; empty if no namespace
  std::string modulite_namespace;
  // for "@msg/channels", parent is "@msg"
  ModulitePtr parent;

  // "export" from yaml lists all exported symbols (classes, functions, submodulites, etc.)
  std::vector<ModuliteSymbol> exports;
  // denormalization: if its parent lists @this in "export" (so, it's visible always, without 'allow-internal' lookups)
  bool exported_from_parent{false};
  // submodulites from "export" (including exported sub-submodulites, etc.) are stored separately for optimization
  std::vector<ModulitePtr> submodulites_exported_at_any_depth;

  // "force-internal" from yaml lists methods/constants making them internal despite their classes are exported
  std::vector<ModuliteSymbol> force_internal;

  // "require" from yaml lists external symbols it depends on (other modulites, composer packages, globals, etc.)
  std::vector<ModuliteSymbol> require;

  // "allow-internal-access" from yaml lists additional "export" rules for specific usage contexts
  std::vector<std::pair<ModuliteSymbol, std::vector<ModuliteSymbol>>> allow_internal;


  void resolve_names_to_pointers();

  void validate_yaml_requires();
  void validate_yaml_exports();
  void validate_yaml_force_internal();

  ModulitePtr find_lca_with(ModulitePtr another_m);
  ModulitePtr get_modulite_corresponding_to_composer_json();
};

