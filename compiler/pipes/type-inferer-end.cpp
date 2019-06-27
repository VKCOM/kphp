#include "compiler/pipes/type-inferer-end.h"

#include "compiler/data/class-data.h"
#include "compiler/data/function-data.h"
#include "compiler/inferring/public.h"

void TypeInfererEndF::on_finish(DataStream<FunctionAndCFG> &os) {
  tinf::get_inferer()->check_restrictions();
  tinf::get_inferer()->finish();
  stage::die_if_global_errors();
  for (auto &el : this->tmp_stream.get_as_vector()) {
    FunctionPtr f = el.function;
    if (f->type != FunctionData::func_class_holder || f->class_id->really_used) {
      os << std::move(el);
    }
  }
}
