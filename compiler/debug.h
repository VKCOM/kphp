// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// using this macro inside a class: DEBUG_STRING_METHOD { ... cpp code ...; return std::string }
// that code is also embedded into a release binary, we decided not to do anything about it
#define DEBUG_STRING_METHOD std::string _debug_string() const __attribute__((used))
