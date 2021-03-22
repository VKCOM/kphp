// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "function-colors.h"

using namespace function_palette;

const std::map<std::string, color_t> str2color_type = {
  {"*", color_t::any},
  {"highload", color_t::highload},
  {"no-highload", color_t::no_highload},
  {"ssr", color_t::ssr},
  {"ssr-allow-db", color_t::ssr_allow_db},
  {"has-db-access", color_t::has_db_access},
  {"api", color_t::api},
  {"api-callback", color_t::api_callback},
  {"api-allow-curl", color_t::api_allow_curl},
  {"has-curl", color_t::has_curl},
  {"message-internals", color_t::message_internals},
  {"message-module", color_t::message_module},
  {"danger-zone", color_t::danger_zone},
};

color_t function_palette::parse_color(const std::string &str) {
  const auto it = str2color_type.find(str);
  const auto found = it != str2color_type.end();
  return found ? it->second : color_t::none;
}

std::string function_palette::color_to_string(color_t color) {
  for (const auto &pair : str2color_type) {
    if (pair.second == color) {
      return pair.first;
    }
  }
  return "";
}

const ColorContainer::ColorsRaw &ColorContainer::colors() {
  return this->data;
}

void ColorContainer::add(const color_t &color) {
  if (this->count >= static_cast<size_t>(color_t::_count)) {
    return;
  }
  this->data[this->count] = color;
  this->count++;
}

size_t ColorContainer::size() const noexcept {
  return this->count;
}

bool ColorContainer::empty() const noexcept {
  return this->count == 0;
}

color_t ColorContainer::operator[](size_t index) const {
  return this->data[index];
}

std::string ColorContainer::to_string() const {
  std::string desc;

  desc += "colors: {";

  for (int i = 0; i < this->count; ++i) {
    const auto color = this->data[i];
    const auto color_str = color_to_string(color);

    desc += TermStringFormat::paint_green(color_str);

    if (i != this->count - 1) {
      desc += ", ";
    }
  }

  desc += "}";

  return desc;
}

void NodeContainer::add(Node *node) {
  this->count++;
  this->data[static_cast<size_t>(node->color)] = node;
}

Node *NodeContainer::get(color_t color) const {
  return this->data[static_cast<size_t>(color)];
}

bool NodeContainer::has(color_t color) const {
  return this->get(color) != nullptr;
}

void NodeContainer::del(color_t color) {
  this->count--;
  this->data[static_cast<size_t>(color)] = nullptr;
}

size_t NodeContainer::size() const {
  return this->count;
}

bool NodeContainer::empty() const {
  return this->count == 0;
}

Node *NodeContainer::operator[](size_t index) const {
  if (index >= Size) {
    return nullptr;
  }
  return this->data[index];
}
