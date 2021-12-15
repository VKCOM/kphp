// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/pdo/pdo_statement.h"

bool f$PDOStatement$$execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params) noexcept {
  return v$this.get()->statement->execute(v$this, params);
}

mixed f$PDOStatement$$fetch(const class_instance<C$PDOStatement> &v$this, int mode, int cursorOrientation, int cursorOffset) noexcept {
  return v$this.get()->statement->fetch(v$this, mode, cursorOrientation, cursorOffset);
}
