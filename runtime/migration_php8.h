// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

extern bool show_number_string_conversion_warning;

bool f$set_show_number_string_conversion_warning(bool show);

void free_migration_php8();
