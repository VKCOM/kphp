// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/runtime-types.h"

// Use explicit template instantiation to make result binary smaller and force common instantiations to be compiled with -O3
// see https://en.cppreference.com/w/cpp/language/class_template

extern template class __runtime_core::array<int64_t, ScriptAllocator>;
extern template class __runtime_core::array<string, ScriptAllocator>;
/*
 * Commented out, because it breaks @kphp-flatten optimization.
 * When array<double> is explicitly instantiated here it's compiled with -O3 instead of -Os.
 * It spoils inlining of array<>::get_value functions called from @kphp-flatten annotated ML related functions:
 * some strange `call` asm instructions are generated instead of normal inlining.
 */
// extern template class array<double>;
extern template class __runtime_core::array<bool, ScriptAllocator>;
extern template class __runtime_core::array<mixed, ScriptAllocator>;

extern template class __runtime_core::array<Optional<int64_t>, ScriptAllocator>;
extern template class __runtime_core::array<Optional<string>, ScriptAllocator>;
extern template class __runtime_core::array<Optional<double>, ScriptAllocator>;
extern template class __runtime_core::array<Optional<bool>, ScriptAllocator>;

extern template class __runtime_core::array<__runtime_core::array<int64_t, ScriptAllocator>, ScriptAllocator>;
extern template class __runtime_core::array<__runtime_core::array<string, ScriptAllocator>, ScriptAllocator>;
extern template class __runtime_core::array<__runtime_core::array<double, ScriptAllocator>, ScriptAllocator>;
extern template class __runtime_core::array<__runtime_core::array<bool, ScriptAllocator>, ScriptAllocator>;
extern template class __runtime_core::array<__runtime_core::array<mixed, ScriptAllocator>, ScriptAllocator>;

extern template class Optional<int64_t>;
extern template class Optional<string>;
extern template class Optional<double>;

extern template class Optional<array<int64_t>>;
extern template class Optional<array<string>>;
extern template class Optional<array<double>>;
extern template class Optional<array<bool>>;
extern template class Optional<array<mixed>>;
