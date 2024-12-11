// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/file/file-resource.h"
#include "runtime-light/stdlib/file/file-stream-state.h"

inline constexpr int64_t STREAM_CLIENT_CONNECT = 1;
inline constexpr int64_t DEFAULT_SOCKET_TIMEOUT = 60;

resource f$fopen(const string &filename, const string &mode, bool use_include_path = false, const resource &context = mixed());

resource f$stream_socket_client(const string &url, mixed &error_number = FileStreamInstanceState::get().error_number_dummy,
                                mixed &error_description = FileStreamInstanceState::get().error_description_dummy, double timeout = DEFAULT_SOCKET_TIMEOUT,
                                int64_t flags = STREAM_CLIENT_CONNECT, const resource &context = mixed()) noexcept;

task_t<Optional<string>> f$file_get_contents(const string &stream) noexcept;

task_t<Optional<int64_t>> f$fwrite(const resource &stream, const string &text) noexcept;

bool f$fflush(const resource &stream) noexcept;

bool f$fclose(const resource &stream) noexcept;
