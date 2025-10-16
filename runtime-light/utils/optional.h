// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

template<typename Opt, typename Func>
void apply_if_has_value(Opt&& opt, Func&& fn) noexcept(noexcept(fn(*opt))) {
  if (opt.has_value()) {
    fn(*opt);
  }
}
