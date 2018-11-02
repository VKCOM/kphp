#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/inferring/node.h"
#include "compiler/inferring/type-inferer.h"
#include "compiler/inferring/type-data.h"

namespace tinf {

const TypeData *fast_get_type(VertexPtr vertex);
const TypeData *fast_get_type(VarPtr var);
const TypeData *fast_get_type(FunctionPtr function, int id);
tinf::Node *get_tinf_node(VertexPtr vertex);
tinf::Node *get_tinf_node(VarPtr var);
tinf::Node *get_tinf_node(FunctionPtr function, int id);

void register_inferer(tinf::TypeInferer *inferer);
tinf::TypeInferer *get_inferer();
const TypeData *get_type(VertexPtr vertex);
const TypeData *get_type(VarPtr var);
const TypeData *get_type(FunctionPtr function, int id);

}
