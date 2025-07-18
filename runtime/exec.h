// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/system/system-functions.h"

int64_t& get_dummy_result_code() noexcept;

Optional<string> f$exec(const string& command);
Optional<string> f$exec(const string& command, mixed& output, int64_t& result_code = get_dummy_result_code());
int64_t f$system(const string& command, int64_t& result_code = get_dummy_result_code());
Optional<bool> f$passthru(const string& command, int64_t& result_code = get_dummy_result_code());
