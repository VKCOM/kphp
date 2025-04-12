// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

#include "compiler/debug.h"

class Key {
  DEBUG_STRING_METHOD { return to_string(); }

private:
  int id{-1};

  explicit Key(int id) :
    id(id) {
  }

public:
  Key() = default;
  Key(const Key &other) = default;
  Key &operator=(const Key &other) = default;

  static Key any_key() { return Key(0); }

  static Key string_key(const std::string &key);
  static Key int_key(int key);

  std::string to_string() const;

  inline bool is_any_key() const { return id == 0; }

  inline bool is_int_key() const { return id > 0 && id % 2 == 1; }

  inline bool is_string_key() const { return id > 0 && id % 2 == 0; }

  friend inline bool operator<(const Key &a, const Key &b);
  friend inline bool operator!=(const Key &a, const Key &b);
  friend inline bool operator==(const Key &a, const Key &b);
};

inline bool operator<(const Key &a, const Key &b) {
  return a.id < b.id;
}

inline bool operator!=(const Key &a, const Key &b) {
  return a.id != b.id;
}

inline bool operator==(const Key &a, const Key &b) {
  return a.id == b.id;
}
