// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/modulite-data.h"

#include <yaml-cpp/yaml.h>

#include "common/algorithms/contains.h"
#include "common/wrappers/likely.h"

#include "compiler/compiler-core.h"
#include "compiler/data/composer-json-data.h"
#include "compiler/data/define-data.h"
#include "compiler/data/function-data.h"
#include "compiler/data/src-dir.h"
#include "compiler/data/src-file.h"
#include "compiler/data/var-data.h"

/*
 * Modulite is a conception of isolating parts of code.
 * PHP, as a language, allows calling any class/function from any other place in the code.
 * For huge PHP projects, it's bad practice, because if you develop some kernel db connectors and public API,
 * you expect your users to use this API, but you have no guarantee that nobody calls your internals directly,
 * even if you expose a Composer package.
 * PHP has no "modules" in a language — and that's what Modulite would take care about.
 *
 * The concept of colored functions is more flexible, but Modulite is more generalized.
 * It's designed in a way to be integrated into IDE.
 *
 * A directory can now contain .modulite.yaml file, which specifies exports, requires, and other.
 * ModuliteData is a representation of this file.
 *
 * For more info about Modulite, its files, structure, and use cases, read a dedicated landing page.
 *
 * Note, that this file is only about parsing and validating .modulite.yaml.
 * Checking for rules is implemented in a separate file, see `modulite-check-rules.cpp`.
 *
 * IMPORTANT! keep this file and logic very close to ModuliteData in modulite-phpstan
 */


static const std::string EMPTY_STRING_RETURNED_WHEN_GOT_NONSTRING_IN_YAML;


[[gnu::cold]] static void fire_yaml_error(ModulitePtr inside_m, const std::string &reason, int line) {
  inside_m->yaml_file->load();  // load a file from disk, so that error message in console outputs a line

  stage::set_file(inside_m->yaml_file);
  stage::set_line(line);
  kphp_error(0, fmt_format("Failed loading {}:\n{}", inside_m->yaml_file->relative_file_name, reason));
}

[[gnu::cold]] static void fire_yaml_error(ModulitePtr inside_m, const std::string &reason, const YAML::Node &y_node) {
  fire_yaml_error(inside_m, reason, y_node.Mark().line + 1);
}

[[gnu::noinline]] static std::string paint_bold(vk::string_view s) {
  return TermStringFormat::paint_bold(static_cast<std::string>(s));
}

// IMPORTANT! keep this class and logic very close to ModuliteYamlParser in modulite-phpstan
class ModuliteYamlParser {
  ModulitePtr out;

  [[gnu::always_inline]] const std::string &as_string(const YAML::Node &y) {
    if (unlikely(!y.IsScalar() || y.Scalar().empty())) {
      fire_yaml_error(out, "expected non-empty string", y);
      return EMPTY_STRING_RETURNED_WHEN_GOT_NONSTRING_IN_YAML;
    }
    return y.Scalar();
  }

  void parse_yaml_name(const YAML::Node &y_name) {
    const std::string &name = y_name.Scalar();
    if (name.empty() || name[0] != '@') {
      if (name == "<composer_root>") {
        return;
      }
      fire_yaml_error(out, "'name' should start with @", y_name);
    }

    std::string modulite_name = out->composer_json ? prepend_composer_package_to_name(out->composer_json, name) : name;
    size_t last_slash = modulite_name.rfind('/');
    vk::string_view expected_parent_name = vk::string_view{modulite_name}.substr(0, last_slash);

    // valid:   @msg/channels/infra is placed in @msg/channels
    // invalid: @msg/channels/infra is placed in @msg, @feed is placed in @msg
    if (!out->is_composer_package && out->parent && out->parent->modulite_name != expected_parent_name) {
      fire_yaml_error(out, fmt_format("inconsistent nesting: {} placed in {}", modulite_name, out->parent->modulite_name), y_name);
    }
    // invalid: @msg/channels is placed outside
    if (!out->is_composer_package && !out->parent && last_slash != std::string::npos) {
      fire_yaml_error(out, fmt_format("inconsistent nesting: {} placed outside of {}", modulite_name, expected_parent_name), -1);
    }

    out->modulite_name = std::move(modulite_name);
  }

