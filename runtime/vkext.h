#pragma once

#include "runtime/kphp_core.h"

string f$vk_utf8_to_win (const string &text, int max_len = 0, bool exit_on_error = false);

string f$vk_win_to_utf8 (const string &text);

string f$vk_flex (const string &name, const string &case_name, int sex, const string &type, int lang_id = 0);

string f$vk_json_encode (const var &v);

string f$vk_whitespace_pack (const string &str, bool html_opt = false);

string f$vk_sp_simplify (const string& s);

string f$vk_sp_full_simplify (const string& s);

string f$vk_sp_deunicode (const string& s);

string f$vk_sp_to_upper (const string& s);

string f$vk_sp_to_lower (const string& s);

string f$vk_sp_to_sort (const string& s);

string f$vk_sp_remove_repeats (const string& s);

string f$vk_sp_to_cyrillic (const string& s);

string f$vk_sp_words_only (const string& s);