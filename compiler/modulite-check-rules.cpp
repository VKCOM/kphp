// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/modulite-check-rules.h"

#include "common/algorithms/contains.h"

#include "compiler/compiler-core.h"
#include "compiler/data/define-data.h"
#include "compiler/data/function-data.h"

/*
 * Modulite is a conception of isolating parts of code. Read `modulite-data.cpp` for a brief description.
 *
 * This file is devoted to checking func calls / class access / etc.
 * Say, @feed declares `class PostInfo` as internal, and you call `new PostInfo` from `logQuery()` in @rpc.
 * Then `modulite_check_when_call_function(logQuery, PostInfo::__construct)` would fire an error,
 * unless there is a special exception in @feed that allows accessing an internal PostInfo from that concrete place.
 *
 * All these functions are called after `FunctionData::modulite`, `ClassData::modulite`, `DefineData::modulite` are set.
 * (they are assigned in `CalcRealDefinesAndAssignModulitesF` sync pipe).
 * Hence, we use that property for detecting what modulite a function/class belongs to.
 *
 * On any error breaking exports/requires, `kphp_error()` is triggered.
 * Note, that checks for calls/globals/consts are performed in different stages of pipeline,
 * that's why if for example there would be any error in using constants, no checks for calls will be performed,
 * since the compilation will stop before activating another pipe.
 *
 * This file is for checking rules, after all yaml files have been loaded.
 * Parsing and validation of .modulite.yaml is implemented in another file, see `modulite-data.cpp`.
 *
 * IMPORTANT! keep this file and logic very close to ModuliteCheckRules in modulite-phpstan
 */


static bool does_allow_internal_rule_satisfy_usage_context(FunctionPtr usage_context, const ModuliteSymbol &rule) {
  const FunctionData *outer = usage_context->get_this_or_topmost_if_lambda();
  return (rule.kind == ModuliteSymbol::kind_function && rule.function.operator->() == outer) ||
         (rule.kind == ModuliteSymbol::kind_klass && rule.klass == outer->class_id) ||
         (rule.kind == ModuliteSymbol::kind_modulite && rule.modulite == outer->modulite);
}

static inline bool should_this_usage_context_be_ignored(FunctionPtr usage_context) {
  // case 1.
  // @common: class Err { act() { echo static::CONST; } }
  // @feed:   class FeedErr extends \Common\Err { }
  // then Err::act (static=FeedErr) becomes { echo FeedErr::CONST; }, whereas FeedErr::act()->file_id = @common
  // due to f->modulite, @common calls @feed, but it does not actually
  // solution: analyze only Err::act(), but skip FeedErr::act()
  if (usage_context && usage_context->modifiers.is_static() &&
      usage_context->class_id != usage_context->context_class) {
    return true; // NOLINT(readability-simplify-boolean-expr)
  }
  return false;
}

// class A is exported from a modulite when
// - "A" is declared in 'export'
// - or "A" is declared in 'allow internal' for current usage context
static bool is_class_exported_from(ClassPtr klass, ModulitePtr owner, FunctionPtr usage_context) {
  for (const ModuliteSymbol &e : owner->exports) {
    if (e.kind == ModuliteSymbol::kind_klass && e.klass == klass) {
      return true;
    }
  }

  for (const auto &p : owner->allow_internal) {
    if (does_allow_internal_rule_satisfy_usage_context(usage_context, p.first)) {
      for (const ModuliteSymbol &e : p.second) {
        if (e.kind == ModuliteSymbol::kind_klass && e.klass == klass) {
          return true;
        }
      }
    }
  }

  if (owner->is_composer_package) {   // when it's an implicit modulite created from composer.json, all is exported
    return owner->exports.empty();    // (unless .modulite.yaml exists near composer.json and manually provides "export")
  }

  return false;
}

