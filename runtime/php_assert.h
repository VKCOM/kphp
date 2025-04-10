// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <cstdio>
#include <unistd.h>

#include "common/mixin/not_copyable.h"
#include "common/wrappers/likely.h"

#include "runtime-common/core/utils/kphp-assert-core.h"

extern int die_on_fail;

extern const char* engine_tag;
extern long long engine_tag_number;

extern const char* engine_pid;

extern int php_warning_level;
extern int php_warning_minimum_level;

void php_out_of_memory_warning(char const* message, ...) __attribute__((format(printf, 1, 2)));

template<class T>
class class_instance;
struct C$Throwable;
const char* php_uncaught_exception_error(const class_instance<C$Throwable>& ex) noexcept;

int64_t f$error_reporting(int64_t level);

int64_t f$error_reporting();