  void parse_yaml_namespace(const YAML::Node &y_namespace) {
    const std::string &ns = y_namespace.Scalar();
    if (ns.empty() || ns == "\\") {
      return;
    }

    // "\\Some\\Ns" => "Some\\Ns\\"
    out->modulite_namespace = ns;
    if (out->modulite_namespace[0] == '\\') {
      out->modulite_namespace = out->modulite_namespace.substr(1);
    }
    if (out->modulite_namespace[out->modulite_namespace.size() - 1] != '\\') {
      out->modulite_namespace += '\\';
    }
  }

  // parse any symbol as a string: "\\f()", "@msg/channels", "RelativeClass", etc.
  // at the moment of parsing, they are stored as strings, and later resolved to symbols (see resolve_names_to_pointers())
  ModuliteSymbol parse_any_scalar_symbol(const std::string &s, const YAML::Node &y_loc) const {
    int line = y_loc.Mark().line + 1;
    return ModuliteSymbol{.kind=ModuliteSymbol::kind_ref_stringname, .line=line, .ref_stringname=string_view_dup(s)};
  }

  void parse_yaml_export(const YAML::Node &y_export) {
    for (const auto &y : y_export) {
      const std::string &y_name = as_string(y);
      out->exports.emplace_back(parse_any_scalar_symbol(y_name, y));
    }
  }

  void parse_yaml_force_internal(const YAML::Node &y_force_internal) {
    for (const auto &y : y_force_internal) {
      const std::string &y_name = as_string(y);
      out->force_internal.emplace_back(parse_any_scalar_symbol(y_name, y));
    }
  }

  void parse_yaml_require(const YAML::Node &y_require) {
    for (const auto &y : y_require) {
      const std::string &y_name = as_string(y);
      out->require.emplace_back(parse_any_scalar_symbol(y_name, y));
    }
  }

  void parse_yaml_allow_internal(const YAML::Node &y_allow_internal) {
    for (const auto &y : y_allow_internal) {
      const std::string &y_rule = as_string(y.first);
      if (!y.second.IsSequence()) {
        fire_yaml_error(out, "invalid format, expected a map of sequences", y.first);
        continue;
      }
      std::vector<ModuliteSymbol> e_vector;
      e_vector.reserve(y.second.size());
      for (const auto &y_rule_val : y.second) {
        e_vector.emplace_back(parse_any_scalar_symbol(as_string(y_rule_val), y_rule_val));
      }
      out->allow_internal.emplace_back(parse_any_scalar_symbol(y_rule, y.first), std::move(e_vector));
    }
  }

public:
  explicit ModuliteYamlParser(ModulitePtr out) : out(std::move(out)) {}

  void parse_modulite_yaml_file(const YAML::Node &y_file) {
    const auto &y_name = y_file["name"];
    if (y_name && y_name.IsScalar()) {
      parse_yaml_name(y_name);
    } else {
      fire_yaml_error(out, "'name' not specified", -1);
    }

    const auto &y_namespace = y_file["namespace"];
    if (y_namespace && y_namespace.IsScalar()) {
      parse_yaml_namespace(y_namespace);
    } else {
      fire_yaml_error(out, "'namespace' not specified", -1);
    }

    const auto &y_export = y_file["export"];
    if (y_export && y_export.IsSequence()) {
      parse_yaml_export(y_export);
    } else if (!y_export) {
      fire_yaml_error(out, "'export' not specified", 0);
    } else if (!y_export.IsNull()) {
      fire_yaml_error(out, "'export' has incorrect format", y_export);
    }

    const auto &y_force_internal = y_file["force-internal"];
    if (y_force_internal && y_force_internal.IsSequence()) {
      parse_yaml_force_internal(y_force_internal);
    } else if (y_force_internal && !y_force_internal.IsNull()) {
      fire_yaml_error(out, "'force-internal' has incorrect format", y_force_internal);
    }

    const auto &y_require = y_file["require"];
    if (y_require && y_require.IsSequence()) {
      parse_yaml_require(y_require);
    } else if (!y_require) {
      fire_yaml_error(out, "'require' not specified", 0);
    } else if (!y_require.IsNull()) {
      fire_yaml_error(out, "'require' has incorrect format", y_require);
    }

    const auto &y_allow_internal = y_file["allow-internal-access"];
    if (y_allow_internal && y_allow_internal.IsMap()) {
      parse_yaml_allow_internal(y_allow_internal);
    } else if (y_allow_internal && !y_allow_internal.IsNull()) {
      fire_yaml_error(out, "'allow-internal-access' has incorrect format", y_allow_internal);
    }
  }

