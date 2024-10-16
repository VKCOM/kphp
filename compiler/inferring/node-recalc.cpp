// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/inferring/node-recalc.h"

#include "common/termformat/termformat.h"
#include "common/wrappers/likely.h"

#include "compiler/data/class-data.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/type-inferer.h"
#include "compiler/stage.h"

// this callback is invoked immediately when new_type_ has become tp_Error during recalculation
// by default, we try to detect the reason and print it out
// (it may be because of mixing classes with primitives in an array, for example — they any_key will emerge tp_Error)
// note, that if a node is restricted with phpdoc, no error is printed, as VarNode overloads this callback
void NodeRecalc::on_new_type_became_tpError(const TypeData *because_of_type, const RValue &because_of_rvalue) {
  PrimitiveType ptype_before_error = node_->get_type()->ptype();
  ClassPtr mix_class = new_type_->get_first_class_type_inside();
  ClassPtr mix_class2 = because_of_type->get_first_class_type_inside();
  std::string desc1 = node_->get_description();
  std::string desc2 = because_of_rvalue.node ? because_of_rvalue.node->get_description() : because_of_rvalue.type->as_human_readable();
  if (because_of_rvalue.node) {
    stage::set_location(because_of_rvalue.node->get_location());
  }

  if (mix_class && mix_class2 && mix_class != mix_class2 && !vk::any_of_equal(ptype_before_error, tp_tuple, tp_shape)) {
    kphp_error(0, fmt_format("Type Error: mix classes {} and {}: {} and {}\n",
                             TermStringFormat::paint_green(mix_class->name), TermStringFormat::paint_green(mix_class2->name),
                             desc1, desc2));

  } else if ((mix_class || mix_class2) && !vk::any_of_equal(ptype_before_error, tp_tuple, tp_shape)) {
    const auto class_name = TermStringFormat::paint_green(mix_class ? mix_class->name : mix_class2->name);
    kphp_error(0, fmt_format("Type Error: mix class {} with non-class: {} [{}] and {} [{}]\n", class_name, desc1, (void*)node_, desc2, (void*)because_of_rvalue.node));

  } else if (ptype_before_error == tp_tuple && because_of_type->ptype() == tp_tuple) {
    kphp_error(0, fmt_format("Type Error: inconsistent tuples {} and {}\n", desc1, desc2));

  } else if (ptype_before_error == tp_shape && because_of_type->ptype() == tp_shape) {
    kphp_error(0, fmt_format("Error combining shapes: some type mismatch inside, turned into Error"));

  } else if (ptype_before_error != tp_tuple && because_of_type->ptype() == tp_tuple) {
    kphp_error(0, fmt_format("Type Error: tuples are read-only (tuple {})\n", desc1));

  } else if (ptype_before_error != tp_shape && because_of_type->ptype() == tp_shape) {
    kphp_error(0, fmt_format("Type Error: shapes are read-only (shape {})\n", desc1));

  } else if (ptype_before_error == tp_void || because_of_type->ptype() == tp_void) {
    kphp_error(0, fmt_format("Type Error: mixing void and non-void expressions ({} and {})\n", desc1, desc2));
    
  } else {
    kphp_error (0, fmt_format("Type Error [{}] updated by [{}]\n", desc1, desc2));
  }
}


const TypeData *NodeRecalc::new_type() {
  return new_type_;
}

bool NodeRecalc::auto_edge_flag() {
  return false;
}

void NodeRecalc::add_dependency_impl(tinf::Node *from, tinf::Node *to, const MultiKey *from_at) {
  tinf::Edge *e = new tinf::Edge();
  e->from = from;
  e->to = to;
  // for any key (most of cases) save the pointer, as it remains valid always, otherwise clone, as it may be in temporary memory
  e->from_at = from_at ? from_at->depth() == 1 && from_at->begin()->is_any_key() ? from_at : new MultiKey(*from_at) : nullptr;
  inferer_->add_edge(e);
  inferer_->add_node(e->to);
}

