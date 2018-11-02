#include "compiler/pipes/type-inferer-end.h"

#include "compiler/inferring/public.h"

void TypeInfererEndF::on_finish(DataStream<FunctionAndCFG> &os) {
  tinf::get_inferer()->check_restrictions();
  tinf::get_inferer()->finish();

  for (auto &f_and_cfg : tmp_stream.get_as_vector()) {
    os << f_and_cfg;
  }
}
