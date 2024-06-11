// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/force-sync.h"

#include "compiler/compiler-core.h"
#include "compiler/inferring/public.h"
#include "compiler/data/class-data.h"
#include "compiler/function-pass.h"

VertexPtr ForceSyncPass::on_enter_vertex(VertexPtr v) {
  return v;
}

bool ForceSyncPass::user_recursion(VertexPtr v) {
  if (auto force_sync_v = v.try_as<op_force_sync>()) {
    inside_force_sync++;
    run_function_pass(force_sync_v->func_call_ref(), this);
    inside_force_sync--;
    return true;
  }

  return false;
}