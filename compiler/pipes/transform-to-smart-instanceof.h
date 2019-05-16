#pragma once

#include <set>
#include <string>

#include "compiler/data/data_ptr.h"
#include "compiler/function-pass.h"

class TransformToSmartInstanceof final : public FunctionPassBase {
private:
  struct NewNameAndLeftDerived {
    std::string new_name;
    std::set<ClassPtr> left_derived;
  };

public:
  std::string get_description() final {
    return "Trasform To Smart Instanceof";
  }

  bool user_recursion(VertexPtr v, LocalT *, VisitVertex<TransformToSmartInstanceof> &visit);

  VertexPtr on_enter_vertex(VertexPtr v, FunctionPassBase::LocalT *);

private:
  static VertexAdaptor<op_set> generate_tmp_var_with_instance_cast(VertexPtr instance_var, VertexPtr derived_name_vertex, std::string &new_name);
  static VertexAdaptor<op_instanceof> get_instanceof_from_if(VertexAdaptor<op_if> if_vertex);
  bool fill_derived_classes(VertexPtr instance_var, VertexPtr name_of_derived_vertex, NewNameAndLeftDerived &state);
  void visit_cmd(VisitVertex<TransformToSmartInstanceof> &visit, VertexAdaptor<op_if> if_vertex, VertexPtr name_of_derived_vertex, VertexPtr &cmd);
  static bool is_good_var_for_instanceof(VertexPtr v);

private:
  std::map<std::string, NewNameAndLeftDerived> variable_state;
};
