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
 */


static bool does_allow_internal_rule_satisfy_usage_context(FunctionPtr usage_context, const ModuliteSymbol &rule) {
  const FunctionData *outer = usage_context->get_this_or_topmost_if_lambda();
  return (rule.kind == ModuliteSymbol::kind_function && rule.function.operator->() == outer) ||
         (rule.kind == ModuliteSymbol::kind_klass && rule.klass == outer->class_id) ||
         (rule.kind == ModuliteSymbol::kind_modulite && rule.modulite == outer->modulite);
}

static inline bool is_env_modulite_enabled() {
  return G->settings().modulite_enabled.get();
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

  return false;
}

// constant A::C is exported from a modulite when
// - "A::C" is declared in 'export'
// - or "A" is declared in 'export' AND "A::C" is not declared in 'force-internal'
// - or either "A" or "A::C" is declared in 'allow internal' for current usage context
static bool is_constant_exported_from(DefinePtr constant, ModulitePtr owner, FunctionPtr usage_context) {
  for (const ModuliteSymbol &e : owner->exports) {
    if (e.kind == ModuliteSymbol::kind_constant && e.constant == constant) {
      return true;
    }
    if (e.kind == ModuliteSymbol::kind_klass && e.klass == constant->class_id) {
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
        if (e.kind == ModuliteSymbol::kind_klass && e.klass == constant->class_id) {
          return true;
        }
      }
    }
  }

  return false;
}

