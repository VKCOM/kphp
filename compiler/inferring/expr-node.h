#pragma once

#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/node.h"
#include "compiler/stage.h"

namespace tinf {

class ExprNode : public Node {
private:
  VertexPtr expr_;
public:
  explicit ExprNode(VertexPtr expr = VertexPtr()) :
    expr_(expr) {
  }

  void recalc(TypeInferer *inferer);

  VertexPtr get_expr() const {
    return expr_;
  }

  string get_description();
  string get_location_text();
  const Location &get_location();
};

} // namespace tinf
