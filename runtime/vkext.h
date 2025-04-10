// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

#include "runtime-common/stdlib/vkext/vkext-functions.h"

string f$vk_flex(const string& name, const string& case_name, int64_t sex, const string& type, int64_t lang_id = 0) noexcept;
