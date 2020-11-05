// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

namespace vk {

template<typename V, typename T>
struct is_contiguous_iterator_of_type : std::false_type {};

template<typename V>
struct is_contiguous_iterator_of_type<V, std::remove_const_t<V>*> : std::true_type {};
template<typename V>
struct is_contiguous_iterator_of_type<const V, const V*> : std::true_type {};
template<typename V>
struct is_contiguous_iterator_of_type<V, std::nullptr_t> : std::true_type {};

template<>                                                                                                                        \
struct is_contiguous_iterator_of_type<const bool, typename std::vector<bool>::const_iterator> : std::false_type {};                    \

template<>                                                                                                                        \
struct is_contiguous_iterator_of_type<const bool, typename std::vector<bool>::iterator> : std::false_type {};
} // namespace vk

#define DECLARE_IS_CONTIGUOUS_FOR_ALLOCATOR(vector, string)                                                                       \
namespace vk {                                                                                                                    \
template<typename V>                                                                                                              \
struct is_contiguous_iterator_of_type<V, typename vector<std::remove_const_t<V>>::iterator> : std::true_type {};                  \
template<typename V>                                                                                                              \
struct is_contiguous_iterator_of_type<const V, typename vector<std::remove_const_t<V>>::const_iterator> : std::true_type {};      \
template<>                                                                                                                        \
struct is_contiguous_iterator_of_type<char, typename string::iterator> : std::true_type {};                                       \
template<>                                                                                                                        \
struct is_contiguous_iterator_of_type<const char, typename string::iterator> : std::true_type {};                                 \
template<>                                                                                                                        \
struct is_contiguous_iterator_of_type<const char, typename string::const_iterator> : std::true_type {};                           \
}

DECLARE_IS_CONTIGUOUS_FOR_ALLOCATOR(std::vector, std::string)
