// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-real-defines-values.h"

#include "common/version-string.h"
#include "common/wrappers/likely.h"

#include "compiler/data/src-dir.h"
#include "compiler/data/src-file.h"
#include "compiler/modulite-check-rules.h"

/*
 * This is a sync point needed for two things.
 *
 * 1) Calculate real defines values, dealing with dependencies (defines can't be recursive):
 *      const ONE = 1;                            // 1
 *      const TWO = self::ONE + self::ONE;        // 1+1
 *      const IDS = [self::TWO*self::ONE, ID_ME]; // [2*1, 336]
 *    Before, in previous pipes, defines values were extracted and registered,
 *    here we merge them all in a single thread.
 *    After `on_finish()`, defines are inlined as constant values. That's why we need a sync point here.
 *
 * 2) Assign f->modulite, klass->modulite, def->modulite.
 *    These properties will later be used for checking func calls, class references, etc.
 *    They are assigned from ->file_id: all functions/classes in file.php are placed in the same modulite.
 *    After `on_finish()`, they will be used in parallel. So, they must be assigned before this sync point finishes.
 *    This pipe is chosen, because it handles all defines in a bulk, so it's logical to assign def->modulite here also.
 *    (functions and classes could be assigned some pipes before, but there's no reason to split them)
 */


CalcRealDefinesAndAssignModulitesF::CalcRealDefinesAndAssignModulitesF() : Base() {
  auto val = VertexAdaptor<op_string>::create();
  val->set_string(G->settings().get_version());
  DefineData *data = new DefineData("KPHP_COMPILER_VERSION", val, DefineData::def_const);
  data->file_id = G->get_main_file();
  G->register_define(DefinePtr(data));
}

void CalcRealDefinesAndAssignModulitesF::execute(FunctionPtr f, DataStream<FunctionPtr> &unused_os) {
  if (f->type == FunctionData::func_class_holder) {
    f->class_id->modulite = f->file_id->dir->nested_files_modulite;
  }

  // not just f->file_id, because if f = (method cloned from trait), then f->file_id = (file with trait)
  // but we need a file of a containing class, since a trait might be imported from another modulite
  SrcFilePtr file_id = f->modifiers.is_nonmember() ? f->file_id : f->class_id->file_id;
  f->modulite = file_id->dir->nested_files_modulite;

  Base::execute(f, unused_os);
}

void CalcRealDefinesAndAssignModulitesF::on_finish(DataStream<FunctionPtr> &os) {
  stage::set_name("Calc real defines values");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();

  for (DefinePtr define : G->get_defines()) {
    process_define(define);
    define->modulite = (define->class_id ? define->class_id->file_id : define->file_id)->dir->nested_files_modulite;
  }
  stage::die_if_global_errors();

  for (ClassPtr klass : G->get_classes()) {
    if (!klass->parent_class && klass->implements.empty()) {
      continue;
    }

    FunctionPtr f_klass = klass->get_holder_function();
    stage::set_location({klass->file_id, f_klass, klass->location_line_num});

    if (klass->parent_class) {
      if (klass->modulite != klass->parent_class->modulite) {
        modulite_check_when_use_class(f_klass, klass->parent_class);
      }
    }
    for (ClassPtr interface : klass->implements) {
      if (klass->modulite != interface->modulite) {
        modulite_check_when_use_class(f_klass, interface);
      }
    }
  }
  stage::die_if_global_errors();

  Base::on_finish(os);
}

void CalcRealDefinesAndAssignModulitesF::process_define_recursive(VertexPtr root) {
  if (root->type() == op_func_name) {
    auto define_name = resolve_define_name(root->get_string());
    DefinePtr define = G->get_define(define_name);
    if (define) {
      process_define(define);
    } else {
      kphp_error(0, fmt_format("Can't find definition for '{}'", define_name));
    }
  }
  for (auto i : *root) {
    process_define_recursive(i);
  }
}

void CalcRealDefinesAndAssignModulitesF::process_define(DefinePtr def) {
  stage::set_location(def->val->location);

  if (def->type() != DefineData::def_unknown) {
    return;
  }

  if (unlikely(in_progress.find(&def->name) != in_progress.end())) {
    print_error_infinite_define(def);
    stage::die_if_global_errors();
    return;
  }

  in_progress.insert(&def->name);
  stack.push_back(&def->name);

  process_define_recursive(def->val);
  stage::set_location(def->val->location);

  in_progress.erase(&def->name);
  stack.pop_back();

  if (check_const.is_const(def->val)) {
    if (def->class_id) {
      check_const_access.check(def->val, def->class_id);
    }
    def->type() = DefineData::def_const;
    def->val = make_const.make_const(def->val);
    def->val->const_type = cnst_const_val;
    if (auto s = def->val.try_as<op_define_val>()) {
      s->value()->const_type = cnst_const_val;
    }
  } else {
    def->type() = DefineData::def_var;
  }
}

void CalcRealDefinesAndAssignModulitesF::print_error_infinite_define(DefinePtr cur_def) {
  std::string str_stack;
  int id = -1;
  for (size_t i = 0; i < stack.size(); i++) {
    if (stack[i] == &cur_def->name) {
      id = i;
      break;
    }
  }
  kphp_assert(id != -1);
  for (size_t i = id; i < stack.size(); i++) {
    str_stack += *stack[i];
    str_stack += " -> ";
  }
  str_stack += cur_def->name;
  kphp_error(0, fmt_format("Recursive define dependency:\n{}\n", str_stack));
}
