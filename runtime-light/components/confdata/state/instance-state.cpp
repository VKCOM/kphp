// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/components/confdata/state/instance-state.h"

#include <chrono>
#include <cstddef>
#include <expected>
#include <format>
#include <iterator>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/stdlib/serialization/json-functions.h"
#include "runtime-common/stdlib/serialization/serialize-functions.h"
#include "runtime-light/components/confdata/confdata-proxy/sync-functions.h"
#include "runtime-light/components/confdata/confdata-proxy/tl.h"
#include "runtime-light/components/confdata/state/component-state.h"
#include "runtime-light/coroutine/event.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/when-all.h"
#include "runtime-light/stdlib/confdata/confdata-constants.h"
#include "runtime-light/stdlib/confdata/confdata-reader-lease.h"
#include "runtime-light/stdlib/confdata/confdata-storage.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/streams/connection.h"
#include "runtime-light/streams/stream.h"

namespace {

constexpr auto CONFDATA_RETRY_INTERVAL{std::chrono::seconds{1}};

// Event decoding stays component-local: only the writer sees serialization
// flags, while readers consume the already-decoded shared `mixed` values.
auto decode_value(const tl::confdata::keyValuePair& event) noexcept -> mixed {
  if (event.is_php_serialized.value && event.is_json_serialized.value) [[unlikely]] {
    kphp::log::warning("confdata value has both php_serialized and json_serialized flags set: key -> {}", event.key.value);
    return {};
  }
  if (event.is_php_serialized.value) {
    return unserialize_raw(event.value.value.data(), static_cast<string::size_type>(event.value.value.size()));
  } else if (event.is_json_serialized.value) {
    return json_decode(event.value.value).value_or(mixed{});
  }
  return string{event.value.value.data(), static_cast<string::size_type>(event.value.value.size())};
}

auto report_reached_oom_threshold(const kphp::confdata::storage& storage) noexcept -> bool {
  if (!storage.is_oom_threshold_reached()) {
    return false;
  }
  const auto usage{storage.memory_usage()};
  kphp::log::warning("confdata shared-memory OOM threshold reached: used -> {}, threshold -> {}, capacity -> {}", usage.m_used, usage.m_oom_threshold,
                     usage.m_capacity);
  return true;
}

} // namespace

template<>
struct std::formatter<InstanceState::confdata_sync_error> {
  template<typename parse_context_type>
  constexpr auto parse(parse_context_type& ctx) const noexcept {
    return ctx.begin();
  }

  template<typename format_context_type>
  auto format(const InstanceState::confdata_sync_error& error, format_context_type& ctx) const noexcept {
    using stage = InstanceState::confdata_sync_error::stage;

    std::string_view stage_name{"unknown"};
    switch (error.m_stage) {
    case stage::memory_size:
      stage_name = "memory size calculation";
      break;
    case stage::shared_memory_allocation:
      stage_name = "shared memory allocation";
      break;
    case stage::storage_initialization:
      stage_name = "storage initialization";
      break;
    case stage::wildcard_initialization:
      stage_name = "predefined wildcard initialization";
      break;
    case stage::oom_threshold:
      stage_name = "OOM threshold check";
      break;
    case stage::synchronization:
      stage_name = "clean synchronization";
      break;
    case stage::shared_memory_publication:
      stage_name = "shared memory publication";
      break;
    }
    return std::format_to(ctx.out(), "{}: error -> {}", stage_name, error.m_code);
  }
};

class InstanceState::reader_session final {
  /** Owner that removes a retired piece after this reader disconnects. */
  InstanceState& m_instance_state;
  /** Stable iterator to the registry node containing this session's sample. */
  confdata_piece_list::iterator m_piece_it;
  /** Ring sample pinned in the piece referenced by `m_piece_it`. */
  kphp::confdata::storage::sample_id m_sample_id;

public:
  reader_session(InstanceState& instance_state, confdata_piece_list::iterator piece_it) noexcept
      : m_instance_state{instance_state},
        m_piece_it{piece_it},
        m_sample_id{m_piece_it->acquire_active_sample()} {}

  ~reader_session() {
    m_instance_state.release_reader(m_piece_it, m_sample_id);
  }

  reader_session(const reader_session&) = delete;
  reader_session(reader_session&&) = delete;
  auto operator=(const reader_session&) -> reader_session& = delete;
  auto operator=(reader_session&&) -> reader_session& = delete;

