// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/node.h"
#include "compiler/stage.h"

namespace tinf {

class ExprNode : public Node {
private:
  VertexPtr expr_;

  static std::string convert_expr_to_human_readable(VertexPtr expr);

public:
  explicit ExprNode(VertexPtr expr) :
    expr_(expr) {
  }

 void copy_type_from(const TypeData *from) {
    type_ = from;
    recalc_state_ = recalc_st_waiting | recalc_bit_at_least_once;
  }

  void recalc(TypeInferer *inferer);

  VertexPtr get_expr() const {
    return expr_;
  }

  std::string get_description() final;
  std::string get_expr_human_readable() const;
  const Location &get_location() const final;
};

} // namespace tinf
