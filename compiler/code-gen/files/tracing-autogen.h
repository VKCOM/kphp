// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <mutex>
#include <vector>

#include "compiler/code-gen/code-gen-root-cmd.h"

class CodeGenerator;
class KphpTracingDeclarationMixin;

struct TracingAutogen : CodeGenRootCmd {
  void compile(CodeGenerator &W) const final;

  static void register_function_marked_kphp_tracing(FunctionPtr f);
  static void finished_appending_and_prepare();
  static bool empty() { return all_with_aggregate.empty(); }

  static void codegen_runtime_func_guard_declaration(CodeGenerator &W, FunctionPtr f);
  static void codegen_runtime_func_guard_start(CodeGenerator &W, FunctionPtr f);

private:
  static std::vector<FunctionPtr> all_with_aggregate;
  static std::vector<vk::string_view> all_aggregate_names;
  static std::mutex mu;
};
