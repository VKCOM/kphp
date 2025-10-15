// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

namespace kphp::coro {

enum class await_set_waiting_policy : uint8_t { suspend_on_empty, resume_on_empty };
enum class await_set_next_result_t : uint8_t { stopped, empty };
enum class await_set_push_result_t : uint8_t { ok, stopped };

template<typename return_type, await_set_waiting_policy waiting_policy>
class await_set;

} // namespace kphp::coro
