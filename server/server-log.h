// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdio>

#include "common/wrappers/string_view.h"

enum class ServerLog { Critical = -1, Error = -2, Warning = -3 };

// This api should be used only from the master process or from the worker processes net coroutine context
void write_log_server_impl(ServerLog type, const char *format, ...) noexcept __attribute__((format(printf, 2, 3)));
void write_log_server_impl(ServerLog type, vk::string_view msg) noexcept;

#define log_server_critical(...) write_log_server_impl(ServerLog::Critical, __VA_ARGS__)
#define log_server_error(...) write_log_server_impl(ServerLog::Error, __VA_ARGS__)
#define log_server_warning(...) write_log_server_impl(ServerLog::Warning, __VA_ARGS__)
