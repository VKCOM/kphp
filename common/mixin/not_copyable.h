#pragma once

namespace vk {

class not_copyable {
protected:
  not_copyable(const not_copyable &) = delete;
  not_copyable &operator=(const not_copyable &) = delete;
  not_copyable(not_copyable &&) = delete;
  not_copyable &operator=(not_copyable &&) = delete;

  not_copyable() = default;
  ~not_copyable() = default;
};

}
