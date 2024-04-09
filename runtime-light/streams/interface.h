#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"
#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"
#include "runtime-light/core/class_instance/refcountable_php_classes.h"
#include "runtime-light/streams/streams.h"

constexpr int64_t v$COMPONENT_ERROR = -1;

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

task_t<class_instance<C$ComponentQuery>> f$component_client_send_query(const string &name, const string & message);
task_t<string> f$component_client_get_result(class_instance<C$ComponentQuery> query);

task_t<string> f$component_server_get_query();
task_t<void> f$component_server_send_result(const string &message);
