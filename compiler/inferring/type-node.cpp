#include "compiler/inferring/type-node.h"
#include "compiler/stage.h"

string tinf::TypeNode::get_location_text() {
  return stage::to_str(location_);
}

string tinf::TypeNode::get_description() {
  stringstream ss;
  ss << "as type:" << "  " << "(casted to) " << colored_type_out(type_) << "  " << "at " + get_location_text();
  return ss.str();
}
