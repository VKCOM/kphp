#pragma once

#include <tuple>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"

namespace SharedData {
struct WebServerStatsBuffer;
} // namespace SharedData

struct DataSharing : vk::not_copyable {
  using Storage = std::tuple<SharedData::WebServerStatsBuffer>;
  friend vk::singleton<DataSharing>;

  void init();

  template<std::size_t I>
  std::tuple_element_t<I, Storage> & acquire() const noexcept {
    return std::get<I>(*storage);
  }

  template<typename T>
  T & acquire() const noexcept {
    return std::get<T>(*storage);
  }

private:
  DataSharing() = default;
  Storage * storage;
};