// function A::f is exported from a modulite when
// - "A::f" is declared in 'export'
// - or "A" is declared in 'export' AND "A::f" is not declared in 'force-internal'
// - or either "A" or "A::f" is declared in 'allow internal' for current usage context
static bool is_function_exported_from(FunctionPtr function, ModulitePtr owner, FunctionPtr usage_context) {
  for (const ModuliteSymbol &e : owner->exports) {
    if (e.kind == ModuliteSymbol::kind_function && e.function == function) {
      return true;
    }
    if (e.kind == ModuliteSymbol::kind_klass && e.klass == function->class_id) {
      for (const ModuliteSymbol &fi : owner->force_internal) {
        if (fi.kind == ModuliteSymbol::kind_function && fi.function == function) {
          goto search_allow_internal;
        }
      }
      return true;
    }
  }

  search_allow_internal:
  for (const auto &p : owner->allow_internal) {
    if (does_allow_internal_rule_satisfy_usage_context(usage_context, p.first)) {
      for (const ModuliteSymbol &e : p.second) {
        if (e.kind == ModuliteSymbol::kind_function && e.function == function) {
          return true;
        }
        if (e.kind == ModuliteSymbol::kind_klass && e.klass == function->class_id) {
          return true;
        }
      }
    }
  }

  if (owner->is_composer_package) {   // when it's an implicit modulite created from composer.json, all is exported
    return owner->exports.empty();    // (unless .modulite.yaml exists near composer.json and manually provides "export")
  }

  return false;
}

// constant GLOBAL_DEFINE is exported from a modulite when
// - "\\GLOBAL_DEFINE" is declared in 'export' (already checked that it's declared inside a modulite)
static bool is_global_const_exported_from(DefinePtr constant, ModulitePtr owner, FunctionPtr usage_context) {
  for (const ModuliteSymbol &e : owner->exports) {
    if (e.kind == ModuliteSymbol::kind_global_const && e.constant == constant) {
      return true;
    }
  }

  for (const auto &p : owner->allow_internal) {
    if (does_allow_internal_rule_satisfy_usage_context(usage_context, p.first)) {
      for (const ModuliteSymbol &e : p.second) {
        if (e.kind == ModuliteSymbol::kind_global_const && e.constant == constant) {
          return true;
        }
      }
    }
  }

  if (owner->is_composer_package) {   // when it's an implicit modulite created from composer.json, all is exported
    return owner->exports.empty();    // (unless .modulite.yaml exists near composer.json and manually provides "export")
  }

  return false;
}

// constant A::C is exported from a modulite when
// - "A::C" is declared in 'export'
// - or "A" is declared in 'export' AND "A::C" is not declared in 'force-internal'
// - or either "A" or "A::C" is declared in 'allow internal' for current usage context
static bool is_constant_exported_from(DefinePtr constant, ClassPtr requested_class, ModulitePtr owner, FunctionPtr usage_context) {
  for (const ModuliteSymbol &e : owner->exports) {
    if (e.kind == ModuliteSymbol::kind_constant && e.constant == constant) {
      return true;
    }
    if (e.kind == ModuliteSymbol::kind_klass && e.klass == requested_class) {
      for (const ModuliteSymbol &fi : owner->force_internal) {
        if (fi.kind == ModuliteSymbol::kind_constant && fi.constant == constant) {
          goto search_allow_internal;
        }
      }
      return true;
    }
  }

  search_allow_internal:
  for (const auto &p : owner->allow_internal) {
    if (does_allow_internal_rule_satisfy_usage_context(usage_context, p.first)) {
      for (const ModuliteSymbol &e : p.second) {
        if (e.kind == ModuliteSymbol::kind_constant && e.constant == constant) {
          return true;
        }
        if (e.kind == ModuliteSymbol::kind_klass && e.klass == requested_class) {
          return true;
        }
      }
    }
  }

  if (owner->is_composer_package) {   // when it's an implicit modulite created from composer.json, all is exported
    return owner->exports.empty();    // (unless .modulite.yaml exists near composer.json and manually provides "export")
  }

  return false;
}