  auto sample_id() const noexcept -> kphp::confdata::storage::sample_id {
    return m_sample_id;
  }
};

InstanceState::confdata_piece::confdata_piece(const creation_token& /* token */, void* memory) noexcept
    : m_memory{memory} {}

InstanceState::confdata_piece::~confdata_piece() {
  if (m_storage.is_initialized()) {
    m_storage.close();
  }
  // TODO:
  // if (const auto released{k2::free_shared_memory(m_memory)}; !released) [[unlikely]] {
  //   kphp::log::warning("failed to free confdata shared memory: error -> {}", released.error());
  // }
}

auto InstanceState::confdata_piece::create(confdata_piece_list& owner, size_t memory_limit, size_t oom_handling_size,
                                           std::span<const std::string_view> predefined_wildcards) noexcept
    -> std::expected<confdata_piece_list::iterator, confdata_sync_error> {
  kphp::log::assertion(owner.empty());

  const auto shared_memory_size{kphp::confdata::storage::memory_size(memory_limit)};
  if (!shared_memory_size) [[unlikely]] {
    return std::unexpected{confdata_sync_error{.m_stage = confdata_sync_error::stage::memory_size,
                                               .m_code = static_cast<int32_t>(std::to_underlying(shared_memory_size.error()))}};
  }

  const auto shared_memory{k2::alloc_shared_memory(*shared_memory_size, kphp::confdata::storage::memory_alignment())};
  if (!shared_memory) [[unlikely]] {
    return std::unexpected{confdata_sync_error{.m_stage = confdata_sync_error::stage::shared_memory_allocation, .m_code = shared_memory.error()}};
  }

  const auto piece_it{owner.emplace(owner.end(), creation_token{}, *shared_memory)};
  if (const auto initialized{piece_it->m_storage.init({static_cast<std::byte*>(*shared_memory), *shared_memory_size}, oom_handling_size)}; !initialized)
      [[unlikely]] {
    const confdata_sync_error error{.m_stage = confdata_sync_error::stage::storage_initialization,
                                    .m_code = static_cast<int32_t>(std::to_underlying(initialized.error()))};
    owner.erase(piece_it);
    return std::unexpected{error};
  }
  if (const auto initialized{piece_it->m_storage.initialize_wildcards(predefined_wildcards)}; !initialized) [[unlikely]] {
    const confdata_sync_error error{.m_stage = confdata_sync_error::stage::wildcard_initialization,
                                    .m_code = static_cast<int32_t>(std::to_underlying(initialized.error()))};
    owner.erase(piece_it);
    return std::unexpected{error};
  }
  if (piece_it->m_storage.is_oom_threshold_reached()) [[unlikely]] {
    const confdata_sync_error error{.m_stage = confdata_sync_error::stage::oom_threshold, .m_code = k2::errno_enomem};
    owner.erase(piece_it);
    return std::unexpected{error};
  }
  return piece_it;
}

auto InstanceState::confdata_piece::storage() noexcept -> kphp::confdata::storage& {
  return m_storage;
}

auto InstanceState::confdata_piece::acquire_active_sample() noexcept -> kphp::confdata::storage::sample_id {
  ++m_readers;
  return m_storage.acquire_active_sample();
}

auto InstanceState::confdata_piece::release_sample(kphp::confdata::storage::sample_id sample_id) noexcept -> void {
  kphp::log::assertion(m_readers != 0);
  m_storage.release_sample(sample_id);
  --m_readers;
}

auto InstanceState::confdata_piece::has_readers() const noexcept -> bool {
  return m_readers != 0;
}

auto InstanceState::release_reader(confdata_piece_list::iterator piece_it, kphp::confdata::storage::sample_id sample_id) noexcept -> void {
  piece_it->release_sample(sample_id);
  erase_if_retired_and_unused(piece_it);
}

auto InstanceState::erase_if_retired_and_unused(confdata_piece_list::iterator piece_it) noexcept -> void {
  kphp::log::assertion(!m_confdata_pieces.empty());
  kphp::log::assertion(piece_it != m_confdata_pieces.end());
  if (piece_it == std::prev(m_confdata_pieces.end()) || piece_it->has_readers()) {
    return;
  }
  m_confdata_pieces.erase(piece_it);
}

