// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp-core/kphp_core.h"

// Use explicit template instantiation to make result binary smaller and force common instantiations to be compiled with -O3
// see https://en.cppreference.com/w/cpp/language/class_template

extern template class array<int64_t>;
extern template class array<string>;
/*
 * Commented out, because it breaks @kphp-flatten optimization.
 * When array<double> is explicitly instantiated here it's compiled with -O3 instead of -Os.
 * It spoils inlining of array<>::get_value functions called from @kphp-flatten annotated ML related functions:
 * some strange `call` asm instructions are generated instead of normal inlining.
 */
// extern template class array<double>;
extern template class array<bool>;
extern template class array<mixed>;

extern template class array<Optional<int64_t>>;
extern template class array<Optional<string>>;
extern template class array<Optional<double>>;
extern template class array<Optional<bool>>;

extern template class array<array<int64_t>>;
extern template class array<array<string>>;
extern template class array<array<double>>;
extern template class array<array<bool>>;
extern template class array<array<mixed>>;

extern template class Optional<int64_t>;
extern template class Optional<string>;
extern template class Optional<double>;

extern template class Optional<array<int64_t>>;
extern template class Optional<array<string>>;
extern template class Optional<array<double>>;
extern template class Optional<array<bool>>;
extern template class Optional<array<mixed>>;
