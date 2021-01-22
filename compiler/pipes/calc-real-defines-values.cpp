// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/calc-real-defines-values.h"

#include "common/version-string.h"
#include "common/wrappers/likely.h"

CalcRealDefinesValuesF::CalcRealDefinesValuesF() : Base() {
  auto val = VertexAdaptor<op_string>::create();
  val->set_string(G->settings().get_version());
  DefineData *data = new DefineData("KPHP_COMPILER_VERSION", val, DefineData::def_const);
  G->register_define(DefinePtr(data));
}


void CalcRealDefinesValuesF::on_finish(DataStream<FunctionPtr> &os) {
  stage::set_name("Calc real defines values");
  stage::set_file(SrcFilePtr());
  stage::die_if_global_errors();

  for (const auto &define : G->get_defines()) {
    process_define(define);
  }

  stage::die_if_global_errors();
  Base::on_finish(os);
}

void CalcRealDefinesValuesF::process_define_recursive(VertexPtr root) {
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

void CalcRealDefinesValuesF::process_define(DefinePtr def) {
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
  } else {
    def->type() = DefineData::def_var;
  }
}

void CalcRealDefinesValuesF::print_error_infinite_define(DefinePtr cur_def) {
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
