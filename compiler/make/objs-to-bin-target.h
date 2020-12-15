// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

class Objs2BinTarget : public Target {
public:
  string get_cmd() final {
#if defined(__APPLE__)
    vk::string_view open_dep{" -Wl,-all_load "};
    vk::string_view close_dep;
#else
    vk::string_view open_dep{" -Wl,--whole-archive -Wl,--start-group "};
    vk::string_view close_dep{" -Wl,--end-group -Wl,--no-whole-archive "};
#endif
    std::stringstream ss;
    ss << env->cxx << " -o " << target() << open_dep << dep_list() << close_dep << env->ld_flags;
    return ss.str();
  }
};
