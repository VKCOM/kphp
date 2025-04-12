// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>

char* simd_uint32_to_string(uint32_t value, char* out_buffer) noexcept;
char* simd_int32_to_string(int32_t value, char* out_buffer) noexcept;
char* simd_uint64_to_string(uint64_t value, char* out_buffer);
char* simd_int64_to_string(int64_t value, char* out_buffer);
