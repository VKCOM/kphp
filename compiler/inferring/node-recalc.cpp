#include "compiler/inferring/node-recalc.h"

#include "common/wrappers/likely.h"

#include "compiler/data/class-data.h"
#include "compiler/inferring/edge.h"
#include "compiler/inferring/type-inferer.h"
#include "compiler/stage.h"

static void print_why_tinf_occured_error(
  const TypeData *errored_type,
  const TypeData *because_of_type,
  PrimitiveType ptype_before_error,
  tinf::Node *node1,
  tinf::Node *node2
) {
  ClassPtr mix_class = errored_type->get_first_class_type_inside();
  ClassPtr mix_class2 = because_of_type->get_first_class_type_inside();
  std::string desc1 = node1->get_description();
  std::string desc2 = node2 ? node2->get_description() : "unknown";

  if (mix_class && mix_class2 && mix_class != mix_class2) {
    kphp_error(0, format("Type Error: mix classes %s and %s: %s and %s\n",
                          mix_class->name.c_str(), mix_class2->name.c_str(),
                          desc1.c_str(), desc2.c_str()));

  } else if (mix_class || mix_class2) {
    kphp_error(0, format("Type Error: mix class %s with non-class: %s and %s\n",
                          mix_class ? mix_class->name.c_str() : mix_class2->name.c_str(),
                          desc1.c_str(), desc2.c_str()));

  } else if (ptype_before_error == tp_tuple && because_of_type->ptype() == tp_tuple) {
    kphp_error(0, format("Type Error: inconsistent tuples %s and %s\n",
                          desc1.c_str(), desc2.c_str()));

  } else if (ptype_before_error != tp_tuple && because_of_type->ptype() == tp_tuple) {
    kphp_error(0, format("Type Error: tuples are read-only (tuple %s)\n",
                          desc1.c_str()));

  } else if (ptype_before_error == tp_void || because_of_type->ptype() == tp_void) {
    kphp_error(0, format("Type Error: mixing void and non-void expressions (%s and %s)\n", desc1.c_str(), desc2.c_str()));
  } else {
    kphp_error (0, format("Type Error [%s] updated by [%s]\n", desc1.c_str(), desc2.c_str()));
  }
}


const TypeData *NodeRecalc::new_type() {
  return new_type_;
}

bool NodeRecalc::auto_edge_flag() {
  return false;
}

void NodeRecalc::add_dependency_impl(tinf::Node *from, tinf::Node *to) {
  tinf::Edge *e = new tinf::Edge();
  e->from = from;
  e->to = to;
  e->from_at = nullptr;
  inferer_->add_edge(e);
  inferer_->add_node(e->to);
}

void NodeRecalc::add_dependency(const RValue &rvalue) {
  if (auto_edge_flag() && rvalue.node != nullptr) {
    add_dependency_impl(node_, rvalue.node);
  }
}

void NodeRecalc::set_lca_at(const MultiKey *key, const RValue &rvalue) {
  if (new_type_->error_flag()) {
    return;
  }
  const TypeData *type = nullptr;
  PrimitiveType ptype = new_type_->ptype();
  if (rvalue.node != nullptr) {
    if (auto_edge_flag()) {
      add_dependency_impl(node_, rvalue.node);
    }
    __sync_synchronize();
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

  if (type->error_flag()) {
    return;
  }

  if (types_stack.empty()) {
    new_type_->set_lca_at(*key, type, !rvalue.drop_or_false);
  } else {
    types_stack.back()->set_lca_at(*key, type, !rvalue.drop_or_false);
  }

  if (unlikely(new_type_->error_flag())) {
    print_why_tinf_occured_error(new_type_, type, ptype, node_, rvalue.node);
  }
}

void NodeRecalc::set_lca_at(const MultiKey *key, VertexPtr expr) {
  set_lca_at(key, as_rvalue(expr));
}

void NodeRecalc::set_lca_at(const MultiKey *key, PrimitiveType ptype) {
  set_lca_at(key, as_rvalue(ptype));
}

void NodeRecalc::set_lca(const RValue &rvalue) {
  set_lca_at(nullptr, rvalue);
}

void NodeRecalc::set_lca(PrimitiveType ptype) {
  set_lca(as_rvalue(ptype));
}

void NodeRecalc::set_lca(FunctionPtr function, int id) {
  set_lca(as_rvalue(function, id));
}

void NodeRecalc::set_lca(VertexPtr vertex, const MultiKey *key /* = nullptr*/) {
  set_lca(as_rvalue(vertex, key));
}

void NodeRecalc::set_lca(const TypeData *type, const MultiKey *key /* = nullptr*/) {
  set_lca(as_rvalue(type, key));
}

void NodeRecalc::set_lca(VarPtr var) {
  set_lca(as_rvalue(var));
}

void NodeRecalc::set_lca(ClassPtr klass) {
  TypeData *type = TypeData::get_type(tp_Class)->clone();
  type->set_class_type(klass);
  set_lca(type);
  delete type;
}


NodeRecalc::NodeRecalc(tinf::Node *node, tinf::TypeInferer *inferer) :
  new_type_(nullptr),
  node_(node),
  inferer_(inferer) {}

void NodeRecalc::on_changed() {
  __sync_synchronize();

  //fprintf(stderr, "%s : %s -> %s\n", node_->get_description().c_str(), type_out(node_->get_type()).c_str(), type_out(new_type()).c_str());
  node_->set_type(new_type_);
  new_type_ = nullptr;

  __sync_synchronize();

  AutoLocker<Lockable *> locker(node_);
  for (auto e : node_->get_rev_next()) {
    inferer_->recalc_node(e->from);
  }
}

void NodeRecalc::run() {
  const TypeData *old_type = node_->get_type();
  new_type_ = old_type->clone();

  TypeData::upd_generation(new_type_->generation());
  TypeData::generation_t old_generation = new_type_->generation();
  TypeData::inc_generation();

  do_recalc();
  new_type_->fix_inf_array();

  if (new_type_->generation() != old_generation) {
    new_type_->mark_classes_used();
    //fprintf(stderr, "%s %s->%s\n", node_->get_description().c_str(), type_out(node_->get_type()).c_str(), type_out(new_type_).c_str());
    on_changed();
  }

  delete new_type_;
}

void NodeRecalc::push_type() {
  types_stack.push_back(TypeData::get_type(tp_Unknown)->clone());
}

TypeData *NodeRecalc::pop_type() {
  TypeData *result = types_stack.back();
  types_stack.pop_back();
  return result;
}
