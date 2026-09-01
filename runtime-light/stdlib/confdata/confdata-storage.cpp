// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/confdata/confdata-storage.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <new>
#include <string_view>
#include <utility>

#include "common/php-functions.h"
#include "runtime-light/stdlib/confdata/confdata-keys.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace {

constexpr uint64_t STORAGE_MAGIC{0x4b32'434f'4e46'4441}; // "K2CONFDA"
constexpr uint32_t STORAGE_VERSION{1};

struct storage_layout final {
  size_t m_pool_offset{};
  size_t m_total_size{};
};

constexpr auto checked_add(size_t lhs, size_t rhs) noexcept -> std::expected<size_t, kphp::confdata::storage_error> {
  if (lhs > std::numeric_limits<size_t>::max() - rhs) [[unlikely]] {
    return std::unexpected{kphp::confdata::storage_error::size_overflow};
  }
  return lhs + rhs;
}

constexpr auto checked_align_up(size_t size) noexcept -> std::expected<size_t, kphp::confdata::storage_error> {
  constexpr auto alignment{kphp::confdata::storage::memory_alignment()};
  static_assert(std::has_single_bit(alignment));

  const auto with_padding{checked_add(size, alignment - 1)};
  if (!with_padding) [[unlikely]] {
    return std::unexpected{with_padding.error()};
  }
  return *with_padding & ~(alignment - 1);
}

constexpr auto calculate_layout(size_t header_size, size_t memory_limit) noexcept -> std::expected<storage_layout, kphp::confdata::storage_error> {
  const auto pool_offset{checked_align_up(header_size)};
  if (!pool_offset) [[unlikely]] {
    return std::unexpected{pool_offset.error()};
  }
  const auto total_size{checked_add(*pool_offset, memory_limit)};
  if (!total_size) [[unlikely]] {
    return std::unexpected{total_size.error()};
  }
  return storage_layout{.m_pool_offset = *pool_offset, .m_total_size = *total_size};
}

auto is_same_allocation(const mixed& lhs, const mixed& rhs) noexcept -> bool {
  if (lhs.get_type() != rhs.get_type()) {
    return false;
  }
  if (lhs.is_string()) {
    return lhs.as_string().c_str() == rhs.as_string().c_str();
  }
  if (lhs.is_array()) {
    return lhs.as_array().is_equal_inner_pointer(rhs.as_array());
  }
  return false;
}

auto mark_string_as_confdata(string& value) noexcept -> void {
  kphp::log::assertion(!value.is_reference_counter(ExtraRefCnt::for_instance_cache));
  if (!value.is_reference_counter(ExtraRefCnt::for_confdata) && !value.is_reference_counter(ExtraRefCnt::for_global_const)) {
    value.set_reference_counter_to(ExtraRefCnt::for_confdata);
  }
}

auto mark_value_as_confdata(mixed& value) noexcept -> void {
  kphp::log::assertion(!value.is_reference_counter(ExtraRefCnt::for_instance_cache));
  if (value.is_reference_counter(ExtraRefCnt::for_global_const) || value.is_reference_counter(ExtraRefCnt::for_confdata)) {
    return;
  }
  if (value.is_string()) {
    mark_string_as_confdata(value.as_string());
    return;
  }
  if (!value.is_array()) {
    return;
  }

  auto& array{value.as_array()};
  array.set_reference_counter_to(ExtraRefCnt::for_confdata);
  for (auto it{array.begin_no_mutate()}, last{array.end_no_mutate()}; it != last; ++it) {
    if (it.is_string_key()) {
      mark_string_as_confdata(it.get_string_key());
    }
    mark_value_as_confdata(it.get_value());
  }
}

auto recursively_destroy_value(mixed& value) noexcept -> void {
  if (value.is_reference_counter(ExtraRefCnt::for_global_const)) {
    return;
  }
  if (value.is_array()) {
    auto& array{value.as_array()};
    for (auto it{array.begin_no_mutate()}, last{array.end_no_mutate()}; it != last; ++it) {
      if (it.is_string_key() && !it.get_string_key().is_reference_counter(ExtraRefCnt::for_global_const)) {
        it.get_string_key().force_destroy(ExtraRefCnt::for_confdata);
      }
      recursively_destroy_value(it.get_value());
    }
  } else if (!value.is_string()) {
    return;
  }
  value.force_destroy(ExtraRefCnt::for_confdata);
}

} // namespace

