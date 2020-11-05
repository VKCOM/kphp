// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

class Objs2BinTarget : public Target {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << env->cxx << " -o " << target() << " -Wl,--whole-archive -Wl,--start-group " << dep_list()
       << " -Wl,--end-group -Wl,--no-whole-archive " << env->ld_flags;
    return ss.str();
  }
};
