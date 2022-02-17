// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/pdo/pdo_statement.h"

bool f$PDOStatement$$execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params) noexcept {
  // TODO: prepared statements are not implemented so far. This is stub for future implementation
  return v$this.get()->statement->execute(v$this, params);
}

mixed f$PDOStatement$$fetch(const class_instance<C$PDOStatement> &v$this) noexcept {
  if (v$this.is_null()) {
    return false;
  }
  return v$this.get()->statement->fetch(v$this);
}

array<mixed> f$PDOStatement$$fetchAll(const class_instance<C$PDOStatement> &v$this) noexcept {
  array<mixed> res;
  mixed row;
  while (f$boolval(row = f$PDOStatement$$fetch(v$this))) {
    res.emplace_back(std::move(row));
  };
  return res;
}
