// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

string f$vk_utf8_to_win(const string &text, int64_t max_len = 0, bool exit_on_error = false) noexcept;

string f$vk_win_to_utf8(const string &text, bool escape = true) noexcept;

string f$vk_flex(const string &name, const string &case_name, int64_t sex, const string &type, int64_t lang_id = 0) noexcept;

string f$vk_whitespace_pack(const string &str, bool html_opt = false) noexcept;

string f$vk_sp_simplify(const string &s) noexcept;

string f$vk_sp_full_simplify(const string &s) noexcept;

string f$vk_sp_deunicode(const string &s) noexcept;

string f$vk_sp_to_upper(const string &s) noexcept;

string f$vk_sp_to_lower(const string &s) noexcept;

string f$vk_sp_to_sort(const string &s) noexcept;

string f$vk_sp_remove_repeats(const string &s) noexcept;

string f$vk_sp_to_cyrillic(const string &s) noexcept;

string f$vk_sp_words_only(const string &s) noexcept;

inline string f$cp1251(const string &utf8_string) noexcept {
  return f$vk_utf8_to_win(utf8_string);
}
