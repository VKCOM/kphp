// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class ConvertLocalPhpdocsPass final : public FunctionPassBase {
  VertexPtr visit_phpdoc_and_extract_vars(VertexAdaptor<op_phpdoc_raw> phpdoc_raw);

public:
  string get_description() override {
    return "Analyze local phpdocs";
  }

  VertexPtr on_enter_vertex(VertexPtr root) override;
};
