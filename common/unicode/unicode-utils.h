// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

inline constexpr size_t MAX_NAME_SIZE = 65536;
inline constexpr size_t MAX_NAME_BYTES_SIZE = 4 * MAX_NAME_SIZE + 4;
inline constexpr size_t MAX_NAME_CODE_POINTS_SIZE = MAX_NAME_SIZE + 4;

int unicode_toupper(int code);
int unicode_tolower(int code);
size_t clean_str(const char* x, int32_t* code_points, size_t* word_start_indices, int32_t* prepared_code_points, std::byte* utf8_result,
                 void (*assertf)(bool));
