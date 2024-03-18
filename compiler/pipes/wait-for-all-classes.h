// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class WaitForAllClassesF final : public SyncPipeF<FunctionPtr, FunctionPtr> {
  using Base = SyncPipeF<FunctionPtr, FunctionPtr>;

public:
  bool forward_to_next_pipe(const FunctionPtr &f) final {
    if (f->type == FunctionData::func_local || f->type == FunctionData::func_class_holder) {
      return !f->class_id || !f->class_id->is_trait();
    }
    if (f->type == FunctionData::func_lambda) {
      const FunctionData *p = f->get_this_or_topmost_if_lambda();
      return !p->class_id || !p->class_id->is_trait();
    }
    return true;
  }

  void on_finish(DataStream<FunctionPtr> &os) final;
  void resolve_and_validate_modulites();
};
