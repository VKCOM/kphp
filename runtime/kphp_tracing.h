// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <functional>

namespace kphp_tracing {

using on_wait_start_callback_t = std::function<void()>;
using on_fork_state_change_callback_t = std::function<void(int64_t)>;
using on_fork_switch_callback_t = std::function<void(int64_t, int64_t)>;

extern on_wait_start_callback_t on_wait_start_callback;
extern on_fork_state_change_callback_t on_wait_finish_callback;
extern on_fork_state_change_callback_t on_fork_start;
extern on_fork_state_change_callback_t on_fork_finish;
extern on_fork_switch_callback_t on_fork_switch_callback;

void clear_callbacks();

}

// TODO: ensure this callbacks never swaps context
void f$kphp_tracing_register_on_wait_callbacks(const kphp_tracing::on_wait_start_callback_t &on_start,
                                               const kphp_tracing::on_fork_state_change_callback_t &on_finish);
void f$kphp_tracing_register_on_fork_switch_callback(const kphp_tracing::on_fork_switch_callback_t &callback);
void f$kphp_tracing_register_on_fork_callbacks(const kphp_tracing::on_fork_state_change_callback_t &on_start,
                                               const kphp_tracing::on_fork_state_change_callback_t &on_finish);
