#include "compiler/data/define-data.h"

DefineData::DefineData() :
  id(),
  type_(def_unknown),
  val(nullptr) {}

DefineData::DefineData(VertexPtr val, DefineType type_) :
  id(),
  type_(type_),
  val(val) {}
