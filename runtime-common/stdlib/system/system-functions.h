// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

string f$escapeshellarg(const string& arg) noexcept;

string f$escapeshellcmd(const string& cmd) noexcept;

inline bool f$extension_loaded(const string& /*extension*/) noexcept {
  return true;
}