auto InstanceState::init() noexcept -> void {
  auto main_task{run()};
  // initialize async stack
  auto& main_task_async_stack_frame{main_task.get_handle().promise().get_async_stack_frame()};
  main_task_async_stack_frame.async_stack_root = std::addressof(m_coroutine_instance_state.coroutine_stack_root);
  m_coroutine_instance_state.coroutine_stack_root.top_async_stack_frame = std::addressof(main_task_async_stack_frame);
  // spawn main task onto the scheduler
  kphp::log::assertion(m_io_scheduler.spawn(std::move(main_task)));
}

auto InstanceState::run() noexcept -> kphp::coro::task<> {
  co_await kphp::coro::when_all(service_loop(), accept_loop()); // both never return
  kphp::log::assertion(false);
}

auto InstanceState::accept_loop() noexcept -> kphp::coro::task<> {
  for (;;) {
    auto opt_stream{co_await kphp::component::stream::accept()};
    if (!opt_stream.has_value()) [[unlikely]] {
      continue;
    }

    auto stream{std::move(*opt_stream)};
    kphp::log::debug("accepted a stream: descriptor -> {}", stream.descriptor());
    if (!m_io_scheduler.spawn(serve_reader_lease(std::move(stream)))) [[unlikely]] {
      kphp::log::warning("failed to serve a confdata reader lease");
    }
  }
}

auto InstanceState::serve_reader_lease(kphp::component::stream reader_stream) noexcept -> kphp::coro::task<> {
  auto expected_connection{kphp::component::connection::from_stream(std::move(reader_stream))};
  if (!expected_connection) [[unlikely]] {
    co_return kphp::log::warning("failed to create a confdata reader connection: error -> {}", expected_connection.error());
  }

  if (m_confdata_pieces.empty()) [[unlikely]] {
    co_return kphp::log::warning("can't serve a confdata reader lease: can't find confdata piece");
  }

  auto connection{*std::move(expected_connection)};
  reader_session session{*this, std::prev(m_confdata_pieces.end())};
  const auto lease{kphp::confdata::reader_lease::create(kphp::confdata::SHARED_MEMORY_NAME, session.sample_id())};
  kphp::log::assertion(lease.has_value());
  if (const auto written{co_await connection.get_stream().write_all(std::as_bytes(std::span{std::addressof(*lease), 1}))}; !written) [[unlikely]] {
    kphp::log::warning("failed to write a confdata reader lease: error -> {}", written.error());
    co_return;
  }

  kphp::coro::event reader_disconnected{};
  if (const auto registered{connection.register_abort_handler([&reader_disconnected] noexcept { reader_disconnected.set(); })}; !registered) [[unlikely]] {
    co_return kphp::log::warning("failed to watch a confdata reader connection: error -> {}", registered.error());
  }
  co_await reader_disconnected;
}

auto InstanceState::perform_sync(std::string_view confdata_proxy_actor) noexcept -> kphp::coro::task<std::expected<void, confdata_sync_error>> {
  // A separate one-node list owns the unpublished piece and later permits a
  // zero-allocation transfer into the registry.
  confdata_piece_list pending_piece{};
  const auto created_piece{confdata_piece::create(pending_piece, m_component_state.m_confdata_memory_limit, m_component_state.m_confdata_oom_handling_size,
                                                  m_component_state.m_predefined_wildcards)};
  if (!created_piece) [[unlikely]] {
    co_return std::unexpected{created_piece.error()};
  }
  auto& piece{**created_piece};

  auto sync_editor{piece.storage().start_sync()};
  auto sync_result{co_await kphp::confdata::sync(confdata_proxy_actor,
                                                 [this, &storage = piece.storage(), &sync_editor](std::span<const tl::confdata::KeyValuePair> events) noexcept {
                                                   return try_apply_events(storage, sync_editor, events);
                                                 })};
  if (!sync_result) [[unlikely]] {
    sync_editor.cancel();
    co_return std::unexpected{
        confdata_sync_error{.m_stage = confdata_sync_error::stage::synchronization, .m_code = static_cast<int32_t>(std::to_underlying(sync_result.error()))}};
  }
  if (report_reached_oom_threshold(piece.storage())) [[unlikely]] {
    sync_editor.cancel();
    co_return std::unexpected{confdata_sync_error{.m_stage = confdata_sync_error::stage::oom_threshold, .m_code = k2::errno_enomem}};
  }

  sync_editor.commit();
  // Existing readers keep their mapped allocation; future lookups of the
  // stable name resolve to this newly published piece.
  if (const auto published{k2::publish_shared_memory(kphp::confdata::SHARED_MEMORY_NAME, piece.storage().memory().data(), 0, true, true)}; !published)
      [[unlikely]] {
    co_return std::unexpected{
        confdata_sync_error{.m_stage = confdata_sync_error::stage::shared_memory_publication, .m_code = static_cast<int32_t>(published.error())}};
  }
  // Only successfully synchronized and published pieces enter the registry,
  // so its last element is always the current piece.
  const auto retired_piece_it{m_confdata_pieces.empty() ? m_confdata_pieces.end() : std::prev(m_confdata_pieces.end())};
  m_confdata_pieces.splice(m_confdata_pieces.end(), pending_piece);
  if (retired_piece_it != m_confdata_pieces.end()) {
    erase_if_retired_and_unused(retired_piece_it);
  }
  m_pagination = *std::move(sync_result);
  co_return std::expected<void, confdata_sync_error>{};
}