// static field A::$f is exported from a modulite when
// - "A::$f" is declared in 'export'
// - or "A" is declared in 'export' AND "A::$f" is not declared in 'force-internal'
// - or either "A" or "A::$f" is declared in 'allow internal' for current usage context
static bool is_static_field_exported_from(VarPtr property, ClassPtr requested_class, ModulitePtr owner, FunctionPtr usage_context) {
  for (const ModuliteSymbol &e : owner->exports) {
    if (e.kind == ModuliteSymbol::kind_property && e.property == property) {
      return true;
    }
    if (e.kind == ModuliteSymbol::kind_klass && e.klass == requested_class) {
      for (const ModuliteSymbol &fi : owner->force_internal) {
        if (fi.kind == ModuliteSymbol::kind_property && fi.klass == requested_class && fi.property == property) {
          goto search_allow_internal;
        }
      }
      return true;
    }
  }

  search_allow_internal:
  for (const auto &p : owner->allow_internal) {
    if (does_allow_internal_rule_satisfy_usage_context(usage_context, p.first)) {
      for (const ModuliteSymbol &e : p.second) {
        if (e.kind == ModuliteSymbol::kind_property && e.property == property) {
          return true;
        }
        if (e.kind == ModuliteSymbol::kind_klass && e.klass == requested_class) {
          return true;
        }
      }
    }
  }

  if (owner->is_composer_package) {   // when it's an implicit modulite created from composer.json, all is exported
    return owner->exports.empty();    // (unless .modulite.yaml exists near composer.json and manually provides "export")
  }

  return false;
}

// submodulite @msg/core is exported from @msg when
// - "@msg/core" is declared in 'export'
// - or "@msg/core" is declared in 'allow-internal' for current usage context
// - or usage context is @msg/channels, it can access @msg/core, because @msg is their lca
// if check_all_depth, checks are done for any chains, e.g. @parent/c1/c2/c3 — c3 from c2, c2 from c1, c1 from parent
// else, check is not only for child from child->parent (c3 from c2 if given above)
static bool is_submodulite_exported(ModulitePtr child, FunctionPtr usage_context, bool check_all_depth = true) {
  ModulitePtr parent = child->parent;

  if (child->exported_from_parent) {
    return !check_all_depth || !parent->parent || is_submodulite_exported(parent, usage_context);
  }

  for (const auto &p : parent->allow_internal) {
    if (does_allow_internal_rule_satisfy_usage_context(usage_context, p.first)) {
      for (const ModuliteSymbol &e : p.second) {
        if (e.kind == ModuliteSymbol::kind_modulite && e.modulite == child) {
          return !check_all_depth || !parent->parent || is_submodulite_exported(parent, usage_context);
        }
      }
    }
  }

  if (usage_context && usage_context->modulite && parent == usage_context->modulite->find_lca_with(parent)) {
    return true;
  }

  if (parent->is_composer_package) {  // when parent is an implicit modulite created from composer.json, all is exported
    return parent->exports.empty();   // (unless .modulite.yaml exists near composer.json and manually provides "export")
  }

  return false;
}


static bool does_require_another_modulite(ModulitePtr inside_m, ModulitePtr another_m) {
  // examples: we are at inside_m = @feed, accessing another_m = @msg/channels
  // fast path: if @feed requires @msg/channels
  for (const ModuliteSymbol &req : inside_m->require) {
    if (req.kind == ModuliteSymbol::kind_modulite && req.modulite == another_m) {
      return true;
    }
  }

  // slow path: if @feed requires @msg, then @msg/channels is auto-required unless internal in @msg (for any depth)
  // same for composer packages: if @feed requires #vk/common, #vk/common/@strings is auto-required unless not exported
  for (const ModuliteSymbol &req : inside_m->require) {
    if (req.kind == ModuliteSymbol::kind_modulite && vk::contains(req.modulite->submodulites_exported_at_any_depth, another_m)) {
      return true;
    }
  }

  // contents of composer packages can also use modulite; when it's embedded into a monolith, it's like a global scope
  // so, in a way project root can access any modulite (there is no place to provide "require"),
  // a root of composer package can access any modulite within this package
  if (inside_m->is_composer_package && another_m->composer_json == inside_m->composer_json) {
    return true;
  }

  return false;
}


