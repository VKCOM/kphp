// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#define PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2/pcre2.h"

using regex_pcre2_general_context_t = std::unique_ptr<pcre2_general_context_8, decltype(std::addressof(pcre2_general_context_free_8))>;
using regex_pcre2_compile_context_t = std::unique_ptr<pcre2_compile_context_8, decltype(std::addressof(pcre2_compile_context_free_8))>;
using regex_pcre2_match_context_t = std::unique_ptr<pcre2_match_context_8, decltype(std::addressof(pcre2_match_context_free_8))>;
using regex_pcre2_match_data_t = std::unique_ptr<pcre2_match_data_8, decltype(std::addressof(pcre2_match_data_free_8))>;
using regex_pcre2_code_t = std::unique_ptr<pcre2_code_8, decltype(std::addressof(pcre2_code_free_8))>;
