// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

array<string> f$getenv() noexcept;
Optional<string> f$getenv(const string &varname, bool local_only = false) noexcept;

#include "env_putenv.h"