// this class is close to ModuliteErrFormatter in modulite-phpstan
class ModuliteErr {
  ModulitePtr inside_m;
  ModulitePtr another_m;
  FunctionPtr usage_context;
  std::string desc;

public:
  [[gnu::cold]] ModuliteErr(FunctionPtr usage_context, ClassPtr klass)
    : inside_m(usage_context->modulite)
    , another_m(klass->modulite)
    , usage_context(usage_context)
    , desc(fmt_format("use {}", TermStringFormat::paint_bold(klass->as_human_readable()))) {}

  [[gnu::cold]] ModuliteErr(FunctionPtr usage_context, FunctionPtr called_f)
    : inside_m(usage_context->modulite)
    , another_m(called_f->modulite)
    , usage_context(usage_context)
    , desc(fmt_format("call {}()", TermStringFormat::paint_bold(called_f->as_human_readable()))) {}

  [[gnu::cold]] ModuliteErr(FunctionPtr usage_context, ClassPtr requested_class, DefinePtr used_c)
    : inside_m(usage_context->modulite)
    , another_m(used_c->modulite)
    , usage_context(usage_context)
    , desc(fmt_format("use {}", TermStringFormat::paint_bold(requested_class ? requested_class->as_human_readable() : used_c->as_human_readable()))) {}

  [[gnu::cold]] ModuliteErr(FunctionPtr usage_context, ClassPtr requested_class, VarPtr property)
    : inside_m(usage_context->modulite)
    , another_m(property->class_id->modulite)
    , usage_context(usage_context)
    , desc(fmt_format("use {}", TermStringFormat::paint_bold(requested_class ? requested_class->as_human_readable() : property->as_human_readable()))) {}

  [[gnu::cold]] ModuliteErr(FunctionPtr usage_context, const std::string &global_var_name)
    : inside_m(usage_context->modulite)
    , another_m({})
    , usage_context(usage_context)
    , desc(fmt_format("use global {}", TermStringFormat::paint_bold("$" + global_var_name))) {}

  [[gnu::cold]] void print_error_symbol_is_not_exported() {
    kphp_error(0, fmt_format("[modulite] restricted to {}, it's internal in {}", desc, another_m->modulite_name));
  }

  [[gnu::cold]] void print_error_submodulite_is_not_exported() {
    kphp_assert(another_m->parent && !is_submodulite_exported(another_m, usage_context));
    ModulitePtr child_internal = another_m;
    while (is_submodulite_exported(child_internal, usage_context, false)) {
      child_internal = child_internal->parent;
    }
    kphp_error(0, fmt_format("[modulite] restricted to {}, {} is internal in {}", desc, child_internal->modulite_name, child_internal->parent->modulite_name));
  }

  [[gnu::cold]] void print_error_modulite_is_not_required() {
    if (inside_m->composer_json && another_m->composer_json) {
      kphp_error(0, fmt_format("[modulite] restricted to {}, {} is not required by {} in composer.json", desc, another_m->modulite_name, inside_m->modulite_name));
    } else {
      kphp_error(0, fmt_format("[modulite] restricted to {}, {} is not required by {}", desc, another_m->modulite_name, inside_m->modulite_name));
    }
  }

  [[gnu::cold]] void print_error_symbol_is_not_required() {
    if (inside_m->is_composer_package) {
      kphp_error(0, fmt_format("[modulite] restricted to {}, it does not belong to package {}", desc, inside_m->modulite_name));
    } else {
      kphp_error(0, fmt_format("[modulite] restricted to {}, it's not required by {}", desc, inside_m->modulite_name));
    }
  }
};


