// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/function-pass.h"

class GenTreePostprocessPass final : public FunctionPassBase {
  struct builtin_fun {
    Operation op;
    int args;
  };

  builtin_fun get_builtin_function(const std::string &name);

public:
  string get_description() override {
    return "Gen tree postprocess";
  }
  VertexPtr on_enter_vertex(VertexPtr root) override;
  VertexPtr on_exit_vertex(VertexPtr root) override;

  // converts the spread operator (...$a) to a call to the array_merge_spread function
  static VertexPtr convert_array_with_spread_operators(VertexAdaptor<op_array> array_vertex);

  // converts the match($a) { ... } to a rvalue statements list
  // For example:
  //   $a = match($b) {
  //      10 => 1,
  //      20 => 2,
  //      default => 3,
  //   }
  // Converts to something like this:
  //   $a = ({
  //      switch ($b) {
  //         case 10: {
  //            $tmp = 1; break;
  //         }
  //         case 20: {
  //            $tmp = 2; break;
  //         }
  //         default: {
  //            $tmp = 3; break;
  //         }
  //      };
  //      $tmp;
  //  });
  //
  // If there are several conditions, then the code is generated only once
  // and all conditions fallthrough to this code:
  //
  // For example:
  //   $a = match($b) {
  //      10, 20 => 1,
  //      default => 3,
  //   }
  // Converts to something like this:
  //   $a = ({
  //      switch ($b) {
  //         case 10: {} // fallthrough
  //         case 20: {
  //            $tmp = 1; break;
  //         }
  //         default: {
  //            $tmp = 3; break;
  //         }
  //      };
  //      $tmp;
  //  });
  //
  // If there is no default branch, then the default version is generated.
  // The default version always gives a warning about an unhandled value.
  // For example:
  //   $a = match($b) {
  //      10 => 1,
  //   }
  // Converts to something like this:
  //   $a = ({
  //      switch ($b) {
  //         case 10: {
  //            $tmp = 1; break;
  //         }
  //         default: {
  //            warning("Unhandled match value '$b'"); break;
  //         }
  //      };
  //      $tmp;
  //  });
  static VertexPtr convert_match_to_switch(VertexAdaptor<op_match> &match);
};
