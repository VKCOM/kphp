// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/inferring/node.h"
#include "compiler/inferring/rvalue.h"

class NodeRecalc {
protected:
  TypeData* new_type_;
  tinf::Node* node_;
  tinf::TypeInferer* inferer_;

public:
  const TypeData* new_type();
  void add_dependency_impl(tinf::Node* from, tinf::Node* to, const MultiKey* from_at);
  void add_dependency(const RValue& rvalue);
  void set_lca_at(const MultiKey* key, const RValue& rvalue);
  void set_lca_at(const MultiKey* key, VertexPtr expr);
  void set_lca_at(const MultiKey* key, PrimitiveType ptype);
  void set_lca(const RValue& rvalue);
  void set_lca(PrimitiveType ptype);
  void set_lca(FunctionPtr function, int id);
  void set_lca(VertexPtr vertex, const MultiKey* key = nullptr);
  void set_lca(const TypeData* type, const MultiKey* key = nullptr);
  void set_lca(VarPtr var);
  void set_lca(ClassPtr klass);
  NodeRecalc(tinf::Node* node, tinf::TypeInferer* inferer);

  virtual ~NodeRecalc() = default;
  NodeRecalc(const NodeRecalc&) = delete;
  NodeRecalc& operator=(const NodeRecalc&) = delete;

  void on_changed();
  virtual bool auto_edge_flag();

  virtual void do_recalc() = 0;

  virtual void on_new_type_became_tpError(const TypeData* because_of_type, const RValue& because_of_rvalue);

  void run();
};
