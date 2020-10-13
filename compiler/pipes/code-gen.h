#pragma once

#include <map>

#include "compiler/code-gen/writer-data.h"
#include "compiler/inferring/type-data.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class CodeGenerator;

class CodeGenF final : public SyncPipeF<FunctionPtr, WriterData> {
  std::vector<const TypeData *> forkable_types;
  std::vector<const TypeData *> waitable_types;
  std::once_flag dest_dir_synced;

  void prepare_generate_class(ClassPtr klass);
  void prepare_generate_function(FunctionPtr func);
  std::string get_subdir(const std::string &base);
  void write_lib_version(CodeGenerator &W);
  size_t calc_count_of_parts(size_t cnt_global_vars);

public:

  void execute(FunctionPtr function, DataStream<WriterData> &os) final;
  void on_finish(DataStream<WriterData> &os) final;
};
