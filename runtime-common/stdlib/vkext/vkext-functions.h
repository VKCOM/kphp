// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/vkext/string-processing.h"

string f$vk_utf8_to_win(const string& text, int64_t max_len = 0, bool exit_on_error = false) noexcept;

string f$vk_win_to_utf8(const string& text, bool escape = true) noexcept;

string f$vk_whitespace_pack(const string& str, bool html_opt = false) noexcept;

inline string f$vk_sp_simplify(const string& s) noexcept {
  return vkext_impl_::sp_simplify(s);
}

inline string f$vk_sp_full_simplify(const string& s) noexcept {
  return vkext_impl_::sp_full_simplify(s);
}

inline string f$vk_sp_deunicode(const string& s) noexcept {
  return vkext_impl_::sp_deunicode(s);
}

inline string f$vk_sp_to_upper(const string& s) noexcept {
  return vkext_impl_::sp_to_upper(s);
}

inline string f$vk_sp_to_lower(const string& s) noexcept {
  return vkext_impl_::sp_to_lower(s);
}

inline string f$vk_sp_to_sort(const string& s) noexcept {
  return vkext_impl_::sp_sort(s);
}

inline string f$vk_sp_remove_repeats(const string& s) noexcept {
  return vkext_impl_::sp_remove_repeats(s);
}

inline string f$vk_sp_to_cyrillic(const string& s) noexcept {
  return vkext_impl_::sp_to_cyrillic(s);
}

inline string f$vk_sp_words_only(const string& s) noexcept {
  return vkext_impl_::sp_words_only(s);
}

string f$vk_flex(const string& name, const string& case_name, int64_t sex, const string& type, int64_t lang_id = 0) noexcept;
