// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/data_ptr.h"
#include "compiler/data/vertex-adaptor.h"
#include "compiler/inferring/node.h"
#include "compiler/inferring/type-data.h"
#include "compiler/inferring/type-inferer.h"

namespace tinf {

tinf::Node* get_tinf_node(VertexPtr vertex);
tinf::Node* get_tinf_node(VarPtr vertex);
tinf::Node* get_tinf_node(FunctionPtr function, int param_i);

tinf::TypeInferer* get_inferer();
const TypeData* get_type(VertexPtr vertex);
const TypeData* get_type(VarPtr var);
const TypeData* get_type(FunctionPtr function, int param_i);

} // namespace tinf
