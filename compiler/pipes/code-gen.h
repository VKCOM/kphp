// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "compiler/code-gen/code-gen-root-cmd.h"
#include "compiler/code-gen/writer-data.h"
#include "compiler/pipes/sync.h"
#include "compiler/threading/data-stream.h"

// this pipe launches all needed codegen commands
// all these commands are launches in "just calc hashes" mode (without storing cpp/h contents in memory) — to detect diff
// output of this pipe are commands that emerged diff in files since previous kphp launch
class CodeGenF final : public SyncPipeF<FunctionPtr, std::unique_ptr<CodeGenRootCmd>> {
  using need_profiler = std::true_type;
  using Base = SyncPipeF<FunctionPtr, WriterData>;

  void prepare_generate_function(FunctionPtr func);
  std::string calc_subdir_for_function(FunctionPtr func);
  std::string shorten_occurence_of_class_in_file_name(ClassPtr occuring_class, const std::string& file_name);

public:
  void execute(FunctionPtr function, DataStream<std::unique_ptr<CodeGenRootCmd>>& unused_os) final;
  void on_finish(DataStream<std::unique_ptr<CodeGenRootCmd>>& os) final;
};

// this pipe re-launches codegen commands that differ from previous kphp launch
// now they are launches not as "just calc hashes", but as "store all cpp/h contents and php comments"
class CodeGenForDiffF {
public:
  void execute(std::unique_ptr<CodeGenRootCmd> cmd, DataStream<WriterData*>& os);
};
