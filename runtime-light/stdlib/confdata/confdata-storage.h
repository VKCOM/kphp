// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/resource_allocator.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/stdlib/confdata/predefined-wildcards.h"

namespace kphp::confdata {

enum class storage_error : uint8_t { misaligned_buffer, insufficient_buffer, size_overflow, invalid_storage };

struct key_views;

/**
 * A non-owning view of one confdata shared-memory piece.
 *
 * A writer initializes the piece, builds its initial sync sample, and
 * then publishes incremental samples through a 30-slot ring. Readers open the
 * same piece and address the immutable sample named by their reader lease.
 *
 * The shared-memory allocation owns every object reachable through this view.
 * Releasing the whole piece therefore requires no object-by-object teardown.
 */
class storage final : private vk::not_copyable {
  // === TYPES =====================================================================================
  using resource_type = memory_resource::unsynchronized_pool_resource;
  using retired_list = memory_resource::stl::forward_list<mixed, resource_type>;

public:
  using map_type = memory_resource::stl::map<string, mixed, resource_type, stl_string_less>;
  /** Opaque sample token exchanged by the confdata component's reader-lease protocol. */
  using sample_id = uint32_t;

  class editor;

private:
  struct retired_allocations final {
    /** Detached arrays and keys whose child allocations remain owned elsewhere. */
    retired_list m_detached_allocations;
    /** Logical values and nested keys that this generation owned recursively. */
    retired_list m_owned_values;

    explicit retired_allocations(resource_type& resource) noexcept;

    auto empty() const noexcept -> bool;
    auto swap(retired_allocations& other) noexcept -> void;
  };

  struct sample final {
    /** Number of connected readers whose lease names this sample. */
    size_t m_readers{};
    /** Whether this sample was superseded and awaits ordered reclamation. */
    bool m_retired{};
    /** Immutable confdata representation visible to readers of this sample. */
    map_type m_values;
    /** Allocations detached while the following sample was being constructed. */
    retired_allocations m_retired_allocations;

    explicit sample(resource_type& resource) noexcept;
  };

  struct shared_state;

  friend class editor;

  // === MEMBERS ==================================================================================
public:
  static constexpr size_t SAMPLE_COUNT{30};
  static constexpr sample_id INVALID_SAMPLE_ID{std::numeric_limits<sample_id>::max()};

private:
  // Common local view state used in both writer and reader roles.
  /** Header constructed at the beginning of the shared-memory piece. */
  shared_state* m_state{};
  /** Complete logical extent of the shared-memory piece. */
  std::span<std::byte> m_memory;

  // Writer-local state. A reader-side view leaves these members at their defaults.
  /** Writer-local current sample; readers receive their sample ID in the lease. */
  sample_id m_active_sample{INVALID_SAMPLE_ID};
  /** Enforces the single-writer, single-unpublished-update invariant. */
  bool m_update_in_progress{};
  /** Distinguishes a fresh sync piece from an incrementally updated one. */
  bool m_has_committed_sample{};

  // === METHODS ==================================================================================
public:
  // Common layout and view API.
  static constexpr auto memory_alignment() noexcept -> size_t;
  static auto memory_size(size_t memory_limit) noexcept -> std::expected<size_t, storage_error>;
  static auto is_valid_sample_id(sample_id id) noexcept -> bool;

  auto is_initialized() const noexcept -> bool;
  /** Detaches this local view without modifying the shared-memory piece. */
  auto close() noexcept -> void;
  auto wildcards() const noexcept -> const predefined_wildcards&;

  // Writer-side API.
  /** Constructs a new writer-side storage in `memory`. */
  auto init(std::span<std::byte> memory) noexcept -> std::expected<void, storage_error>;
  /** Builds the immutable wildcard index owned by this shared-memory piece. */
  auto initialize_wildcards(std::span<const std::string_view> wildcards) noexcept -> std::expected<void, predefined_wildcards_error>;
  auto memory() const noexcept -> std::span<const std::byte>;
  /** Pins the current sample for a newly connected reader. */
  auto acquire_active_sample() noexcept -> sample_id;
  /** Releases the sample when that reader disconnects. */
  auto release_sample(sample_id id) noexcept -> void;
  /** Starts the initial empty sample used by a sync on a fresh piece. */
  auto start_sync() noexcept -> editor;
  /** Starts an incremental update by copying the current immutable map. */
  auto start_update() noexcept -> std::optional<editor>;

