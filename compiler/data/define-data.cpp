// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/data/define-data.h"

#include "compiler/data/class-data.h"
#include "compiler/data/src-file.h"

DefineData::DefineData()
    : id(),
      type_(def_unknown),
      val(nullptr) {}

DefineData::DefineData(std::string name, VertexPtr val, DefineType type_)
    : id(),
      type_(type_),
      val(val),
      name(std::move(name)) {}

std::string DefineData::as_human_readable() const {
  return class_id ? class_id->as_human_readable() + "::" + get_local_name_from_global_$$(name) : name;
}

bool DefineData::is_builtin() const {
  return file_id && file_id->is_builtin();
}
