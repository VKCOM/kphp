// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

namespace vkext_impl_ {

// Returns sorted s string.
string sp_sort(const string& s) noexcept;

// Returns upper/lower case string for s in cp1251.
string sp_to_upper(const string& s) noexcept;
string sp_to_lower(const string& s) noexcept;

/**
 * Simplifications: look to source code to see full list of replacements.
 */

// Returns simplified s.
// Deletes all except digits, latin and russian letters in cp1251, lowercase letters.
string sp_simplify(const string& s) noexcept;

// Returns ultra-simplified s.
// Recognizes unicode characters encoded in cp1251 and html-entities. Remove diacritics
// from unicode characters, delete all except digits, latin and russian letters, lowercase
// letters. Unifies similar russian and english characters (i.e. ('n'|'п') --> 'п')
string sp_full_simplify(const string& s) noexcept;

// Converts all unicode characters encoded in cp1251 and html-entities into real cp1251,
// removing diacritics if possible. If converting is impossible - removes such characters.
string sp_deunicode(const string& s) noexcept;

string sp_remove_repeats(const string& s) noexcept;
string sp_to_cyrillic(const string& s) noexcept;
string sp_words_only(const string& s) noexcept;

} // namespace vkext_impl_
