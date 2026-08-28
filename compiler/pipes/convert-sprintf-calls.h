// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/compiler-core.h"
#include "compiler/function-pass.h"

struct FormatCallInfo;
struct FormatPart;

// This pipe rewrites some sprintf calls.
//
// If the format string contains only simple specifiers %s and %d,
// then such a call is replaced with concatenation.
//
// For example:
//   echo sprintf("Hello %s", $name);
// converted to:
//   echo "Hello " . $name;
//
// Depending on the length of the string and count specifiers, the speed
// of concatenation is several times faster. Even for the given example,
// the speed of sprintf is 4 times slower than that of concatenation.
//
// This optimization was taken out of the optimization pipe in order to
// place it before the pipe, where the constants are moved into variables,
// so that the constant strings created in this pipe are moved into variables.
class ConvertSprintfCallsPass final : public FunctionPassBase {
public:
  std::string get_description() final {
    return "Convert sprintf calls";
  }

  VertexPtr on_exit_vertex(VertexPtr root) override;

private:
  static VertexPtr convert_sprintf_call(VertexAdaptor<op_func_call> call);
  static VertexPtr convert_format_part_to_vertex(const FormatPart& part, size_t arg_index, const FormatCallInfo& info, const Location& call_location);
};
