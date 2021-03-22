// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "function-colors.h"

const std::map<std::string, Color> FunctionColors::str2color_type = {
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

void PaletteNodeContainer::add(PaletteNode* node) {
  this->count++;
  this->data[static_cast<size_t>(node->color)] = node;
}

PaletteNode *PaletteNodeContainer::get(Color color) const {
  return this->data[static_cast<size_t>(color)];
}

bool PaletteNodeContainer::has(Color color) const {
  return this->get(color) != nullptr;
}

void PaletteNodeContainer::del(Color color) {
  this->data[static_cast<size_t>(color)] = nullptr;
}

size_t PaletteNodeContainer::size() const {
  return this->count;
}

bool PaletteNodeContainer::empty() const {
  return this->count == 0;
}

PaletteNode *PaletteNodeContainer::operator[](size_t index) const {
  if (index >= Size) {
    return nullptr;
  }
  return this->data[index];
}