  // when a PHP developer writes a Composer package and uses modulites in it,
  // modulites named absolutely, like @utils or @flood-details — like in any other project
  // but when a monolith uses that package placed in vendor/, all its modulites are prefixed:
  // @flood/details in #vk/common is represented as "#vk/common/@flood/details" on monolith compilation
  static std::string prepend_composer_package_to_name(ComposerJsonPtr composer_json, const std::string &modulite_name) {
    return "#" + composer_json->package_name + "/" + modulite_name;
  }
};


// having some_file.php, detect a modulite this file belongs to
// up to this point, all dirs have been traversed and propagated to child dirs also
static inline ModulitePtr get_modulite_of_file(const SrcFilePtr &file_id) {
  return file_id->dir->nested_files_modulite;
}

// having SomeClass or someFunction(), detect a modulite this symbol belongs to
// pay attention, that we can't use `FunctionData::modulite` and similar,
// because that properties haven't been assigned up to the point of yaml validation
// `FunctionData::modulite` and similar will be assigned later, to be used for checking func calls / class access / etc.
// See `CalcRealDefinesAndAssignModulitesF` sync pipe.
static ModulitePtr get_modulite_of_symbol(const ModuliteSymbol &s) {
  switch (s.kind) {
    case ModuliteSymbol::kind_ref_stringname:
      return ModulitePtr{};
    case ModuliteSymbol::kind_modulite:
      return s.modulite->parent;
    case ModuliteSymbol::kind_klass:
    case ModuliteSymbol::kind_constant:
    case ModuliteSymbol::kind_property:
      return get_modulite_of_file(s.klass->file_id);
    case ModuliteSymbol::kind_function:
      return get_modulite_of_file(s.klass ? s.klass->file_id : s.function->file_id);
    case ModuliteSymbol::kind_global_const:
      return get_modulite_of_file(s.constant->file_id);
    case ModuliteSymbol::kind_global_var:
      return ModulitePtr{};
    default:
      __builtin_unreachable();
  }
}

// parse composer.json and emit ModulitePtr from it
// composer packages are implicit modulites named "#"+json->name
// if a folder also contains .modulite.yaml, it's also parsed and can manually declare "export"
ModulitePtr ModuliteData::create_from_composer_json(ComposerJsonPtr composer_json, bool has_modulite_yaml_also) {
  ModulitePtr out = ModulitePtr(new ModuliteData());
  out->yaml_file = composer_json->json_file;
  out->modulite_name = "#" + composer_json->package_name;
  out->composer_json = composer_json;
  out->is_composer_package = true;

  // json->require (dependent packages, e.g. "vk/utils") are copied to modulite->require, e.g. "#vk/utils"
  for (const ComposerJsonData::RequireItem &c_require : composer_json->require) {
    out->require.emplace_back(ModuliteSymbol{.kind=ModuliteSymbol::kind_ref_stringname, .line=-1, .ref_stringname=string_view_dup("#" + c_require.package_name)});
  }

  if (has_modulite_yaml_also) {
    std::string yaml_filename = composer_json->json_file->dir->get_modulite_yaml_filename();
    out->yaml_file = G->register_file(yaml_filename, LibPtr{});
    try {
      YAML::Node y_file = YAML::LoadFile(out->yaml_file->file_name);
      ModuliteYamlParser parser(out);
      parser.parse_modulite_yaml_file(y_file);
    } catch (const YAML::Exception &ex) {
      fire_yaml_error(out, ex.msg, ex.mark.line + 1);
      return ModulitePtr{};
    }
  }

  return out;
}

// parse modulite.yaml and emit ModulitePtr from it
ModulitePtr ModuliteData::create_from_modulite_yaml(const std::string &yaml_filename, ModulitePtr parent) {
  ModulitePtr out = ModulitePtr(new ModuliteData());
  out->yaml_file = G->register_file(yaml_filename, LibPtr{});
  out->parent = parent;
  out->composer_json = parent ? parent->composer_json : ComposerJsonPtr{};
  out->is_composer_package = false;

  try {
    YAML::Node y_file = YAML::LoadFile(out->yaml_file->file_name);
    ModuliteYamlParser parser(out);
    parser.parse_modulite_yaml_file(y_file);
    return out;
  } catch (const YAML::Exception &ex) {
    fire_yaml_error(out, ex.msg, ex.mark.line + 1);
    return ModulitePtr{};
  }
}

