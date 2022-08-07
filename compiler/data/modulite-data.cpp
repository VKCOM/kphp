// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/modulite-data.h"

#include <yaml-cpp/yaml.h>

#include "common/algorithms/contains.h"
#include "common/wrappers/likely.h"

#include "compiler/compiler-core.h"
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

class ModuliteYamlParser {
  ModulitePtr out;

  [[gnu::always_inline]] const std::string &as_string(const YAML::Node &y) {
    if (unlikely(!y.IsScalar() || y.Scalar().empty())) {
      fire_yaml_error(out, "expected non-empty string", y);
      return EMPTY_STRING_RETURNED_WHEN_GOT_NONSTRING_IN_YAML;
    }
    return y.Scalar();
  }

  static vk::string_view alloc_stringview(vk::string_view sv_on_local_mem) {
    const auto *alloc = new std::string(sv_on_local_mem.data(), sv_on_local_mem.size());
    return vk::string_view{alloc->c_str(), alloc->size()};
  }


  void parse_yaml_name(const YAML::Node &y_name) {
    const std::string &name = y_name.Scalar();
    if (name.empty() || name[0] != '@') {
      fire_yaml_error(out, "'name' should start with @", y_name);
    }

    std::string modulite_name = name;
    size_t last_slash = modulite_name.rfind('/');
    vk::string_view expected_parent_name = vk::string_view{modulite_name}.substr(0, last_slash);

    // valid:   @msg/channels/infra is placed in @msg/channels
    // invalid: @msg/channels/infra is placed in @msg, @feed is placed in @msg
    if (out->parent && out->parent->modulite_name != expected_parent_name) {
      fire_yaml_error(out, fmt_format("inconsistent nesting: {} placed in {}", modulite_name, out->parent->modulite_name), y_name);
    }
    // invalid: @msg/channels is placed outside
    if (!out->parent && last_slash != std::string::npos) {
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

  // this function exists, because g++ does not understand such statements inside parse_any_scalar_symbol():
  // return {.kind=ModuliteSymbol::kind_function, .line=line, .function=function};
  template<ModuliteSymbol::Kind kind, class T>
  [[gnu::always_inline]] ModuliteSymbol create_symbol(int line, T value) const {
    ModuliteSymbol s{.kind=kind, .line=line};
    if constexpr (kind == ModuliteSymbol::kind_ref_stringname) { s.ref_stringname = value; }
    if constexpr (kind == ModuliteSymbol::kind_klass) { s.klass = value; }
    if constexpr (kind == ModuliteSymbol::kind_function) { s.function = value; }
    if constexpr (kind == ModuliteSymbol::kind_constant) { s.constant = value; }
    if constexpr (kind == ModuliteSymbol::kind_global_var) { s.global_var = value; }
    return s;
  }

  // resolve any string to a symbol, example inputs:
  // "\\f()", "@msg/channels", "RelativeClass", "REL_CONST", "\\GLOBAL_CONST", "\\AbsClass::$static_field"
  ModuliteSymbol parse_any_scalar_symbol(const std::string &s, const YAML::Node &y_loc) const {
    int line = y_loc.Mark().line + 1;

    // @msg, @msg/channels
    if (s[0] == '@') {  // will be resolved later, store as string
      return create_symbol<ModuliteSymbol::kind_ref_stringname>(line, alloc_stringview(s));
    }
    // #composer-package
    if (s[0] == '#') {  // will be resolved later, store as string
      return create_symbol<ModuliteSymbol::kind_ref_stringname>(line, alloc_stringview(s));
    }

    size_t pos$$ = s.find("::");
    bool abs_name = s[0] == '\\';

    // if no :: then it doesn't belong to a class
    if (pos$$ == std::string::npos) {
      if (s[0] == '$') {
        // $global_var
        return create_symbol<ModuliteSymbol::kind_global_var>(line, alloc_stringview(s).substr(1));
      } else if (s[s.size() - 1] == ')') {
        // relative_func() or \\global_func()
        std::string f_fqn = abs_name ? s.substr(1, s.size() - 3) : out->modulite_namespace + s.substr(0, s.size() - 2);
        if (FunctionPtr function = G->get_function(replace_backslashes(f_fqn))) {
          return create_symbol<ModuliteSymbol::kind_function>(line, function);
        }
        fire_yaml_error(out, fmt_format("can't find function {}()", TermStringFormat::paint_bold(f_fqn)), line);
        return create_symbol<ModuliteSymbol::kind_ref_stringname>(line, "");
      } else {
        // RelativeClass or \\GlobalClass or RELATIVE_CONST or \\GLOBAL_CONST
        std::string fqn = abs_name ? s.substr(1) : out->modulite_namespace + s;
        if (ClassPtr klass = G->get_class(fqn)) {
          return create_symbol<ModuliteSymbol::kind_klass>(line, klass);
        }
        if (DefinePtr constant = G->get_define(fqn)) {
          return create_symbol<ModuliteSymbol::kind_constant>(line, constant);
        }
        fire_yaml_error(out, fmt_format("can't find class/constant {}", TermStringFormat::paint_bold(fqn)), line);
        return create_symbol<ModuliteSymbol::kind_ref_stringname>(line, "");
      }
      __builtin_unreachable();
    }

    // if :: exists, then it's RelativeClass::{something} or \\GlobalClass::{something}
    std::string c_fqn = abs_name ? s.substr(1, pos$$ - 1) : out->modulite_namespace + s.substr(0, pos$$);
    ClassPtr klass = G->get_class(c_fqn);
    if (!klass) {
      fire_yaml_error(out, fmt_format("can't find class {}", TermStringFormat::paint_bold(c_fqn)), line);
      return create_symbol<ModuliteSymbol::kind_ref_stringname>(line, "");
    }

    if (s[pos$$ + 2] == '$') {
      // C::$static_field
      std::string local_name = s.substr(pos$$ + 3);
      if (const ClassMemberStaticField *f = klass->members.get_static_field(local_name)) {
        return create_symbol<ModuliteSymbol::kind_global_var>(line, vk::string_view(f->var->name));
      }
      fire_yaml_error(out, fmt_format("can't find static field {}::${}", TermStringFormat::paint_bold(c_fqn), TermStringFormat::paint_bold(local_name)), line);
      return create_symbol<ModuliteSymbol::kind_ref_stringname>(line, "");
    } else if (s[s.size() - 1] == ')') {
      // C::static_method() or C::instance_method()
      std::string local_name = s.substr(pos$$ + 2, s.size() - pos$$ - 4);
      if (const ClassMemberStaticMethod *m = klass->members.get_static_method(local_name)) {
        return create_symbol<ModuliteSymbol::kind_function>(line, m->function);
      }
      if (const ClassMemberInstanceMethod *m = klass->members.get_instance_method(local_name)) {
        return create_symbol<ModuliteSymbol::kind_function>(line, m->function);
      }
      fire_yaml_error(out, fmt_format("can't find method {}::{}()", TermStringFormat::paint_bold(c_fqn), TermStringFormat::paint_bold(local_name)), line);
      return create_symbol<ModuliteSymbol::kind_ref_stringname>(line, "");
    } else {
      // C::CONST
      std::string local_name = s.substr(pos$$ + 2);
      if (const ClassMemberConstant *c = klass->members.get_constant(local_name)) {
        return create_symbol<ModuliteSymbol::kind_constant>(line, G->get_define(c->define_name));
      }
      fire_yaml_error(out, fmt_format("can't find constant {}::{}", TermStringFormat::paint_bold(c_fqn), TermStringFormat::paint_bold(local_name)), line);
      return create_symbol<ModuliteSymbol::kind_ref_stringname>(line, "");
    }

    __builtin_unreachable();
  }

  void parse_yaml_export(const YAML::Node &y_export) {
    for (const auto &y : y_export) {
      const std::string &y_name = as_string(y);
      out->exports.emplace_back(parse_any_scalar_symbol(y_name, y));
    }
  }

  void parse_yaml_force_internal_symbols(const YAML::Node &y_force_internal) {
    for (const auto &y : y_force_internal) {
      const std::string &y_name = as_string(y);
      out->force_internal.emplace_back(parse_any_scalar_symbol(y_name, y));
    }
  }

  void parse_yaml_require(const YAML::Node &y_require) {
    for (const auto &y : y_require) {
      const std::string &y_name = as_string(y);   // e.g. "@rpc" / "#flood-lib" / "\\id()" / "\\VK\\Post"
      ModuliteSymbol s = parse_any_scalar_symbol(y_name, y);
      out->require.emplace_back(s);
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
      parse_yaml_force_internal_symbols(y_force_internal);
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
};


// having SomeClass or someFunction(), detect a modulite this symbol belongs to
// pay attention, that we can't use `FunctionData::modulite` and similar,
// because that properties haven't been assigned up to the point of yaml validation
// (but dir->nested_files_modulite has already been set, propagated to child dirs also, that's what we rely on here)
// `FunctionData::modulite` and similar will be assigned later, to be used for checking func calls / class access / etc.
// See `CalcRealDefinesAndAssignModulitesF` sync pipe.
static ModulitePtr get_modulite_of_symbol(const ModuliteSymbol &s) {
  switch (s.kind) {
    case ModuliteSymbol::kind_ref_stringname:
      return ModulitePtr{};
    case ModuliteSymbol::kind_modulite:
      return s.modulite->parent;
    case ModuliteSymbol::kind_klass:
      return s.klass->file_id->dir->nested_files_modulite;
    case ModuliteSymbol::kind_function:
      return s.function->file_id->dir->nested_files_modulite;
    case ModuliteSymbol::kind_constant:
      return s.constant->file_id->dir->nested_files_modulite;
    case ModuliteSymbol::kind_global_var: {
      size_t pos$$ = s.global_var.find("$$");
      if (pos$$ == std::string::npos) {
        return ModulitePtr{};
      }
      ClassPtr klass = G->get_class(replace_characters(static_cast<std::string>(s.global_var.substr(0, pos$$)), '$', '\\'));
      return klass && klass->file_id ? klass->file_id->dir->nested_files_modulite : ModulitePtr{};
    }
    default:
      __builtin_unreachable();
  }
}

ModulitePtr ModuliteData::create_from_modulite_yaml(const std::string &yaml_filename, ModulitePtr parent) {
  ModulitePtr out = ModulitePtr(new ModuliteData());
  out->yaml_file = G->register_file(yaml_filename, LibPtr{});
  out->parent = parent;

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

  auto resolve_stringname = [inside_m](ModuliteSymbol &s) {
    if (s.kind == ModuliteSymbol::kind_ref_stringname && s.ref_stringname.starts_with("@")) {
      ModulitePtr ref = G->get_modulite(s.ref_stringname);
      if (!ref) {
        fire_yaml_error(inside_m, fmt_format("{} not found", TermStringFormat::paint_bold(static_cast<std::string>(s.ref_stringname))), s.line);
      }
      s.kind = ModuliteSymbol::kind_modulite;
      s.modulite = ref;

    } else if (s.kind == ModuliteSymbol::kind_ref_stringname && s.ref_stringname.starts_with("#")) {
      fire_yaml_error(inside_m, "composer packages are not supported yet", s.line);
    }
  };

  for (ModuliteSymbol &symbol : exports) {
    resolve_stringname(symbol);
  }
  for (ModuliteSymbol &symbol : force_internal) {
    resolve_stringname(symbol);
  }
  for (ModuliteSymbol &symbol : require) {
    resolve_stringname(symbol);
  }
  for (auto &p_allow_rule : allow_internal) {
    resolve_stringname(p_allow_rule.first);
    for (ModuliteSymbol &symbol : p_allow_rule.second) {
      resolve_stringname(symbol);
    }
  }

  // append this (inside_m) to submodulites_exported_at_any_depth all the tree up
  for (ModulitePtr child = inside_m; child->parent; child = child->parent) {
    if (!child->is_exported_from_parent()) {
      break;
    }
    child->parent->submodulites_exported_at_any_depth.emplace_back(inside_m);
  }
}

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

      ModulitePtr common_lca_parent = inside_m->find_lca_with(another_m);
      for (ModulitePtr child = another_m; child->parent != common_lca_parent; child = child->parent) {
        if (!child->is_exported_from_parent()) {
          // valid: even if @some/another is not exported, but mentioned in "allow-internal-access" for @msg in @some
          ModulitePtr cur_parent = child->parent;
          if (cur_parent) {
            auto it_this = std::find_if(cur_parent->allow_internal.begin(), cur_parent->allow_internal.end(),
                                        [inside_m](const auto &p) { return p.first.kind == ModuliteSymbol::kind_modulite && p.first.modulite == inside_m; });
            if (it_this != cur_parent->allow_internal.end()) {
              bool lists_cur = vk::any_of(it_this->second, [child](const ModuliteSymbol &s) { return s.kind == ModuliteSymbol::kind_modulite && s.modulite == child; });
              if (lists_cur) {
                continue;
              }
            }
          }
          fire_yaml_error(inside_m, fmt_format("can't require {}: {} is internal in {}", another_m->modulite_name, child->modulite_name, child->parent->modulite_name), r.line);
        }
      }

    } else {
      // @msg requires SomeClass / someFunction() / A::someMethod() / A::CONST / etc.
      // valid: it's in a global scope
      // invalid: it's in @some-modulite (@msg should require @some-modulite, not its symbols)
      if (ModulitePtr of_modulite = get_modulite_of_symbol(r)) {
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

    } else {
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

    bool is_allowed = (fi.kind == ModuliteSymbol::kind_constant && fi.constant->class_id)
                      || (fi.kind == ModuliteSymbol::kind_function && fi.function->class_id)
                      || (fi.kind == ModuliteSymbol::kind_global_var && fi.global_var.find("$$") != std::string::npos);
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

// @msg exports @msg/channels if it's listed in "export" directly
bool ModuliteData::is_exported_from_parent() {
  if (!parent) {
    return false;
  }

  ModulitePtr self = get_self_ptr();
  return vk::any_of(parent->exports, [self](const ModuliteSymbol &e) {
    return e.kind == ModuliteSymbol::kind_modulite && e.modulite == self;
  });
}

// for @msg/channels/infra and @msg/folders, lca is @msg
// for @msg and @feed, lca is null
ModulitePtr ModuliteData::find_lca_with(ModulitePtr another_m) {
  ModulitePtr lca = get_self_ptr();
  for (; lca; lca = lca->parent) {
    bool is_common = vk::string_view{another_m->modulite_name}.starts_with(lca->modulite_name) && another_m->modulite_name[lca->modulite_name.size()] == '/';
    if (is_common) {
      break;
    }
  }
  return lca;
}
