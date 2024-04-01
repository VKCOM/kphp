// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/common_template_instantiations.h"

template class array<int64_t>;
template class array<string>;
template class array<mixed>;
// template class array<double>; // see the header for comments
template class array<bool>;

template class array<Optional<int64_t>>;
template class array<Optional<string>>;
template class array<Optional<double>>;
template class array<Optional<bool>>;

template class array<array<int64_t>>;
template class array<array<string>>;
template class array<array<double>>;
template class array<array<bool>>;
template class array<array<mixed>>;

template class Optional<int64_t>;
template class Optional<string>;
template class Optional<double>;

template class Optional<array<int64_t>>;
template class Optional<array<string>>;
template class Optional<array<double>>;
template class Optional<array<bool>>;
template class Optional<array<mixed>>;
