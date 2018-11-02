#include "compiler/inferring/type-node.h"

string tinf::TypeNode::get_description() {
  stringstream ss;
  ss << "as type:" << "  " << type_out(type_);
  return ss.str();
}
