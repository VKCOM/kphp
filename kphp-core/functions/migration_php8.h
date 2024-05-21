// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

constexpr int MIGRATION_PHP8_STRING_COMPARISON_FLAG = 1 << 0;
constexpr int MIGRATION_PHP8_STRING_TO_FLOAT_FLAG = 1 << 1;

void f$set_migration_php8_warning(int mask);

void free_migration_php8();
