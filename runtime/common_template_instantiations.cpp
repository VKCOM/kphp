// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/common_template_instantiations.h"

template class __runtime_core::array<int64_t, ScriptAllocator>;
template class __runtime_core::array<string, ScriptAllocator>;
/*
 * Commented out, because it breaks @kphp-flatten optimization.
 * When array<double> is explicitly instantiated here it's compiled with -O3 instead of -Os.
 * It spoils inlining of array<>::get_value functions called from @kphp-flatten annotated ML related functions:
 * some strange `call` asm instructions are generated instead of normal inlining.
 */
// template class array<double>;
template class __runtime_core::array<bool, ScriptAllocator>;
template class __runtime_core::array<mixed, ScriptAllocator>;

template class __runtime_core::array<Optional<int64_t>, ScriptAllocator>;
template class __runtime_core::array<Optional<string>, ScriptAllocator>;
template class __runtime_core::array<Optional<double>, ScriptAllocator>;
template class __runtime_core::array<Optional<bool>, ScriptAllocator>;

template class __runtime_core::array<__runtime_core::array<int64_t, ScriptAllocator>, ScriptAllocator>;
template class __runtime_core::array<__runtime_core::array<string, ScriptAllocator>, ScriptAllocator>;
template class __runtime_core::array<__runtime_core::array<double, ScriptAllocator>, ScriptAllocator>;
template class __runtime_core::array<__runtime_core::array<bool, ScriptAllocator>, ScriptAllocator>;
template class __runtime_core::array<__runtime_core::array<mixed, ScriptAllocator>, ScriptAllocator>;

template class Optional<int64_t>;
template class Optional<string>;
template class Optional<double>;

template class Optional<array<int64_t>>;
template class Optional<array<string>>;
template class Optional<array<double>>;
template class Optional<array<bool>>;
template class Optional<array<mixed>>;
