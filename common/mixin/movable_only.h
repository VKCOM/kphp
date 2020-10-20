#pragma once

namespace vk {

class movable_only {
protected:
  movable_only(const movable_only &) = delete;
  movable_only &operator=(const movable_only &) = delete;

  movable_only() = default;
  ~movable_only() = default;
  movable_only(movable_only &&) = default;
  movable_only &operator=(movable_only &&) = default;
};

}