void modulite_check_when_use_class(FunctionPtr usage_context, ClassPtr klass) {
  ModulitePtr inside_m = usage_context->modulite;
  ModulitePtr another_m = klass->modulite;
  if (inside_m == another_m || should_this_usage_context_be_ignored(usage_context)) {
    return;
  }

  if (another_m) {
    if (!is_class_exported_from(klass, another_m, usage_context)) {
      ModuliteErr(usage_context, klass).print_error_symbol_is_not_exported();
      return;
    }

    if (another_m->parent && !is_submodulite_exported(another_m, usage_context)) {
      ModuliteErr(usage_context, klass).print_error_submodulite_is_not_exported();
      return;
    }
  }

  if (inside_m) {
    bool should_require_m_instead = another_m && (!another_m->is_composer_package || another_m->composer_json != inside_m->composer_json);
    if (should_require_m_instead) {
      if (!does_require_another_modulite(inside_m, another_m)) {
        ModuliteErr(usage_context, klass).print_error_modulite_is_not_required();
      }

    } else {
      bool in_require = vk::any_of(inside_m->require, [klass](const ModuliteSymbol &s) {
        return s.kind == ModuliteSymbol::kind_klass && s.klass == klass;
      });
      if (!in_require && !klass->is_builtin()) {
        ModuliteErr(usage_context, klass).print_error_symbol_is_not_required();
      }
    }
  }
}

void modulite_check_when_use_global_const(FunctionPtr usage_context, DefinePtr used_c) {
  ModulitePtr inside_m = usage_context->modulite;
  ModulitePtr another_m = used_c->modulite;
  if (inside_m == another_m || should_this_usage_context_be_ignored(usage_context)) {
    return;
  }

  if (another_m) {
    if (!is_global_const_exported_from(used_c, another_m, usage_context)) {
      ModuliteErr(usage_context, {}, used_c).print_error_symbol_is_not_exported();
      return;
    }

    if (another_m->parent && !is_submodulite_exported(another_m, usage_context)) {
      ModuliteErr(usage_context, {}, used_c).print_error_submodulite_is_not_exported();
      return;
    }
  }

  if (inside_m) {
    bool should_require_m_instead = another_m && (!another_m->is_composer_package || another_m->composer_json != inside_m->composer_json);
    if (should_require_m_instead) {
      if (!does_require_another_modulite(inside_m, another_m)) {
        ModuliteErr(usage_context, {}, used_c).print_error_modulite_is_not_required();
      }

    } else {
      bool in_require = vk::any_of(inside_m->require, [used_c](const ModuliteSymbol &s) {
        return s.kind == ModuliteSymbol::kind_global_const && s.constant == used_c;
      });
      if (!in_require && !used_c->is_builtin()) {
        ModuliteErr(usage_context, {}, used_c).print_error_symbol_is_not_required();
      }
    }
  }
}

void modulite_check_when_use_constant(FunctionPtr usage_context, DefinePtr used_c, ClassPtr requested_class) {
  ModulitePtr inside_m = usage_context->modulite;
  ModulitePtr another_m = requested_class->modulite;
  if (inside_m == another_m || should_this_usage_context_be_ignored(usage_context)) {
    return;
  }

  if (another_m) {
    if (!is_constant_exported_from(used_c, requested_class, another_m, usage_context)) {
      ModuliteErr(usage_context, requested_class, used_c).print_error_symbol_is_not_exported();
      return;
    }

    if (another_m->parent && !is_submodulite_exported(another_m, usage_context)) {
      ModuliteErr(usage_context, requested_class, used_c).print_error_submodulite_is_not_exported();
      return;
    }
  }

  if (inside_m) {
    bool should_require_m_instead = another_m && (!another_m->is_composer_package || another_m->composer_json != inside_m->composer_json);
    if (should_require_m_instead) {
      if (!does_require_another_modulite(inside_m, another_m)) {
        ModuliteErr(usage_context, requested_class, used_c).print_error_modulite_is_not_required();
      }

    } else {
      bool in_require = vk::any_of(inside_m->require, [used_c, requested_class](const ModuliteSymbol &s) {
        return (s.kind == ModuliteSymbol::kind_klass && s.klass == requested_class) ||
               (s.kind == ModuliteSymbol::kind_constant && s.constant == used_c);
      });
      if (!in_require && !used_c->is_builtin()) {
        ModuliteErr(usage_context, requested_class, used_c).print_error_symbol_is_not_required();
      }
    }
  }
}

