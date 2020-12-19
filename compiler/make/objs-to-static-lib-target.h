// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

class Objs2StaticLibTarget : public Target {
public:
  string get_cmd() final {
    std::stringstream ss;
    ss << settings->archive_creator.get() << " rcs " << target() << " " << dep_list();
    return ss.str();
  }
};