  // Reader-side API.
  /** Opens an initialized reader-side storage. */
  auto open(std::span<const std::byte> memory) noexcept -> std::expected<void, storage_error>;
  auto values(sample_id id) const noexcept -> const map_type&;

private:
  template<std::invocable callback_type>
  requires std::same_as<std::invoke_result_t<callback_type>, void> && std::is_nothrow_invocable_v<callback_type>
  auto with_storage_resource(callback_type&& callback) noexcept -> void;

  auto resource() noexcept -> resource_type&;
  auto begin_update(bool copy_active_sample) noexcept -> std::optional<editor>;
  auto commit(editor& update) noexcept -> void;
  auto cancel(editor& update) noexcept -> void;
  auto reclaim_retired_samples() noexcept -> void;
  auto reclaim_sample(sample_id id) noexcept -> void;
};

inline constexpr auto storage::memory_alignment() noexcept -> size_t {
  return alignof(std::max_align_t);
}

template<std::invocable callback_type>
requires std::same_as<std::invoke_result_t<callback_type>, void> && std::is_nothrow_invocable_v<callback_type>
auto storage::with_storage_resource(callback_type&& callback) noexcept -> void {
  kphp::memory::with_script_memory_resource(resource(), std::forward<callback_type>(callback));
}

inline auto storage::is_initialized() const noexcept -> bool {
  return m_state != nullptr;
}

inline auto storage::memory() const noexcept -> std::span<const std::byte> {
  return m_memory;
}

/**
 * The unpublished working copy of the next sample.
 *
 * Its map and retirement lists allocate from the owning shared-memory piece,
 * while the editor object itself remains local to the writer. Destruction
 * rolls the update back unless `commit()` has published it.
 */
class storage::editor final {
  friend class storage;

  /** Nullable non-owning owner; null after this editor is moved, committed, or cancelled. */
  storage* m_owner{};
  /** Ring slot reserved for this unpublished working copy. */
  sample_id m_destination{INVALID_SAMPLE_ID};
  /** Complete map that will become the destination sample at commit. */
  map_type m_values;
  /** Allocations removed from the current sample while building this map. */
  retired_allocations m_retired_allocations;
  /** Deduplicates the same logical value stored in multiple wildcard sections. */
  mixed m_last_retired_value;
  bool m_changed{};

  editor(storage& owner, sample_id destination, bool copy_active_sample) noexcept;

  auto apply_upsert(std::string_view key, const mixed& value) noexcept -> bool;
  auto apply_erase(std::string_view key) noexcept -> bool;
  auto upsert_one(const key_views& views, const mixed& value) noexcept -> bool;
  auto erase_one(const key_views& views) noexcept -> bool;
  auto retire_value(const mixed& value) noexcept -> void;
  auto retire_for_shallow_destruction(const mixed& value) noexcept -> void;
  auto retire_for_recursive_destruction(const mixed& value) noexcept -> void;

public:
  editor(editor&& other) noexcept;
  editor(const editor&) = delete;
  auto operator=(const editor& other) -> editor& = delete;
  auto operator=(editor&& other) -> editor& = delete;
  ~editor();

  /**
   * Constructs `value_factory()` under this piece's shared allocator and
   * applies the resulting value to every denormalized representation of `key`.
   */
  template<std::invocable value_factory_type>
  requires std::same_as<std::invoke_result_t<value_factory_type>, mixed> && std::is_nothrow_invocable_v<value_factory_type>
  auto upsert(std::string_view key, value_factory_type&& value_factory) noexcept -> bool;
  /** Applies one deletion to every denormalized representation of `key`. */
  auto erase(std::string_view key) noexcept -> bool;
  auto changed() const noexcept -> bool;
  /** Atomically publishes this working copy as the active sample. */
  auto commit() noexcept -> void;
  /** Discards this working copy. Calling this more than once is harmless. */
  auto cancel() noexcept -> void;
};

template<std::invocable value_factory_type>
requires std::same_as<std::invoke_result_t<value_factory_type>, mixed> && std::is_nothrow_invocable_v<value_factory_type>
auto storage::editor::upsert(std::string_view key, value_factory_type&& value_factory) noexcept -> bool {
  kphp::log::assertion(m_owner != nullptr);
  bool changed{};
  m_owner->with_storage_resource([this, key, &value_factory, &changed] noexcept {
    const mixed value{std::invoke(std::forward<value_factory_type>(value_factory))};
    changed = apply_upsert(key, value);
    m_last_retired_value.clear();
  });
  m_changed = changed || m_changed;
  return changed;
}

} // namespace kphp::confdata