namespace kphp::confdata {

struct storage::shared_state final {
  /** Identifies a K2 confdata piece rather than unrelated shared memory. */
  uint64_t m_magic{STORAGE_MAGIC};
  /** Rejects pieces created for a different in-memory layout. */
  uint32_t m_version{STORAGE_VERSION};
  /** Logical initialized size; K2 may expose larger page-aligned capacity. */
  size_t m_total_size{};
  /** Byte offset at which allocator-managed payload memory begins. */
  size_t m_pool_offset{};
  /** Allocator shared by wildcard indexes, sample maps, and PHP values. */
  resource_type m_resource{};
  /** Immutable wildcard index shared by every sample in this piece. */
  predefined_wildcards m_wildcards;
  /** Thirty immutable generations, matching legacy KPHP's backpressure bound. */
  std::array<sample, SAMPLE_COUNT> m_samples;

private:
  template<size_t... indexes>
  static auto make_samples_impl(resource_type& resource, std::index_sequence<indexes...> /*unused*/) noexcept -> std::array<sample, SAMPLE_COUNT> {
    static_assert(sizeof...(indexes) == SAMPLE_COUNT);
    return {((void)indexes, sample{resource})...};
  }

  static auto make_samples(resource_type& resource) noexcept -> std::array<sample, SAMPLE_COUNT> {
    return make_samples_impl(resource, std::make_index_sequence<SAMPLE_COUNT>{});
  }

public:
  shared_state() noexcept
      : m_wildcards{m_resource},
        m_samples{make_samples(m_resource)} {}
};

storage::retired_allocations::retired_allocations(resource_type& resource) noexcept
    : m_detached_allocations{retired_list::allocator_type{resource}},
      m_owned_values{retired_list::allocator_type{resource}} {}

auto storage::retired_allocations::empty() const noexcept -> bool {
  return m_detached_allocations.empty() && m_owned_values.empty();
}

auto storage::retired_allocations::swap(retired_allocations& other) noexcept -> void {
  m_detached_allocations.swap(other.m_detached_allocations);
  m_owned_values.swap(other.m_owned_values);
}

storage::sample::sample(resource_type& resource) noexcept
    : m_values{map_type::allocator_type{resource}},
      m_retired_allocations{resource} {}

storage::editor::editor(storage& owner, sample_id destination, bool copy_active_sample) noexcept
    : m_owner{std::addressof(owner)},
      m_destination{destination},
      m_values{map_type::allocator_type{owner.resource()}},
      m_retired_allocations{owner.resource()} {
  if (copy_active_sample) {
    owner.with_storage_resource([this, &owner] noexcept { m_values = owner.m_state->m_samples[owner.m_active_sample].m_values; });
  }
}

storage::editor::editor(editor&& other) noexcept
    : m_owner{std::exchange(other.m_owner, nullptr)},
      m_destination{std::exchange(other.m_destination, INVALID_SAMPLE_ID)},
      m_values{std::move(other.m_values)},
      m_retired_allocations{std::move(other.m_retired_allocations)},
      m_last_retired_value{std::move(other.m_last_retired_value)},
      m_changed{other.m_changed} {}

storage::editor::~editor() {
  cancel();
}

auto storage::editor::erase(std::string_view key) noexcept -> bool {
  kphp::log::assertion(m_owner != nullptr);
  bool erased{};
  m_owner->with_storage_resource([this, key, &erased] noexcept {
    erased = apply_erase(key);
    m_last_retired_value.clear();
  });
  m_changed = erased || m_changed;
  return erased;
}

auto storage::editor::changed() const noexcept -> bool {
  return m_changed;
}

auto storage::editor::commit() noexcept -> void {
  kphp::log::assertion(m_owner != nullptr);
  m_owner->commit(*this);
}

auto storage::editor::cancel() noexcept -> void {
  if (m_owner != nullptr) {
    m_owner->cancel(*this);
  }
}

auto storage::editor::apply_upsert(std::string_view key, const mixed& value) noexcept -> bool {
  const auto implicit_views{split_key(key)};
  if (!implicit_views) [[unlikely]] {
    return false;
  }

  bool changed{};
  const bool has_predefined_wildcard{
      m_owner->m_state->m_wildcards.for_each_matching_wildcard(key, [this, key, &value, &changed](std::string_view wildcard) noexcept {
        const auto views{split_key_with_predefined_wildcard(key, wildcard.size())};
        kphp::log::assertion(views.has_value());
        changed = upsert_one(*views, value) || changed;
      })};

  if (!has_predefined_wildcard || implicit_views->kind() != section_kind::simple_key) {
    changed = upsert_one(*implicit_views, value) || changed;
    if (implicit_views->kind() == section_kind::two_dots_wildcard) {
      const auto one_dot_views{implicit_views->reinterpret_two_dots_as_one_dot()};
      kphp::log::assertion(one_dot_views.has_value());
      changed = upsert_one(*one_dot_views, value) || changed;
    }
  }
  return changed;
}

auto storage::editor::apply_erase(std::string_view key) noexcept -> bool {
  const auto implicit_views{split_key(key)};
  if (!implicit_views) [[unlikely]] {
    return false;
  }

  bool erased{};
  const bool has_predefined_wildcard{m_owner->m_state->m_wildcards.for_each_matching_wildcard(key, [this, key, &erased](std::string_view wildcard) noexcept {
    const auto views{split_key_with_predefined_wildcard(key, wildcard.size())};
    kphp::log::assertion(views.has_value());
    erased = erase_one(*views) || erased;
  })};

  if (!has_predefined_wildcard || implicit_views->kind() != section_kind::simple_key) {
    erased = erase_one(*implicit_views) || erased;
    if (implicit_views->kind() == section_kind::two_dots_wildcard) {
      const auto one_dot_views{implicit_views->reinterpret_two_dots_as_one_dot()};
      kphp::log::assertion(one_dot_views.has_value());
      erased = erase_one(*one_dot_views) || erased;
    }
  }
  return erased;
}

auto storage::editor::upsert_one(const key_views& views, const mixed& value) noexcept -> bool {
  key_handles handles{views};
  auto section_it{m_values.find(handles.section())};

  if (section_it == m_values.end()) {
    if (views.kind() == section_kind::simple_key) {
      m_values.emplace(handles.make_section_copy(), value);
    } else {
      array<mixed> entries{};
      entries.set_value(handles.make_remainder_copy(), value);
      m_values.emplace(handles.make_section_copy(), mixed{std::move(entries)});
    }
    return true;
  }

  if (views.kind() == section_kind::simple_key) {
    if (equals(section_it->second, value)) {
      return false;
    }
    retire_value(section_it->second);
    section_it->second = value;
    return true;
  }

  kphp::log::assertion(section_it->second.is_array());
  auto& entries{section_it->second.as_array()};
  const auto* previous{entries.find_value(handles.remainder())};
  if (previous != nullptr && equals(*previous, value)) {
    return false;
  }

  retire_for_shallow_destruction(mixed{entries});
  if (previous == nullptr) {
    entries.set_value(handles.make_remainder_copy(), value);
  } else {
    retire_value(*previous);
    entries.mutate_if_shared();
    auto entry_it{entries.find_no_mutate(handles.remainder())};
    kphp::log::assertion(entry_it != entries.end());
    entry_it.get_value() = value;
  }
  return true;
}

auto storage::editor::erase_one(const key_views& views) noexcept -> bool {
  key_handles handles{views};
  auto section_it{m_values.find(handles.section())};
  if (section_it == m_values.end()) {
    return false;
  }

  if (views.kind() == section_kind::simple_key) {
    retire_value(section_it->second);
    retire_for_shallow_destruction(mixed{section_it->first});
    m_values.erase(section_it);
    return true;
  }

  kphp::log::assertion(section_it->second.is_array());
  auto& entries{section_it->second.as_array()};
  if (!entries.has_key(handles.remainder())) {
    return false;
  }

  retire_for_shallow_destruction(mixed{entries});
  entries.mutate_if_shared();
  auto entry_it{entries.find_no_mutate(handles.remainder())};
  kphp::log::assertion(entry_it != entries.end());
  if (entry_it.is_string_key()) {
    retire_for_recursive_destruction(mixed{entry_it.get_string_key()});
  }
  retire_value(entry_it.get_value());
  entries.unset(handles.remainder());

  if (entries.empty()) {
    retire_for_shallow_destruction(mixed{section_it->first});
    m_values.erase(section_it);
  }
  return true;
}

auto storage::editor::retire_value(const mixed& value) noexcept -> void {
  if ((!value.is_string() && !value.is_array()) ||
      (!value.is_reference_counter(ExtraRefCnt::for_confdata) && !value.is_reference_counter(ExtraRefCnt::for_global_const))) {
    return;
  }
  if (!m_last_retired_value.is_null()) {
    kphp::log::assertion(is_same_allocation(m_last_retired_value, value));
    return;
  }
  retire_for_recursive_destruction(value);
  m_last_retired_value = value;
}

auto storage::editor::retire_for_shallow_destruction(const mixed& value) noexcept -> void {
  if ((value.is_string() || value.is_array()) && value.is_reference_counter(ExtraRefCnt::for_confdata)) {
    m_retired_allocations.m_detached_allocations.emplace_front(value);
  }
}

auto storage::editor::retire_for_recursive_destruction(const mixed& value) noexcept -> void {
  if ((value.is_string() || value.is_array()) && value.is_reference_counter(ExtraRefCnt::for_confdata)) {
    m_retired_allocations.m_owned_values.emplace_front(value);
  }
}

auto storage::memory_size(size_t memory_limit) noexcept -> std::expected<size_t, storage_error> {
  static_assert(alignof(shared_state) <= memory_alignment());
  if (memory_limit == 0) [[unlikely]] {
    return std::unexpected{storage_error::insufficient_buffer};
  }
  const auto layout{calculate_layout(sizeof(shared_state), memory_limit)};
  if (!layout) [[unlikely]] {
    return std::unexpected{layout.error()};
  }
  return layout->m_total_size;
}

auto storage::is_valid_sample_id(sample_id id) noexcept -> bool {
  return id < SAMPLE_COUNT;
}

auto storage::init(std::span<std::byte> memory) noexcept -> std::expected<void, storage_error> {
  kphp::log::assertion(!is_initialized());
  if (reinterpret_cast<uintptr_t>(memory.data()) % alignof(shared_state) != 0) [[unlikely]] {
    return std::unexpected{storage_error::misaligned_buffer};
  }
  const auto layout{calculate_layout(sizeof(shared_state), 1)};
  if (!layout) [[unlikely]] {
    return std::unexpected{layout.error()};
  }
  if (memory.size() < layout->m_total_size) [[unlikely]] {
    return std::unexpected{storage_error::insufficient_buffer};
  }

  m_memory = memory;
  m_state = std::construct_at(reinterpret_cast<shared_state*>(m_memory.data()));
  m_active_sample = 0;
  m_state->m_total_size = memory.size();
  m_state->m_pool_offset = layout->m_pool_offset;
  auto pool_memory{memory.subspan(m_state->m_pool_offset)};
  m_state->m_resource.init(pool_memory.data(), pool_memory.size());
  return {};
}

auto storage::open(std::span<const std::byte> memory) noexcept -> std::expected<void, storage_error> {
  kphp::log::assertion(!is_initialized());
  if (reinterpret_cast<uintptr_t>(memory.data()) % alignof(shared_state) != 0) [[unlikely]] {
    return std::unexpected{storage_error::misaligned_buffer};
  }
  if (memory.size() <= sizeof(shared_state)) [[unlikely]] {
    return std::unexpected{storage_error::insufficient_buffer};
  }

  const auto* state{std::launder(reinterpret_cast<const shared_state*>(memory.data()))};
  const auto layout{calculate_layout(sizeof(shared_state), 1)};
  if (!layout || state->m_magic != STORAGE_MAGIC || state->m_version != STORAGE_VERSION || state->m_total_size > memory.size() ||
      state->m_pool_offset != layout->m_pool_offset || state->m_pool_offset >= state->m_total_size) [[unlikely]] {
    return std::unexpected{storage_error::invalid_storage};
  }

  // K2 may report page-aligned physical capacity. Only this logical prefix
  // was initialized by the writer and belongs to the storage.
  m_memory = {const_cast<std::byte*>(memory.data()), state->m_total_size};
  m_state = const_cast<shared_state*>(state);
  m_has_committed_sample = true;
  return {};
}

auto storage::close() noexcept -> void {
  kphp::log::assertion(is_initialized());
  // Reader-side views never start updates and therefore leave this writer-only
  // invariant false. For a writer, it prevents detaching while an editor lives.
  kphp::log::assertion(!m_update_in_progress);
  m_state = nullptr;
  m_memory = {};
  m_active_sample = INVALID_SAMPLE_ID;
  m_has_committed_sample = false;
}

auto storage::initialize_wildcards(std::span<const std::string_view> wildcards) noexcept -> std::expected<void, predefined_wildcards_error> {
  kphp::log::assertion(is_initialized());
  kphp::log::assertion(!m_has_committed_sample);
  std::expected<void, predefined_wildcards_error> result{};
  with_storage_resource([this, wildcards, &result] noexcept { result = m_state->m_wildcards.initialize(wildcards); });
  return result;
}

auto storage::wildcards() const noexcept -> const predefined_wildcards& {
  kphp::log::assertion(is_initialized());
  return m_state->m_wildcards;
}

auto storage::acquire_active_sample() noexcept -> sample_id {
  kphp::log::assertion(is_initialized());
  kphp::log::assertion(is_valid_sample_id(m_active_sample));
  const auto id{m_active_sample};
  ++m_state->m_samples[id].m_readers;
  return id;
}

auto storage::release_sample(sample_id id) noexcept -> void {
  kphp::log::assertion(is_initialized());
  kphp::log::assertion(is_valid_sample_id(id));
  auto& readers{m_state->m_samples[id].m_readers};
  kphp::log::assertion(readers != 0);
  --readers;
}

auto storage::values(sample_id id) const noexcept -> const map_type& {
  kphp::log::assertion(is_initialized());
  kphp::log::assertion(is_valid_sample_id(id));
  return m_state->m_samples[id].m_values;
}

auto storage::start_sync() noexcept -> editor {
  kphp::log::assertion(is_initialized());
  kphp::log::assertion(is_valid_sample_id(m_active_sample));
  kphp::log::assertion(!m_has_committed_sample);
  auto update{begin_update(false)};
  kphp::log::assertion(update.has_value());
  return std::move(*update);
}

auto storage::start_update() noexcept -> std::optional<editor> {
  kphp::log::assertion(is_initialized());
  kphp::log::assertion(is_valid_sample_id(m_active_sample));
  kphp::log::assertion(m_has_committed_sample);
  return begin_update(true);
}

auto storage::resource() noexcept -> resource_type& {
  kphp::log::assertion(is_initialized());
  return m_state->m_resource;
}

auto storage::begin_update(bool copy_active_sample) noexcept -> std::optional<editor> {
  kphp::log::assertion(!m_update_in_progress);
  kphp::log::assertion(is_valid_sample_id(m_active_sample));
  reclaim_retired_samples();

  const auto destination{static_cast<sample_id>((m_active_sample + 1) % SAMPLE_COUNT)};
  const auto& sample{m_state->m_samples[destination]};
  if (sample.m_readers != 0 || sample.m_retired) {
    return std::nullopt;
  }

  kphp::log::assertion(sample.m_values.empty());
  kphp::log::assertion(sample.m_retired_allocations.empty());
  m_update_in_progress = true;
  return editor{*this, destination, copy_active_sample};
}

auto storage::commit(editor& update) noexcept -> void {
  kphp::log::assertion(m_update_in_progress);
  kphp::log::assertion(update.m_owner == this);
  kphp::log::assertion(is_valid_sample_id(update.m_destination));
  kphp::log::assertion(update.m_last_retired_value.is_null());

  with_storage_resource([this, &update] noexcept {
    for (auto& [section, value] : update.m_values) {
      // The map key is const, but a copied handle updates the shared string header.
      string mutable_section{section};
      mark_string_as_confdata(mutable_section);
      mark_value_as_confdata(value);
    }

    auto& destination{m_state->m_samples[update.m_destination]};
    kphp::log::assertion(destination.m_readers == 0);
    kphp::log::assertion(!destination.m_retired);
    kphp::log::assertion(destination.m_values.empty());
    kphp::log::assertion(destination.m_retired_allocations.empty());
    destination.m_values = std::move(update.m_values);

    auto& previous{m_state->m_samples[m_active_sample]};
    kphp::log::assertion(previous.m_retired_allocations.empty());
    previous.m_retired_allocations.swap(update.m_retired_allocations);
    previous.m_retired = true;
    m_active_sample = update.m_destination;
  });

  update.m_owner = nullptr;
  update.m_destination = INVALID_SAMPLE_ID;
  m_update_in_progress = false;
  m_has_committed_sample = true;
}

auto storage::cancel(editor& update) noexcept -> void {
  kphp::log::assertion(m_update_in_progress);
  kphp::log::assertion(update.m_owner == this);
  with_storage_resource([&update] noexcept {
    update.m_last_retired_value.clear();
    update.m_values.clear();
    update.m_retired_allocations.m_detached_allocations.clear();
    update.m_retired_allocations.m_owned_values.clear();
  });
  update.m_owner = nullptr;
  update.m_destination = INVALID_SAMPLE_ID;
  m_update_in_progress = false;
}

auto storage::reclaim_retired_samples() noexcept -> void {
  kphp::log::assertion(is_valid_sample_id(m_active_sample));
  with_storage_resource([this] noexcept {
    const auto active{m_active_sample};
    for (auto id{static_cast<sample_id>((active + 1) % SAMPLE_COUNT)}; id != active; id = static_cast<sample_id>((id + 1) % SAMPLE_COUNT)) {
      const auto& sample{m_state->m_samples[id]};
      if (!sample.m_retired) {
        continue;
      }
      // Neighboring generations can share payloads. Stop at the oldest pinned
      // sample rather than reclaiming a newer sample out of order.
      if (sample.m_readers != 0) {
        break;
      }
      reclaim_sample(id);
    }
  });
}

auto storage::reclaim_sample(sample_id id) noexcept -> void {
  auto& sample{m_state->m_samples[id]};
  sample.m_values.clear();

  // Destroy detached parents first. Their element destructors are no-ops for
  // `for_confdata` handles, after which recursively owned roots are safe.
  while (!sample.m_retired_allocations.m_detached_allocations.empty()) {
    sample.m_retired_allocations.m_detached_allocations.front().force_destroy(ExtraRefCnt::for_confdata);
    sample.m_retired_allocations.m_detached_allocations.pop_front();
  }
  while (!sample.m_retired_allocations.m_owned_values.empty()) {
    recursively_destroy_value(sample.m_retired_allocations.m_owned_values.front());
    sample.m_retired_allocations.m_owned_values.pop_front();
  }
  sample.m_retired = false;
}

} // namespace kphp::confdata
