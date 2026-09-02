// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/components/confdata/confdata-proxy/sync-functions.h"
#include "runtime-light/components/confdata/state/component-state.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/confdata/confdata-storage.h"
#include "runtime-light/stdlib/diagnostics/contextual-tags.h"
#include "runtime-light/streams/stream.h"

struct InstanceState final : vk::not_copyable {
  // === TYPES ====================================================================================
  enum class warmup_status : uint8_t { pending, done };

  struct confdata_sync_error final {
    enum class stage : uint8_t {
      memory_size,
      shared_memory_allocation,
      storage_initialization,
      wildcard_initialization,
      oom_threshold,
      synchronization,
      shared_memory_publication
    };

    /** Step that failed while preparing and publishing the replacement piece. */
    stage m_stage;
    /** Error code produced by that step's underlying API. */
    int32_t m_code;
  };

private:
  class confdata_piece;
  using confdata_piece_list = kphp::stl::list<confdata_piece, kphp::memory::script_allocator>;

  class confdata_piece final {
    class creation_token final {
      friend class confdata_piece;

      creation_token() noexcept = default;
    };

    /** K2 allocation owned and eventually released wholesale by this piece. */
    void* m_memory{};
    /** Number of reader sessions that still refer to this piece. */
    size_t m_readers{};
    /** Non-owning writer view over the allocation. */
    kphp::confdata::storage m_storage;

  public:
    /** Public for allocator-aware container construction; only `create()` can provide the token. */
    confdata_piece(const creation_token& /* token */, void* memory) noexcept;
    ~confdata_piece();

    confdata_piece(const confdata_piece&) = delete;
    confdata_piece(confdata_piece&&) = delete;
    auto operator=(const confdata_piece&) -> confdata_piece& = delete;
    auto operator=(confdata_piece&&) -> confdata_piece& = delete;

    static auto create(confdata_piece_list& owner, size_t memory_limit, size_t oom_handling_size,
                       std::span<const std::string_view> predefined_wildcards) noexcept -> std::expected<confdata_piece_list::iterator, confdata_sync_error>;

    auto storage() noexcept -> kphp::confdata::storage&;
    auto acquire_active_sample() noexcept -> kphp::confdata::storage::sample_id;
    auto release_sample(kphp::confdata::storage::sample_id sample_id) noexcept -> void;
    auto has_readers() const noexcept -> bool;
  };

  class reader_session;

  // === MEMBERS ==================================================================================
  const ComponentState& m_component_state{ComponentState::get()};

public:
  AllocatorState m_allocator_state{m_component_state.m_initial_instance_memory_size, m_component_state.m_min_instance_extra_memory_size, 0};
  warmup_status m_warmup_status{warmup_status::pending};
  kphp::confdata::pagination m_pagination{};

private:
  /** Owns retired pieces still used by readers followed by the current piece. */
  confdata_piece_list m_confdata_pieces;

public:
  kphp::log::contextual_tags m_instance_tags{};
  kphp::coro::instance_state m_coroutine_instance_state;
  kphp::coro::io_scheduler m_io_scheduler{m_coroutine_instance_state};

  // === METHODS ==================================================================================
  InstanceState() noexcept = default;
  static auto get() noexcept -> InstanceState&;

  auto init() noexcept -> void;

private:
  auto release_reader(confdata_piece_list::iterator piece_it, kphp::confdata::storage::sample_id sample_id) noexcept -> void;
  auto erase_if_retired_and_unused(confdata_piece_list::iterator piece_it) noexcept -> void;

  auto run() noexcept -> kphp::coro::task<>;
  auto accept_loop() noexcept -> kphp::coro::task<>;
  auto serve_reader_lease(kphp::component::stream reader_stream) noexcept -> kphp::coro::task<>;

  auto service_loop() noexcept -> kphp::coro::task<>;
  auto perform_sync(std::string_view confdata_proxy_actor) noexcept -> kphp::coro::task<std::expected<void, confdata_sync_error>>;
  auto try_apply_events(kphp::confdata::storage& storage, kphp::confdata::storage::editor& editor,
                        std::span<const tl::confdata::KeyValuePair> events) noexcept -> bool;
  auto apply_incremental_events(std::span<const tl::confdata::KeyValuePair> events) noexcept -> bool;
};

inline auto InstanceState::get() noexcept -> InstanceState& {
  return *k2::instance_state();
}
