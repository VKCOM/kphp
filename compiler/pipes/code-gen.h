#pragma once

#include <map>

#include "compiler/code-gen/writer-data.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

class CodeGenerator;

class CodeGenF final : public SyncPipeF<FunctionPtr, WriterData> {
  std::map<std::string, size_t> subdir_hash;
  void prepare_generate_class(ClassPtr klass);
  void prepare_generate_function(FunctionPtr func);
  std::string get_subdir(const std::string &base);
  void recalc_hash_of_subdirectory(const std::string &subdir, const std::string &file_name);
  void write_hashes_of_subdirs_to_dep_files(CodeGenerator &W);
  void write_tl_schema(CodeGenerator &W);
  void write_lib_version(CodeGenerator &W);
  size_t calc_count_of_parts(size_t cnt_global_vars);

public:

  void on_finish(DataStream<WriterData> &os) final;
};
