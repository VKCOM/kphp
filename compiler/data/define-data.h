#pragma once

#include "compiler/data_ptr.h"

class DefineData {
public:
  enum DefineType {
    def_php,
    def_raw,
    def_var
  };

  int id;

  VertexPtr val;
  string name;
  SrcFilePtr file_id;

  DefineType type_;

  DefineData();
  DefineData(VertexPtr val, DefineType type_);

  inline DefineType &type() { return type_; }

private:
  DISALLOW_COPY_AND_ASSIGN (DefineData);
};
