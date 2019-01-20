#include "compiler/pipes/type-inferer.h"

#include "compiler/inferring/public.h"

void TypeInfererF::execute(FunctionAndCFG input, DataStream<FunctionAndCFG> &os) {
  os << input;
}

void TypeInfererF::on_finish(DataStream<FunctionAndCFG> &) {
  vector<Task *> tasks = tinf::get_inferer()->get_tasks();
  for (int i = 0; i < (int)tasks.size(); i++) {
    register_async_task(tasks[i]);
  }
}
