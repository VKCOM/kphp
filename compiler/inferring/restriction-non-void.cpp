#include "compiler/inferring/restriction-non-void.h"

RestrictionNonVoid::RestrictionNonVoid(tinf::Node *node) : node(node) {}

bool RestrictionNonVoid::check_broken_restriction_impl() {
  return node->get_type()->ptype() == tp_void;
}

const char *RestrictionNonVoid::get_description() {
  desc = "Expression " + node->get_description() + " is not allowed to be void";
  return desc.c_str();
}
