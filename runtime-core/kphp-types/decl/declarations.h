// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

class Unknown {
};

struct array_size;

template<typename T>
class Optional;

struct C$RpcConnection;

namespace __runtime_core {
template<class T, typename Allocator>
class class_instance;

template<class T, typename Allocator>
class array;

template<typename Allocator>
class mixed;

template<typename Allocator>
class string;

template<typename Allocator>
class string_buffer;

/**
 * This allocator used for compile time checks
 * */
class DummyAllocator {
  void * allocate(int size) {return nullptr;}
  void * allocate0(int size) {return nullptr;}
  void deallocate(void * p, int size) {}
  void * reallocate(void * p, int old_size, int new_size){return nullptr;}
};
}

template<typename T, typename Allocator>
using __class_instance = __runtime_core::class_instance<T, Allocator>;

template<typename Allocator>
using __string = __runtime_core::string<Allocator>;

template<typename Allocator>
using __mixed = __runtime_core::mixed<Allocator>;

template<typename T, typename Allocator>
using __array = __runtime_core::array<T, Allocator>;

template<typename Allocator>
using __string_buffer = __runtime_core::string_buffer<Allocator>;
