#include "compiler/data/define-data.h"

DefineData::DefineData() :
  id(),
  val(nullptr),
  type_(def_php) {}

DefineData::DefineData(VertexPtr val, DefineType type_) :
  id(),
  val(val),
  type_(type_) {}
