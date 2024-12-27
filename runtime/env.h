// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

array<string> f$getenv() noexcept;
Optional<string> f$getenv(const string& varname, bool local_only = false) noexcept;
