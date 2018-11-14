#pragma once

#include "compiler/const-manipulations.h"
#include "compiler/vertex.h"

class RegisterDefinesF {
private:
  set<string*> in_progress;
  vector<string*> stack;

  DataStream<FunctionPtr> all_fun;

  CheckConstWithDefines check_const;
  MakeConst make_const;

  void process_define_recursive(VertexPtr root);
  void process_define(DefinePtr def);

  void print_error_infinite_define(DefinePtr cur_def);

public:

  RegisterDefinesF();

  void execute(FunctionPtr function, DataStream<FunctionPtr> &os __attribute__((unused)));

  void on_finish(DataStream<FunctionPtr> &os);
};