// converts "@another_m" and similar to real pointers in-place,
// called when all modulites and composer packages are parsed and registered
void ModuliteData::resolve_names_to_pointers() {
  ModulitePtr inside_m = get_self_ptr();

  for (ModuliteSymbol &symbol : exports) {
    resolve_symbol_from_yaml(symbol);
  }
  for (ModuliteSymbol &symbol : force_internal) {
    resolve_symbol_from_yaml(symbol);
  }
  for (ModuliteSymbol &symbol : require) {
    resolve_symbol_from_yaml(symbol);
  }
  for (auto &p_allow_rule : allow_internal) {
    resolve_symbol_from_yaml(p_allow_rule.first);
    for (ModuliteSymbol &symbol : p_allow_rule.second) {
      resolve_symbol_from_yaml(symbol);
    }
  }

  inside_m->exported_from_parent = parent && vk::any_of(parent->exports, [inside_m](const ModuliteSymbol &e) {
    return e.kind == ModuliteSymbol::kind_modulite && e.modulite == inside_m;
  });

  // if a composer package is written using modulites, then all its child modulites are exported and available from the monolith
  // (unless .modulite.yaml near composer.json explicitly declares "export")
  if (parent && parent->is_composer_package && parent->exports.empty()) {
    inside_m->exported_from_parent = true;
  }

  // append this (inside_m) to submodulites_exported_at_any_depth all the tree up
  for (ModulitePtr child = inside_m; child->parent; child = child->parent) {
    if (!child->exported_from_parent) {
      break;
    }
    child->parent->submodulites_exported_at_any_depth.emplace_back(inside_m);
  }
}

