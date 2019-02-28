#pragma once

#include "compiler/io.h"
#include "compiler/threading/data-stream.h"

class CodeGenerator;

class CodeGenF {
  DataStream<FunctionPtr> tmp_stream;
  map<string, size_t> subdir_hash;
  void prepare_generate_class(ClassPtr klass);
  void prepare_generate_function(FunctionPtr func);
  string get_subdir(const string &base);
  void recalc_hash_of_subdirectory(const string &subdir, const string &file_name);
  void write_hashes_of_subdirs_to_dep_files(CodeGenerator &W);
  void write_tl_schema(CodeGenerator &W);
  void write_lib_version(CodeGenerator &W);
  size_t calc_count_of_parts(size_t cnt_global_vars);

public:
  CodeGenF() {
    tmp_stream.set_sink(true);
  }

  void execute(FunctionPtr input, DataStream<WriterData *> &os __attribute__((unused))) {
    tmp_stream << input;
  }

  void on_finish(DataStream<WriterData *> &os);
};
