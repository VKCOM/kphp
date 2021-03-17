// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "function-colors.h"

const std::map<std::string, Color> Colors::str2color_type = {
  {"*",                 Color::any},
  {"highload",          Color::highload},
  {"no-highload",       Color::no_highload},
  {"ssr",               Color::ssr},
  {"ssr-allow-db",      Color::ssr_allow_db},
  {"has-db-access",     Color::has_db_access},
  {"api",               Color::api},
  {"api-callback",      Color::api_callback},
  {"api-allow-curl",    Color::api_allow_curl},
  {"has-curl",          Color::has_curl},
  {"message-internals", Color::message_internals},
  {"message-module",    Color::message_module},
  {"danger-zone",       Color::danger_zone},
};