// resolve any config item ("SomeClass", "SomeClass::f()", "@msg", etc.) from a string to a symbol
// this is done in-place, modifying s.kind and s.{union_prop}
// on error resolving, it's also printed here, pointing to a concrete line in yaml file
void ModuliteData::resolve_symbol_from_yaml(ModuliteSymbol &s) {
  kphp_assert(s.kind == ModuliteSymbol::kind_ref_stringname);
  ModulitePtr inside_m = get_self_ptr();
  vk::string_view v = s.ref_stringname;

  // @msg, @msg/channels
  if (v[0] == '@') {
    std::string name = static_cast<std::string>(v);
    if (composer_json) {
      name = ModuliteYamlParser::prepend_composer_package_to_name(composer_json, name);
    }
    if (ModulitePtr m_ref = G->get_modulite(name)) {
      s.kind = ModuliteSymbol::kind_modulite;
      s.modulite = m_ref;
    } else {
      fire_yaml_error(inside_m, fmt_format("modulite {} not found", paint_bold(v)), s.line);
    }
    return;
  }
  
  // #composer-package
  if (v[0] == '#') {
    if (ModulitePtr m_ref = G->get_modulite(v)) {
      s.kind = ModuliteSymbol::kind_modulite;
      s.modulite = m_ref;
    } else {
      // in case a ref is not found, don't fire an error,
      // because composer.json often contain "php" and other strange non-installed deps in "require"
    }
    return;
  }

  size_t pos$$ = v.find("::");
  bool abs_name = v[0] == '\\';

  // if no :: then it doesn't belong to a class
  if (pos$$ == std::string::npos) {
    // $global_var
    if (v[0] == '$') {
      s.kind = ModuliteSymbol::kind_global_var;
      s.global_var = v.substr(1);
      return;
    }
    // relative_func() or \\global_func()
    if (v[v.size() - 1] == ')') {
      std::string f_fqn = abs_name ? static_cast<std::string>(v.substr(1, v.size() - 3)) : modulite_namespace + v.substr(0, v.size() - 2);
      if (FunctionPtr function = G->get_function(replace_backslashes(f_fqn))) {
        s.kind = ModuliteSymbol::kind_function;
        s.function = function;
      } else {
        fire_yaml_error(inside_m, fmt_format("can't find function {}()", paint_bold(f_fqn)), s.line);
      }
      return;
    }
    // RelativeClass or \\GlobalClass or RELATIVE_CONST or \\GLOBAL_CONST
    std::string fqn = abs_name ? static_cast<std::string>(v.substr(1)) : modulite_namespace + v;
    if (ClassPtr klass = G->get_class(fqn)) {
      s.kind = ModuliteSymbol::kind_klass;
      s.klass = klass;
    } else if (DefinePtr constant = G->get_define(fqn)) {
      s.kind = ModuliteSymbol::kind_global_const;
      s.constant = constant;
    } else {
      fire_yaml_error(inside_m, fmt_format("can't find class/constant {}", paint_bold(fqn)), s.line);
    }

  } else {
    // if :: exists, then it's RelativeClass::{something} or \\GlobalClass::{something}
    std::string c_fqn = abs_name ? static_cast<std::string>(v.substr(1, pos$$ - 1)) : modulite_namespace + v.substr(0, pos$$);
    ClassPtr klass = G->get_class(c_fqn);
    if (!klass) {
      fire_yaml_error(inside_m, fmt_format("can't find class {}", paint_bold(c_fqn)), s.line);
      return;
    }

    // C::$static_field
    if (v[pos$$ + 2] == '$') {
      vk::string_view local_name = v.substr(pos$$ + 3);
      if (const ClassMemberStaticField *f = klass->get_static_field(local_name)) {
        s.kind = ModuliteSymbol::kind_property;
        s.klass = klass;
        s.property = f->var;
      } else {
        fire_yaml_error(inside_m, fmt_format("can't find class field {}::${}", paint_bold(c_fqn), paint_bold(local_name)), s.line);
      }
      return;
    }
    // C::static_method() or C::instance_method()
    if (v[v.size() - 1] == ')') {
      vk::string_view local_name = v.substr(pos$$ + 2, v.size() - pos$$ - 4);
      if (const ClassMemberStaticMethod *m = klass->members.get_static_method(local_name)) {
        s.kind = ModuliteSymbol::kind_function;
        s.klass = klass;
        s.function = m->function;
      } else if (const ClassMemberInstanceMethod *m = klass->get_instance_method(local_name)) {
        s.kind = ModuliteSymbol::kind_function;
        s.klass = klass;
        s.function = m->function;
      } else {
        fire_yaml_error(inside_m, fmt_format("can't find method {}::{}()", paint_bold(c_fqn), paint_bold(local_name)), s.line);
      }
      return;
    }
    // C::CONST
    vk::string_view local_name = v.substr(pos$$ + 2);
    if (const ClassMemberConstant *c = klass->get_constant(local_name)) {
      s.kind = ModuliteSymbol::kind_constant;
      s.klass = klass;
      s.constant = G->get_define(c->define_name);
    } else {
      fire_yaml_error(inside_m, fmt_format("can't find constant {}::{}", paint_bold(c_fqn), paint_bold(local_name)), s.line);
    }
  }
};

void ModuliteData::validate_yaml_requires() {
  ModulitePtr inside_m = get_self_ptr();

  for (const ModuliteSymbol &r : require) {
    if (r.kind == ModuliteSymbol::kind_modulite) {
      // @msg requires @feed or @some/another
      // valid: @feed has no parent or @some/another is exported from @some
      // invalid: it's not exported (same with longer chains, e.g. @p/c1/c2/c3, but @p/c1/c2 not exported from @p/c1)
      // valid: @api requires @api/internal, @msg/channels/infra requires @msg/internal (because they are scoped by lca)
      ModulitePtr another_m = r.modulite;
      if (inside_m == another_m) {
        fire_yaml_error(inside_m, fmt_format("a modulite lists itself in 'require': {}", inside_m->modulite_name), r.line);
      }

      ModulitePtr common_lca = inside_m->find_lca_with(another_m);
      for (ModulitePtr child = another_m; child != common_lca && child->parent != common_lca; child = child->parent) {
        if (ModulitePtr cur_parent = child->parent; cur_parent && !child->exported_from_parent) {
          // valid: even if @some/another is not exported, but mentioned in "allow-internal-access" for @msg in @some
          auto it_this = std::find_if(cur_parent->allow_internal.begin(), cur_parent->allow_internal.end(),
                                      [inside_m](const auto &p) { return p.first.kind == ModuliteSymbol::kind_modulite && p.first.modulite == inside_m; });
          if (it_this != cur_parent->allow_internal.end()) {
            bool lists_cur = vk::any_of(it_this->second, [child](const ModuliteSymbol &s) { return s.kind == ModuliteSymbol::kind_modulite && s.modulite == child; });
            if (lists_cur) {
              continue;
            }
          }
          fire_yaml_error(inside_m, fmt_format("can't require {}: {} is internal in {}", another_m->modulite_name, child->modulite_name, child->parent->modulite_name), r.line);
        }
      }

    } else {
      // @msg requires SomeClass / someFunction() / A::someMethod() / A::CONST / etc.
      // valid: it's in a global scope
      // invalid: it's in @some-modulite (@msg should require @some-modulite, not its symbols)
      ModulitePtr of_modulite = get_modulite_of_symbol(r);
      bool is_global_scope = !of_modulite || (of_modulite->is_composer_package && of_modulite->composer_json == inside_m->composer_json);
      if (!is_global_scope) {
        fire_yaml_error(inside_m, fmt_format("'require' contains a member of {}; you should require {} directly, not its members", of_modulite->modulite_name, of_modulite->modulite_name), r.line);
      }
    }
  }
}

