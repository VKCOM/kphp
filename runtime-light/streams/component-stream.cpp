// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <memory>

#include "runtime-light/component/component.h"
#include "runtime-light/header.h"
#include "runtime-light/streams/component-stream.h"
#include "runtime-light/utils/context.h"

const char *C$ComponentStream::get_class() const noexcept {
  return "ComponentStream";
}

int32_t C$ComponentStream::get_hash() const noexcept {
  return static_cast<int32_t>(vk::std_hash(vk::string_view(C$ComponentStream::get_class())));
}

C$ComponentStream::~C$ComponentStream() {
  auto &component_ctx{*get_component_context()};
  if (component_ctx.opened_streams().contains(stream_d)) {
    component_ctx.release_stream(stream_d);
  }
}

bool f$ComponentStream$$is_read_closed(const class_instance<C$ComponentStream> &stream) {
  StreamStatus status{};
  if (const auto status_res{get_platform_context()->get_stream_status(stream.get()->stream_d, std::addressof(status))};
      status_res != GetStatusResult::GetStatusOk) {
    php_warning("stream status error %d", status_res);
    return true;
  }
  return status.read_status == IOStatus::IOClosed;
}

bool f$ComponentStream$$is_write_closed(const class_instance<C$ComponentStream> &stream) {
  StreamStatus status{};
  if (const auto status_res{get_platform_context()->get_stream_status(stream.get()->stream_d, std::addressof(status))};
      status_res != GetStatusResult::GetStatusOk) {
    php_warning("stream status error %d", status_res);
    return true;
  }
  return status.write_status == IOStatus::IOClosed;
}

bool f$ComponentStream$$is_please_shutdown_write(const class_instance<C$ComponentStream> &stream) {
  StreamStatus status{};
  if (const auto status_res{get_platform_context()->get_stream_status(stream.get()->stream_d, std::addressof(status))};
      status_res != GetStatusResult::GetStatusOk) {
    php_warning("stream status error %d", status_res);
    return true;
  }
  return status.please_shutdown_write;
}

void f$ComponentStream$$close(const class_instance<C$ComponentStream> &stream) {
  get_component_context()->release_stream(stream->stream_d);
}

void f$ComponentStream$$shutdown_write(const class_instance<C$ComponentStream> &stream) {
  get_platform_context()->shutdown_write(stream->stream_d);
}

void f$ComponentStream$$please_shutdown_write(const class_instance<C$ComponentStream> &stream) {
  get_platform_context()->please_shutdown_write(stream->stream_d);
}

// ================================================================================================

const char *C$ComponentQuery::get_class() const noexcept {
  return "ComponentQuery";
}

int32_t C$ComponentQuery::get_hash() const noexcept {
  return static_cast<int32_t>(vk::std_hash(vk::string_view(C$ComponentQuery::get_class())));
}

C$ComponentQuery::~C$ComponentQuery() {
  auto &component_ctx{*get_component_context()};
  if (component_ctx.opened_streams().contains(stream_d)) {
    component_ctx.release_stream(stream_d);
  }
}
