// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

class Objs2ObjTarget : public Target {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << settings->incremental_linker.get() <<
       " " << settings->incremental_linker_flags.get() <<
       " -o " << target() <<
       " " << dep_list();
    return ss.str();
  }
};
