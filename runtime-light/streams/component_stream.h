#pragma once

#include "runtime-light/core/class_instance/refcountable_php_classes.h"
#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"
#include "runtime-light/streams/streams.h"

struct C$ComponentStream final : public refcountable_php_classes<C$ComponentStream> {
  uint64_t stream_d {};

  const char *get_class() const noexcept {
    return "ComponentStream";
  }

  int32_t get_hash() const noexcept {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(C$ComponentStream::get_class())));
  }

  ~C$ComponentStream() {
    free_descriptor(stream_d);
  }
};

struct C$ComponentQuery final : public refcountable_php_classes<C$ComponentQuery> {
  uint64_t stream_d {};

  const char *get_class() const noexcept {
    return "ComponentQuery";
  }

  int32_t get_hash() const noexcept {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(C$ComponentQuery::get_class())));
  }

  ~C$ComponentQuery() {
    free_descriptor(stream_d);
  }
};

bool f$ComponentStream$$is_read_closed(const class_instance<C$ComponentStream> & stream);
bool f$ComponentStream$$is_write_closed(const class_instance<C$ComponentStream> & stream);
bool f$ComponentStream$$is_please_shutdown_write(const class_instance<C$ComponentStream> & stream);

void f$ComponentStream$$close(const class_instance<C$ComponentStream> & stream);
void f$ComponentStream$$shutdown_write(const class_instance<C$ComponentStream> & stream);
void f$ComponentStream$$please_shutdown_write(const class_instance<C$ComponentStream> & stream);