// static field A::$f is exported from a modulite when
// - "A::$f" is declared in 'export'
// - or "A" is declared in 'export' AND "A::$f" is not declared in 'force-internal'
// - or either "A" or "A::$f" is declared in 'allow internal' for current usage context
static bool is_static_field_exported_from(VarPtr field, ModulitePtr owner, FunctionPtr usage_context) {
  for (const ModuliteSymbol &e : owner->exports) {
    if (e.kind == ModuliteSymbol::kind_global_var && e.global_var == field->name) {
      return true;
    }
    if (e.kind == ModuliteSymbol::kind_klass && e.klass == field->class_id) {
      for (const ModuliteSymbol &fi : owner->force_internal) {
        if (fi.kind == ModuliteSymbol::kind_global_var && fi.global_var == field->name) {
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
        if (e.kind == ModuliteSymbol::kind_global_var && e.global_var == field->name) {
          return true;
        }
        if (e.kind == ModuliteSymbol::kind_klass && e.klass == field->class_id) {
          return true;
        }
      }
    }
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
  for (const ModuliteSymbol &req : inside_m->require) {
    if (req.kind == ModuliteSymbol::kind_modulite && vk::contains(req.modulite->submodulites_exported_at_any_depth, another_m)) {
      return true;
    }
  }
  
  return false;
}




void modulite_check_when_use_class(FunctionPtr usage_context, ClassPtr klass) {
  if (!is_env_modulite_enabled()) {
    return;
  }

  ModulitePtr inside_m = usage_context->modulite;
  ModulitePtr another_m = klass->modulite;
  if (inside_m == another_m) {
    return;
  }

  if (another_m && !is_class_exported_from(klass, another_m, usage_context)) {
    kphp_error(0, fmt_format("[modulite] restricted to use {}, it's internal in {}", TermStringFormat::paint_bold(klass->as_human_readable()), another_m->modulite_name));
    return;
  }

  if (inside_m) {
    if (another_m) {
      kphp_error(does_require_another_modulite(inside_m, another_m),
                 fmt_format("[modulite] restricted to use {}, {} is not required by {}", TermStringFormat::paint_bold(klass->as_human_readable()), another_m->modulite_name, inside_m->modulite_name));
      return;
    }

    bool in_require = vk::any_of(inside_m->require, [klass](const ModuliteSymbol &s) {
      return s.kind == ModuliteSymbol::kind_klass && s.klass == klass;
    });
    kphp_error(in_require, fmt_format("[modulite] restricted to use {}, it's not required by {}", TermStringFormat::paint_bold(klass->as_human_readable()), inside_m->modulite_name));
  }
}

void modulite_check_when_use_constant(FunctionPtr usage_context, DefinePtr used_c) {
  if (!is_env_modulite_enabled()) {
    return;
  }

  ModulitePtr inside_m = usage_context->modulite;
  ModulitePtr another_m = used_c->modulite;
  if (inside_m == another_m) {
    return;
  }

  if (another_m && !is_constant_exported_from(used_c, another_m, usage_context)) {
    kphp_error(0, fmt_format("[modulite] restricted to use {}, it's internal in {}", TermStringFormat::paint_bold(used_c->as_human_readable()), another_m->modulite_name));
    return;
  }

  if (inside_m) {
    if (another_m) {
      kphp_error(does_require_another_modulite(inside_m, another_m),
                 fmt_format("[modulite] restricted to use {}, {} is not required by {}", TermStringFormat::paint_bold(used_c->as_human_readable()), another_m->modulite_name, inside_m->modulite_name));
      return;
    }

    bool in_require = vk::any_of(inside_m->require, [used_c](const ModuliteSymbol &s) {
      return (s.kind == ModuliteSymbol::kind_klass && s.klass == used_c->class_id) ||
             (s.kind == ModuliteSymbol::kind_constant && s.constant == used_c);
    });
    kphp_error(in_require, fmt_format("[modulite] restricted to use {}, it's not required by {}", TermStringFormat::paint_bold(used_c->as_human_readable()), inside_m->modulite_name));
  }
}


void modulite_check_when_call_function(FunctionPtr usage_context, FunctionPtr called_f) {
  if (!is_env_modulite_enabled()) {
    return;
  }

  ModulitePtr inside_m = usage_context->modulite;
  ModulitePtr another_m = called_f->modulite;
  if (inside_m == another_m) {
    return;
  }

  if (another_m && !is_function_exported_from(called_f, another_m, usage_context)) {
    kphp_error(0, fmt_format("[modulite] restricted to call {}(), it's internal in {}", TermStringFormat::paint_bold(called_f->as_human_readable()), another_m->modulite_name));
    return;
  }

  if (inside_m) {
    if (another_m) {
      kphp_error(does_require_another_modulite(inside_m, another_m),
                 fmt_format("[modulite] restricted to call {}(), {} is not required by {}", TermStringFormat::paint_bold(called_f->as_human_readable()), another_m->modulite_name, inside_m->modulite_name));
      return;
    }

    bool in_require = vk::any_of(inside_m->require, [called_f](const ModuliteSymbol &s) {
      return s.kind == ModuliteSymbol::kind_function && s.function == called_f;
    });
    kphp_error(in_require, fmt_format("[modulite] restricted to call {}(), it's not required by {}", TermStringFormat::paint_bold(called_f->as_human_readable()), inside_m->modulite_name));
  }
}

void modulite_check_when_use_static_field(FunctionPtr usage_context, VarPtr field) {
  if (!is_env_modulite_enabled()) {
    return;
  }

  ModulitePtr inside_m = usage_context->modulite;
  ModulitePtr another_m = field->class_id->modulite;
  if (inside_m == another_m) {
    return;
  }

  if (another_m && !is_static_field_exported_from(field, another_m, usage_context)) {
    kphp_error(0, fmt_format("[modulite] restricted to use {}, it's internal in {}", TermStringFormat::paint_bold(field->as_human_readable()), another_m->modulite_name));
    return;
  }

  if (inside_m) {
    if (another_m) {
      kphp_error(does_require_another_modulite(inside_m, another_m),
                 fmt_format("[modulite] restricted to use {}, {} is not required by {}", TermStringFormat::paint_bold(field->as_human_readable()), another_m->modulite_name, inside_m->modulite_name));
      return;
    }

    bool in_require = vk::any_of(inside_m->require, [field](const ModuliteSymbol &s) {
      return (s.kind == ModuliteSymbol::kind_klass && s.klass == field->class_id) ||
             (s.kind == ModuliteSymbol::kind_global_var && s.global_var == field->name);
    });
    kphp_error(in_require, fmt_format("[modulite] restricted to use {}, it's not required by {}", TermStringFormat::paint_bold(field->as_human_readable()), inside_m->modulite_name));
  }
}

void modulite_check_when_global_keyword(FunctionPtr usage_context, const std::string &global_var_name) {
  if (!is_env_modulite_enabled()) {
    return;
  }

  ModulitePtr inside_m = usage_context->modulite;

  if (inside_m) {
    bool in_require = vk::any_of(inside_m->require, [global_var_name](const ModuliteSymbol &s) {
      return s.kind == ModuliteSymbol::kind_global_var && s.global_var == global_var_name;
    });
    kphp_error(in_require, fmt_format("[modulite] restricted to use global {}, it's not required by {}", TermStringFormat::paint_bold("$" + global_var_name), inside_m->modulite_name));
  }
}

