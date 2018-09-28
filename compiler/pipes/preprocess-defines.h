#pragma once

#include "compiler/const-manipulations.h"
#include "compiler/vertex.h"

class PreprocessDefinesF {
private:
  set<string> in_progress;
  set<string> done;
  map<string, VertexPtr> define_vertex;
  vector<string> stack;

  DataStream<VertexPtr> defines_stream;
  DataStream<FunctionPtr> all_fun;

  CheckConstWithDefines check_const;
  MakeConst make_const;

  void process_define_recursive(VertexPtr root);
  void process_define(VertexPtr root);

public:

  PreprocessDefinesF();

  string get_description() {
    return "Process defines concatenation";
  }

  void execute(FunctionPtr function, DataStream<FunctionPtr> &os __attribute__((unused)));

  void on_finish(DataStream<FunctionPtr> &os);

};