void ModuliteData::validate_yaml_exports() {
  ModulitePtr inside_m = get_self_ptr();

  for (const ModuliteSymbol &e : exports) {
    if (e.kind == ModuliteSymbol::kind_modulite) {
      // @msg exports @another_m
      // valid: @msg exports @msg/channels
      // invalid: @msg exports @feed, @msg exports @msg/channels/infra
      ModulitePtr another_m = e.modulite;

      if (inside_m == another_m) {
        fire_yaml_error(inside_m, fmt_format("'export' of {} lists itself", inside_m->modulite_name), e.line);
      }
      if (another_m->parent != inside_m) {
        fire_yaml_error(inside_m, fmt_format("'export' of {} lists a non-child {}", inside_m->modulite_name, another_m->modulite_name), e.line);
      }

    } else if (e.kind != ModuliteSymbol::kind_ref_stringname) {
      // @msg exports SomeClass / someFunction() / A::someMethod() / A::CONST / etc.
      // valid: it belongs to @msg
      // invalid: it's in a global scope, or belongs to @feed, or @msg/channels, or etc.
      if (inside_m != get_modulite_of_symbol(e)) {
        fire_yaml_error(inside_m, fmt_format("'export' contains a symbol that does not belong to {}", inside_m->modulite_name), e.line);
      }
    }
  }
}

void ModuliteData::validate_yaml_force_internal() {
  ModulitePtr inside_m = get_self_ptr();

  for (const ModuliteSymbol &fi : force_internal) {
    // if @msg force internals A::someMethod() or A::CONST,
    // it can't be also declared in exports
    for (const ModuliteSymbol &e : inside_m->exports) {
      if (e.kind == fi.kind && e.function == fi.function) {
        fire_yaml_error(inside_m, fmt_format("'force-internal' contains a symbol which is exported"), fi.line);
      }
    }

    bool is_allowed = (fi.kind == ModuliteSymbol::kind_constant && fi.klass)
                      || (fi.kind == ModuliteSymbol::kind_function && fi.klass)
                      || (fi.kind == ModuliteSymbol::kind_property && fi.klass);
    if (!is_allowed) {
      // @msg force internals @msg/channels or #vk/common or SomeClass
      fire_yaml_error(inside_m, "'force-internal' can contain only class members", fi.line);
    } else {
      // @msg force internals A::someMethod() / A::CONST / etc.
      // valid: it belongs to @msg
      // invalid: it's in a global scope, or belongs to @feed, or @msg/channels, or etc.
      if (inside_m != get_modulite_of_symbol(fi)) {
        fire_yaml_error(inside_m, fmt_format("'force-internal' contains a symbol that does not belong to {}", inside_m->modulite_name), fi.line);
      }
    }
  }
}

// for @msg/channels/infra and @msg/folders, lca is @msg
// for @msg/channels and @msg, lca is @msg
// for @msg and @feed, lca is null
ModulitePtr ModuliteData::find_lca_with(ModulitePtr another_m) {
  ModulitePtr lca = get_self_ptr();
  for (; lca && lca != another_m; lca = lca->parent) {
    bool is_common = vk::string_view{another_m->modulite_name}.starts_with(lca->modulite_name) && another_m->modulite_name[lca->modulite_name.size()] == '/';
    if (is_common) {
      break;
    }
  }
  return lca;
}
