// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/pipes/type-inferer.h"

#include "compiler/inferring/public.h"

void TypeInfererF::execute(FunctionAndCFG input, DataStream<FunctionAndCFG>& os) {
  os << input;
}

void TypeInfererF::on_finish(DataStream<FunctionAndCFG>&) {
  std::vector<Task*> tasks = tinf::get_inferer()->get_tasks();
  std::for_each(tasks.begin(), tasks.end(), register_async_task);
}