auto InstanceState::service_loop() noexcept -> kphp::coro::task<> {
  const std::string_view confdata_proxy_actor{ComponentState::get().m_confdata_proxy_actor_name};

  for (;;) {
    if (!m_pagination.m_has_synced) {
      const auto sync_result{co_await perform_sync(confdata_proxy_actor)};
      if (!sync_result) [[unlikely]] {
        kphp::log::warning("failed to prepare a synchronized confdata shared-memory piece: {}; retrying", sync_result.error());
        co_await m_io_scheduler.schedule(CONFDATA_RETRY_INTERVAL);
        continue;
      }
      m_warmup_status = InstanceState::warmup_status::done;
    }

    auto update{co_await kphp::confdata::update(
        confdata_proxy_actor, m_pagination, [this](std::span<const tl::confdata::KeyValuePair> events) noexcept { return apply_incremental_events(events); })};
    // update returns only on error; m_pagination was advanced in place up to the last applied batch
    kphp::log::assertion(!update.has_value());
    switch (update.error()) {
    case kphp::confdata::subscribe_error::old_offset:
    case kphp::confdata::subscribe_error::not_synced:
      // local version is too old: clean re-sync required
      kphp::log::warning("confdata update failed: error -> {}, resyncing", std::to_underlying(update.error()));
      m_pagination = {};
      break;
    case kphp::confdata::subscribe_error::transport:
    case kphp::confdata::subscribe_error::malformed_response:
      // pagination is still valid; the longpoll resumes from the last applied position
      kphp::log::warning("confdata update failed: error -> {}, retrying", std::to_underlying(update.error()));
      break;
    case kphp::confdata::subscribe_error::batch_rejected:
      // The rejected batch did not advance pagination or alter the active
      // sample. Reset pagination so the next iteration takes the clean-sync
      // path while the current piece keeps serving its readers.
      kphp::log::warning("the current confdata shared-memory piece can't accept an update; resyncing");
      m_pagination = {};
      break;
    }
    co_await m_io_scheduler.schedule(CONFDATA_RETRY_INTERVAL);
  }
}

auto InstanceState::try_apply_events(kphp::confdata::storage& storage, kphp::confdata::storage::editor& editor,
                                     std::span<const tl::confdata::KeyValuePair> events) noexcept -> bool {
  if (report_reached_oom_threshold(storage)) [[unlikely]] {
    return false;
  }
  for (const auto& wrapped_event : events) {
    const auto& event{wrapped_event.inner};
    if (event.key.value.size() > kphp::confdata::MAX_KEY_LENGTH) [[unlikely]] {
      kphp::log::warning("confdata event key is too long and was ignored: size -> {}", event.key.value.size());
      continue;
    }
    if (event.value.value.empty()) {
      static_cast<void>(editor.erase(event.key.value));
    } else {
      static_cast<void>(editor.upsert(event.key.value, [&event] noexcept { return decode_value(event); }));
    }
    if (report_reached_oom_threshold(storage)) [[unlikely]] {
      return false;
    }
  }
  return true;
}

auto InstanceState::apply_incremental_events(std::span<const tl::confdata::KeyValuePair> events) noexcept -> bool {
  kphp::log::assertion(!m_confdata_pieces.empty());
  auto& storage{m_confdata_pieces.back().storage()};
  if (report_reached_oom_threshold(storage)) [[unlikely]] {
    return false;
  }
  auto editor{storage.start_update()};
  if (!editor) [[unlikely]] {
    return false;
  }
  if (!try_apply_events(storage, *editor, events)) [[unlikely]] {
    return false;
  }
  if (editor->changed()) {
    editor->commit();
  }
  return true;
}