void NodeRecalc::add_dependency(const RValue &rvalue) {
  if (auto_edge_flag() && rvalue.node != nullptr) {
    add_dependency_impl(node_, rvalue.node, nullptr);
  }
}

void NodeRecalc::set_lca_at(const MultiKey *key, const RValue &rvalue) {
  if (new_type_->error_flag()) {
    return;
  }
  const TypeData *type = nullptr;
  if (rvalue.node != nullptr) {
    if (auto_edge_flag()) {
      add_dependency_impl(node_, rvalue.node, key);
    }
    type = rvalue.node->get_type();
  } else if (rvalue.type != nullptr) {
    type = rvalue.type;
  } else {
    kphp_fail();
  }

  if (rvalue.key != nullptr) {
    type = type->const_read_at(*rvalue.key);
  }
  if (key == nullptr) {
    key = &MultiKey::any_key(0);
  }

  new_type_->set_lca_at(*key, type, !rvalue.drop_or_false, !rvalue.drop_or_null, rvalue.ffi_flags);

  if (unlikely(new_type_->error_flag())) {
    on_new_type_became_tpError(type, rvalue);
  }
}

void NodeRecalc::set_lca_at(const MultiKey *key, VertexPtr expr) {
  set_lca_at(key, as_rvalue(expr));
}

void NodeRecalc::set_lca_at(const MultiKey *key, PrimitiveType ptype) {
  set_lca_at(key, as_rvalue(TypeData::get_type(ptype)));
}

void NodeRecalc::set_lca(const RValue &rvalue) {
  set_lca_at(nullptr, rvalue);
}

void NodeRecalc::set_lca(PrimitiveType ptype) {
  set_lca_at(nullptr, as_rvalue(TypeData::get_type(ptype)));
}

void NodeRecalc::set_lca(FunctionPtr function, int id) {
  set_lca_at(nullptr, as_rvalue(function, id));
}

void NodeRecalc::set_lca(VertexPtr vertex, const MultiKey *key /* = nullptr*/) {
  set_lca_at(nullptr, as_rvalue(vertex, key));
}

void NodeRecalc::set_lca(const TypeData *type, const MultiKey *key /* = nullptr*/) {
  set_lca_at(nullptr, as_rvalue(type, key));
}

void NodeRecalc::set_lca(VarPtr var) {
  set_lca_at(nullptr, as_rvalue(var));
}

void NodeRecalc::set_lca(ClassPtr klass) {
  kphp_assert(klass);
  set_lca(klass->type_data);
}


NodeRecalc::NodeRecalc(tinf::Node *node, tinf::TypeInferer *inferer) :
  new_type_(nullptr),
  node_(node),
  inferer_(inferer) {}

void NodeRecalc::on_changed() {
  //fmt_fprintf(stderr, "{} : {} -> {}\n", node_->get_description(), type_out(node_->get_type()), type_out(new_type()));
  new_type_->mark_classes_used();
  node_->set_type(new_type_);
  new_type_ = nullptr;

  // here we need a lock: in case another thread updates edges_to_this_ just now via auto edge
  AutoLocker<Lockable *> locker(node_);
  for (const tinf::Edge *e : node_->get_edges_to_this()) {
    inferer_->recalc_node(e->from);
  }
}

void NodeRecalc::run() {
  // current node type is node_->type_
  // while recalculation (in set_lca_at() and others), new_type_ is updated
  // after recalculation, if new_type_ was updated somehow, it is cloned to node_->type_
  const TypeData *before = node_->get_type();
  new_type_ = before->clone();

  do_recalc();
  if (new_type_->ptype() == tp_array) {
    new_type_->fix_inf_array();
  }

  // detect if new_type_ differs from before (the first line is a small optimization, not to invoke a function if obvious)
  bool changed = new_type_->ptype() != before->ptype() || new_type_->flags() != before->flags() ||
                 new_type_->did_type_data_change_after_tinf_step(before);
  if (changed) {
    on_changed();
  }

  delete new_type_;
}