void modulite_check_when_call_function(FunctionPtr usage_context, FunctionPtr called_f) {
  ModulitePtr inside_m = usage_context->modulite;
  ModulitePtr another_m = called_f->modulite;
  if (inside_m == another_m || should_this_usage_context_be_ignored(usage_context)) {
    return;
  }

  if (called_f->is_instantiation_of_generic_function()) {
    called_f = called_f->outer_function;
  }
  if (usage_context->is_instantiation_of_generic_function()) {
    usage_context = usage_context->outer_function;
  }

  if (another_m) {
    if (!is_function_exported_from(called_f, another_m, usage_context)) {
      ModuliteErr(usage_context, called_f).print_error_symbol_is_not_exported();
      return;
    }

    if (another_m->parent && !is_submodulite_exported(another_m, usage_context)) {
      ModuliteErr(usage_context, called_f).print_error_submodulite_is_not_exported();
      return;
    }
  }

  if (inside_m) {
    bool should_require_m_instead = another_m && (!another_m->is_composer_package || another_m->composer_json != inside_m->composer_json);
    if (should_require_m_instead) {
      if (!does_require_another_modulite(inside_m, another_m)) {
        ModuliteErr(usage_context, called_f).print_error_modulite_is_not_required();
      }

    } else {
      bool in_require = vk::any_of(inside_m->require, [called_f](const ModuliteSymbol &s) {
        return s.kind == ModuliteSymbol::kind_function && s.function == called_f;
      });
      if (!in_require) {  // builtin functions were beforehand filtered out by func_local condition
        ModuliteErr(usage_context, called_f).print_error_symbol_is_not_required();
      }
    }
  }
}

void modulite_check_when_use_static_field(FunctionPtr usage_context, VarPtr property, ClassPtr requested_class) {
  ModulitePtr inside_m = usage_context->modulite;
  ModulitePtr another_m = requested_class->modulite;
  if (inside_m == another_m || should_this_usage_context_be_ignored(usage_context)) {
    return;
  }

  if (another_m) {
    if (!is_static_field_exported_from(property, requested_class, another_m, usage_context)) {
      ModuliteErr(usage_context, requested_class, property).print_error_symbol_is_not_exported();
      return;
    }

    if (another_m->parent && !is_submodulite_exported(another_m, usage_context)) {
      ModuliteErr(usage_context, requested_class, property).print_error_submodulite_is_not_exported();
      return;
    }
  }

  if (inside_m) {
    bool should_require_m_instead = another_m && (!another_m->is_composer_package || another_m->composer_json != inside_m->composer_json);
    if (should_require_m_instead) {
      if (!does_require_another_modulite(inside_m, another_m)) {
        ModuliteErr(usage_context, requested_class, property).print_error_modulite_is_not_required();
      }

    } else {
      bool in_require = vk::any_of(inside_m->require, [property, requested_class](const ModuliteSymbol &s) {
        return (s.kind == ModuliteSymbol::kind_klass && s.klass == requested_class) ||
               (s.kind == ModuliteSymbol::kind_property && s.property == property);
      });
      if (!in_require && !requested_class->is_builtin()) {
        ModuliteErr(usage_context, requested_class, property).print_error_symbol_is_not_required();
      }
    }
  }
}

void modulite_check_when_global_keyword(FunctionPtr usage_context, const std::string &global_var_name) {
  ModulitePtr inside_m = usage_context->modulite;
  if (should_this_usage_context_be_ignored(usage_context)) {
    return;
  }

  if (inside_m) {
    bool in_require = vk::any_of(inside_m->require, [global_var_name](const ModuliteSymbol &s) {
      return s.kind == ModuliteSymbol::kind_global_var && s.global_var == global_var_name;
    });
    if (!in_require && !inside_m->is_composer_package) {  // composer root, like project root, has no place to declare requires
      ModuliteErr(usage_context, global_var_name).print_error_symbol_is_not_required();
    }
  }
}

