// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <map>

#include "compiler/code-gen/writer-data.h"
#include "compiler/inferring/type-data.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class CodeGenerator;

class CodeGenF final : public SyncPipeF<FunctionPtr, WriterData> {
  using need_profiler = std::false_type;

  void prepare_generate_class(ClassPtr klass);
  void prepare_generate_function(FunctionPtr func);
  std::string get_subdir(const std::string &base);
  void write_lib_version(CodeGenerator &W);
  void write_main(CodeGenerator &W);
  size_t calc_count_of_parts(size_t cnt_global_vars);

public:
  void on_finish(DataStream<WriterData> &os) final;
};
