// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/file-system-state.h"

#include <string_view>

#include "common/php-functions.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/state/image-state.h"

namespace {

inline constexpr std::string_view READ_MODE_ = "r";
inline constexpr std::string_view WRITE_MODE_ = "w";
inline constexpr std::string_view APPEND_MODE_ = "a";
inline constexpr std::string_view READ_PLUS_MODE_ = "r+";
inline constexpr std::string_view WRITE_PLUS_MODE_ = "w+";
inline constexpr std::string_view APPEND_PLUS_MODE_ = "a+";

} // namespace

FileSystemImageState::FileSystemImageState() noexcept
    : READ_MODE{READ_MODE_.data(), static_cast<string::size_type>(READ_MODE_.size())},
      WRITE_MODE{WRITE_MODE_.data(), static_cast<string::size_type>(WRITE_MODE_.size())},
      APPEND_MODE{APPEND_MODE_.data(), static_cast<string::size_type>(APPEND_MODE_.size())},
      READ_PLUS_MODE{READ_PLUS_MODE_.data(), static_cast<string::size_type>(READ_PLUS_MODE_.size())},
      WRITE_PLUS_MODE{WRITE_PLUS_MODE_.data(), static_cast<string::size_type>(WRITE_PLUS_MODE_.size())},
      APPEND_PLUS_MODE{APPEND_PLUS_MODE_.data(), static_cast<string::size_type>(APPEND_PLUS_MODE_.size())} {

  READ_MODE.set_reference_counter_to(ExtraRefCnt::for_global_const);
  WRITE_MODE.set_reference_counter_to(ExtraRefCnt::for_global_const);
  APPEND_MODE.set_reference_counter_to(ExtraRefCnt::for_global_const);
  READ_PLUS_MODE.set_reference_counter_to(ExtraRefCnt::for_global_const);
  WRITE_PLUS_MODE.set_reference_counter_to(ExtraRefCnt::for_global_const);
  APPEND_PLUS_MODE.set_reference_counter_to(ExtraRefCnt::for_global_const);
}

const FileSystemImageState& FileSystemImageState::get() noexcept {
  return ImageState::get().file_system_image_state;
}
