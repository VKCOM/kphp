// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/pdo/pdo_statement.h"
#include "runtime/resumable.h"

bool f$PDOStatement$$execute(const class_instance<C$PDOStatement> &v$this, const Optional<array<mixed>> &params) noexcept {
  // Могу ли я сделать вот эту функцию прерываемой и вызывать её как обычную отсюда, как сейчас?
  return v$this.get()->statement->execute(v$this, params);
  // Если я хочу чтобы эта штука была прерываемой, мне нужно делать так:
  // return start_resumable<bool>(new pdo_execute_resumable(request_id, timeout));
}

mixed f$PDOStatement$$fetch(const class_instance<C$PDOStatement> &v$this, int mode, int cursorOrientation, int cursorOffset) noexcept {
  return v$this.get()->statement->fetch(v$this, mode, cursorOrientation, cursorOffset);
}
